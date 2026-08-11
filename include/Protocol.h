// Protocol.h
//
// WSS 通道 JSON 协议常量与 action 枚举。详见 docs/技术方案.md 第 5 节。
#pragma once
#include <string>

namespace proto {

// ---- kind ----
constexpr const char* K_AUTH        = "auth";
constexpr const char* K_AUTH_OK     = "auth_ok";
constexpr const char* K_REQUEST     = "request";
constexpr const char* K_RESPONSE    = "response";
constexpr const char* K_EVENT       = "event";
constexpr const char* K_PING        = "ping";
constexpr const char* K_PONG        = "pong";
constexpr const char* K_SUBSCRIBE   = "subscribe";
constexpr const char* K_UNSUBSCRIBE = "unsubscribe";
constexpr const char* K_ERROR       = "error";   // 鉴权阶段无 id 的错误帧

// ---- status ----
constexpr const char* S_OK       = "ok";
constexpr const char* S_ERROR    = "error";
constexpr const char* S_ACCEPTED = "accepted";

// ---- error codes ----
constexpr const char* E_PERM_DENIED = "PERM_DENIED";
constexpr const char* E_BAD_REQUEST = "BAD_REQUEST";
constexpr const char* E_INTERNAL    = "INTERNAL";
constexpr const char* E_NOT_FOUND   = "NOT_FOUND";
constexpr const char* E_RATE_LIMIT  = "RATE_LIMIT";

// ---- event types ----
constexpr const char* EV_LOG    = "log";
constexpr const char* EV_METRIC = "metric";
constexpr const char* EV_STATE  = "state";
constexpr const char* EV_ALERT  = "alert";

// ---- protocol version ----
constexpr const char* PROTO_VER = "v1";

// ---- action 枚举（请求路由用）----
enum class Action {
    getStatus, getMetrics, getConfig, setConfig, getLogs,
    restart, reloadConfig, runDiagnostics, unknown
};
Action      parseAction(const std::string& s);
const char* actionName(Action a);

} // namespace proto
