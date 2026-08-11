// 双通道管理服务器 + 内存态示例宿主。
// 信号：SIGINT/SIGTERM 退出；SIGHUP 热重载证书。
// 环境变量：MINIWEB_ALLOWED_ORIGINS（逗号分隔的 Origin 白名单，默认空=不校验）。
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
    setvbuf(stdout, nullptr, _IONBF, 0);   // 日志无缓冲（运维实时可见）
    if (int rc = proto::demo()) return rc;

    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);
    std::signal(SIGHUP, on_sighup);

    // Origin 白名单：env MINIWEB_ALLOWED_ORIGINS（逗号分隔），默认空 = 不校验
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
    if (!server.start(8443, 8444, "certs/cert.pem", "certs/key.pem")) {
        host.stop();
        return 1;
    }

    std::printf("miniweb running. login: admin/admin. Ctrl-C quit, SIGHUP reload certs.\n");
    if (!origins.empty()) std::printf("[Origin whitelist] %zu entries\n", origins.size());

    while (!g_stop.load()) {
        if (g_reload.exchange(false)) server.reloadCerts();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::printf("shutting down...\n");
    server.stop();
    host.stop();
    return 0;
}
