// IHostManagement.h
//
// 宿主与管理服务器之间的抽象接口（核心松耦合契约）。
// 宿主仅实现本接口，不依赖 cpp-httplib / websocketpp / HTTP / JSON。
// 详见 docs/技术方案.md 第 4 节。
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace mgmt {

struct HostStatus {
    std::string state;        // running / degraded / stopped
    uint64_t    uptimeMs;
    std::string version;
    int         pid;
};

struct Metric {
    std::string name;
    double      value;
    std::string unit;
};

struct ConfigItem {
    std::string key;
    std::string value;
    bool        readOnly;
};

struct LogEntry {
    uint64_t    timestampMs;
    std::string level;        // info / warn / error
    std::string message;
};

struct Event {                // 服务器推送事件
    std::string type;         // log / metric / state / alert
    std::string payloadJson;
};

class IHostManagement {
public:
    virtual ~IHostManagement() {}

    // ---- 数据接口 ----
    virtual HostStatus              getStatus() = 0;
    virtual std::vector<Metric>     getMetrics() = 0;
    virtual std::vector<ConfigItem> getConfig() = 0;
    virtual bool                    setConfig(const std::string& key,
                                              const std::string& value,
                                              std::string& err) = 0;
    virtual std::vector<LogEntry>   getLogs(uint64_t sinceMs,
                                            uint32_t limit) = 0;

    // ---- 调用接口 ----
    // restart: 异步操作，调用后宿主可能立即退出进程。
    //   宿主实现只负责"触发进程退出"，不负责推送事件或关闭连接——
    //   桥接层在调用本方法前已完成 accepted 响应、restart scheduled 事件推送与 WS 连接关闭。
    //   完整时序见 docs/技术方案.md 4.3 节。
    virtual bool restart(std::string& err) = 0;
    virtual bool reloadConfig(std::string& err) = 0;
    virtual bool runDiagnostics(std::string& resultJson,
                                std::string& err) = 0;

    // ---- 认证接口 ----
    // 成功返回 true，token 出参为会话令牌；失败 err 填原因
    virtual bool login(const std::string& user,
                       const std::string& password,
                       std::string& token,
                       std::string& err) = 0;

    // ---- 事件订阅（用于向 Web 端推送）----
    using EventCallback = std::function<void(const Event&)>;
    virtual void setEventListener(EventCallback cb) = 0;

    // ---- Token 校验（由桥接层在 WS 鉴权阶段调用）----
    struct SessionInfo {
        std::string user;
        std::string role;   // admin / viewer
    };
    virtual bool validateToken(const std::string& token,
                               SessionInfo& session,
                               std::string& err) = 0;
};

} // namespace mgmt
