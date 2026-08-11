// ExampleHost.h
//
// 内存态示例宿主，实现 IHostManagement，用于演示与端到端验证。
// 真实宿主自行实现该接口；本示例仅为跑通通道。
//
// 注：Event.payloadJson 需要 JSON 文本，本示例复用 JsonCodec 的序列化。
// 这不违反"宿主不依赖 HTTP/WS"——JSON 是数据载体，宿主序列化自身数据合理。
#pragma once
#include "IHostManagement.h"
#include "JsonCodec.h"   // logEventPayload / metricEventPayload / EV_*

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <openssl/rand.h>
#include <unistd.h>

class ExampleHost : public mgmt::IHostManagement {
public:
    // tokenTtlMs：会话 token 有效期，默认 24h（测试可传短值验证过期）
    explicit ExampleHost(uint64_t tokenTtlMs = 24ULL * 3600 * 1000)
        : start_(std::chrono::steady_clock::now()), ttlMs_(tokenTtlMs) {
        // P3: 口令从 env 读（默认 admin/viewer，便于生产覆盖默认值）
        const char* a = std::getenv("MINIWEB_ADMIN_PASS");
        const char* v = std::getenv("MINIWEB_VIEWER_PASS");
        adminPass_  = a ? a : "admin";
        viewerPass_ = v ? v : "viewer";
        config_ = {
            {"log_level",       "info",  false},
            {"max_connections", "1024",  false},
            {"version",         "1.0.0", true},
        };
        logs_.push_back({nowMs(), "info", "ExampleHost started"});
    }
    ~ExampleHost() override { stop(); }

    void start() { evtThread_ = std::thread(&ExampleHost::eventLoop, this); }
    void stop() {
        if (stopping_.exchange(true)) return;
        if (evtThread_.joinable()) evtThread_.join();
    }

    // ---- 数据接口 ----
    mgmt::HostStatus getStatus() override {
        std::lock_guard<std::mutex> lk(mu_);
        uint64_t up = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_).count();
        return {"running", up, "1.0.0", (int)getpid()};
    }
    std::vector<mgmt::Metric> getMetrics() override {
        long s = (long)std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_).count();
        return {
            {"cpu",    20.0 + (s % 40), "%"},
            {"memory", 40.0 + (s % 30), "%"},
            {"qps",    100.0 + (s % 50), "req/s"},
        };
    }
    std::vector<mgmt::ConfigItem> getConfig() override {
        std::lock_guard<std::mutex> lk(mu_);
        return config_;
    }
    bool setConfig(const std::string& key, const std::string& value, std::string& err) override {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto& c : config_) {
            if (c.key == key) {
                if (c.readOnly) { err = "read-only"; return false; }
                c.value = value;
                return true;
            }
        }
        err = "not found";
        return false;
    }
    std::vector<mgmt::LogEntry> getLogs(uint64_t sinceMs, uint32_t limit) override {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<mgmt::LogEntry> out;
        for (const auto& l : logs_) {
            if (l.timestampMs > sinceMs) {
                out.push_back(l);
                if (out.size() >= limit) break;
            }
        }
        return out;
    }

    // ---- 调用接口 ----
    bool restart(std::string& err) override         { (void)err; return true; } // 示例不真退出
    bool reloadConfig(std::string& err) override    { (void)err; return true; }
    bool runDiagnostics(std::string& resultJson, std::string& err) override {
        (void)err;
        resultJson = "{\"checks\":[{\"name\":\"db\",\"ok\":true}],\"summary\":\"all good\"}";
        return true;
    }

    // ---- 认证接口 ----
    bool login(const std::string& user, const std::string& password,
               std::string& token, std::string& err) override {
        std::string role;
        if      (user == "admin"  && password == adminPass_)  role = "admin";
        else if (user == "viewer" && password == viewerPass_) role = "viewer";
        else { err = "invalid credentials"; return false; }
        token = randomToken();
        std::lock_guard<std::mutex> lk(mu_);
        sessions_[token] = {user, role, nowMs() + ttlMs_};
        return true;
    }
    bool validateToken(const std::string& token, SessionInfo& session, std::string& err) override {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = sessions_.find(token);
        if (it == sessions_.end()) { err = "invalid token"; return false; }
        if (nowMs() > it->second.expiryMs) { sessions_.erase(it); err = "token expired"; return false; }
        session.user = it->second.user;
        session.role = it->second.role;
        return true;
    }

    void setEventListener(EventCallback cb) override {
        std::lock_guard<std::mutex> lk(mu_);
        cb_ = std::move(cb);
    }

private:
    struct Session { std::string user; std::string role; uint64_t expiryMs; };

    static uint64_t nowMs() {
        return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    static std::string randomToken() {
        unsigned char buf[16];
        if (RAND_bytes(buf, sizeof(buf)) != 1) {   // P1: 密码学 RNG（极小概率失败时降级）
            std::random_device rd;
            for (int i = 0; i < 16; ++i) buf[i] = (unsigned char)(rd() & 0xff);
        }
        static const char d[] = "0123456789abcdef";
        std::string hex; hex.reserve(32);
        for (int i = 0; i < 16; ++i) { hex += d[buf[i] >> 4]; hex += d[buf[i] & 0xf]; }
        return hex;
    }

    void eventLoop() {
        long tick = 0;
        while (true) {
            for (int i = 0; i < 10; ++i) {            // 10×200ms = 2s
                if (stopping_.load()) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            {   // P1: 清扫过期 session，防止无界增长
                std::lock_guard<std::mutex> lk(mu_);
                for (auto it = sessions_.begin(); it != sessions_.end(); ) {
                    if (nowMs() > it->second.expiryMs) it = sessions_.erase(it);
                    else ++it;
                }
            }
            EventCallback cb;
            { std::lock_guard<std::mutex> lk(mu_); cb = cb_; }
            if (!cb) continue;

            double cpu = 20.0 + (tick % 40);
            mgmt::Metric m{"cpu", cpu, "%"};
            cb(mgmt::Event{proto::EV_METRIC, proto::metricEventPayload(m)});

            uint64_t ts = nowMs();
            mgmt::LogEntry le{ts, "info", "tick #" + std::to_string(tick)};
            {
                std::lock_guard<std::mutex> lk(mu_);
                logs_.push_back(le);
                if (logs_.size() > 1000) logs_.pop_front();
            }
            cb(mgmt::Event{proto::EV_LOG, proto::logEventPayload(le)});
            ++tick;
        }
    }

    std::mutex mu_;
    std::unordered_map<std::string, Session> sessions_;
    std::vector<mgmt::ConfigItem> config_;
    std::deque<mgmt::LogEntry>    logs_;
    EventCallback cb_;
    std::atomic<bool> stopping_{false};
    std::thread evtThread_;
    std::chrono::steady_clock::time_point start_;
    uint64_t ttlMs_;
    std::string adminPass_, viewerPass_;
};
