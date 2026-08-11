// 双通道管理服务器 + 内存态示例宿主。
// 信号：SIGINT/SIGTERM 退出；SIGHUP 热重载证书；SIGPIPE 忽略（防 send 遇 RST 杀进程）。
// 环境变量：
//   MINIWEB_BIND_ADDR        绑定地址，默认 127.0.0.1（运维通道不暴露所有网卡）
//   MINIWEB_ALLOWED_ORIGINS  Origin 白名单（逗号分隔），默认空=本地策略
#include "ManagementServer.h"
#include "ExampleHost.h"
#include "JsonCodec.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static std::atomic<bool> g_stop{false};
static std::atomic<bool> g_reload{false};
static void on_sig(int)    { g_stop.store(true); }
static void on_sighup(int) { g_reload.store(true); }

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (int rc = proto::demo()) return rc;

    std::signal(SIGPIPE, SIG_IGN);   // send 遇对端 RST 不再杀进程
    // P3: sigaction 替代 signal（可移植性 + SA_RESTART 自动重启被中断的系统调用）
    struct sigaction sa{};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = on_sig;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sa.sa_handler = on_sighup;
    sigaction(SIGHUP, &sa, nullptr);

    // P1: 默认仅绑 loopback；env 切换（生产可设 0.0.0.0 + Origin 白名单 + 防火墙）
    const char* bindRaw = std::getenv("MINIWEB_BIND_ADDR");
    std::string bindAddr = (bindRaw && *bindRaw) ? bindRaw : "127.0.0.1";

    // Origin 白名单：env MINIWEB_ALLOWED_ORIGINS（逗号分隔），默认空=本地策略
    std::vector<std::string> origins;
    if (const char* e = std::getenv("MINIWEB_ALLOWED_ORIGINS")) {
        std::stringstream ss(e);
        std::string tok;
        while (std::getline(ss, tok, ',')) if (!tok.empty()) origins.push_back(tok);
    }

    ExampleHost host;
    host.start();

    ManagementServer server(host, "web");
    server.setAllowedOrigins(origins);
    if (!server.start(8443, 8444, "certs/cert.pem", "certs/key.pem", bindAddr)) {
        host.stop();
        return 1;
    }

    std::printf("miniweb running on %s. login: admin (see docs). Ctrl-C quit, SIGHUP reload certs.\n",
                bindAddr.c_str());
    if (!origins.empty()) std::printf("[Origin whitelist] %zu entries\n", origins.size());

    while (!g_stop.load()) {
        if (g_reload.exchange(false)) server.reloadCerts();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::printf("shutting down...\n");
    host.stop();          // P1: 先 join 事件线程，避免回调访问已析构的 ws_
    server.stop();
    return 0;
}
