// tests/test_host.cpp — ExampleHost 真实测试（非 mock）
#include "test_common.h"
#include "ExampleHost.h"

#include <atomic>
#include <chrono>
#include <thread>

static void test_auth() {
    ExampleHost host;
    std::string token, err;
    CHECK(host.login("admin", "admin", token, err) && !token.empty());
    CHECK(host.login("viewer", "viewer", token, err) && !token.empty());
    CHECK(!host.login("admin", "wrong", token, err) && err.find("credentials") != std::string::npos);
    CHECK(!host.login("nobody", "x", token, err) && err.find("credentials") != std::string::npos);

    std::string t, e;
    CHECK(host.login("admin", "admin", t, e));
    mgmt::IHostManagement::SessionInfo sess;
    CHECK(host.validateToken(t, sess, e));
    CHECK(sess.user == "admin" && sess.role == "admin");
    CHECK(!host.validateToken("deadbeef", sess, e) && e.find("token") != std::string::npos);  // 无效 token + err
}

static void test_status_metrics_config() {
    ExampleHost host;
    auto st = host.getStatus();
    CHECK(st.state == "running");
    CHECK(st.pid > 0);
    CHECK(st.version == "1.0.0");

    CHECK(host.getMetrics().size() >= 1);

    auto cfg = host.getConfig();
    CHECK(cfg.size() >= 1);
    bool hasReadOnly = false;
    for (const auto& c : cfg) if (c.readOnly) hasReadOnly = true;
    CHECK(hasReadOnly);
}

static void test_set_config() {
    ExampleHost host;
    std::string err;
    CHECK(host.setConfig("log_level", "debug", err));          // 存在且可写
    bool found = false;
    for (const auto& c : host.getConfig())
        if (c.key == "log_level" && c.value == "debug") found = true;
    CHECK(found);
    CHECK(!host.setConfig("version", "9.9.9", err) && err.find("read") != std::string::npos);  // 只读项 + err
    CHECK(!host.setConfig("nope", "x", err) && err.find("found") != std::string::npos);        // 不存在 + err
}

static void test_logs_incremental() {
    ExampleHost host;
    auto all = host.getLogs(0, 1000);
    CHECK(!all.empty());                                       // 含启动日志
    uint64_t firstTs = all.front().timestampMs;
    auto after = host.getLogs(firstTs, 1000);                  // sinceMs=firstTs 应排除该条
    for (const auto& l : after) CHECK(l.timestampMs > firstTs);
}

static void test_diagnostics_and_restart() {
    ExampleHost host;
    std::string r, e;
    CHECK(host.runDiagnostics(r, e) && !r.empty());
    CHECK(host.reloadConfig(e));
    CHECK(host.restart(e));
}

static void test_event_callback_fires() {
    ExampleHost host;
    host.start();
    std::atomic<int> count{0};
    host.setEventListener([&count](const mgmt::Event&) { count.fetch_add(1); });
    std::this_thread::sleep_for(std::chrono::seconds(3));      // >2s 至少一轮推送
    CHECK(count.load() > 0);
    host.stop();
}

static void test_token_expiry() {
    ExampleHost host(0);   // ttl=0 → 颁发即过期
    std::string token, e;
    CHECK(host.login("admin", "admin", token, e));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));   // 确保 ms 前进
    mgmt::IHostManagement::SessionInfo sess;
    CHECK(!host.validateToken(token, sess, e) && e.find("expired") != std::string::npos);
}

int main() {
    test_auth();
    test_status_metrics_config();
    test_set_config();
    test_logs_incremental();
    test_diagnostics_and_restart();
    test_event_callback_fires();
    test_token_expiry();
    TEST_SUMMARY("test_host");
}
