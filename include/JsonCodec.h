// JsonCodec.h
//
// WSS 通道 JSON 消息的解析与组装（基于 RapidJSON）。
// 对外只暴露 std::string 与结构体，RapidJSON 类型不泄漏到头文件。
#pragma once
#include "IHostManagement.h"
#include "Protocol.h"
#include <string>
#include <vector>
#include <cstdint>

namespace proto {

// ---- 解析结果结构 ----
struct AuthMsg      { std::string token; std::string proto; };
struct RequestMsg   { std::string id; Action action; std::string paramsJson; };
struct SubscribeMsg { std::string id; std::vector<std::string> events; };

// ---- 顶层 kind ----
bool parseKind(const std::string& msg, std::string& kind);

// ---- 消息解析（失败返回 false，err 出参原因）----
bool parseAuth     (const std::string& msg, AuthMsg& out, std::string& err);
bool parseRequest  (const std::string& msg, RequestMsg& out, std::string& err);
bool parseSubscribe(const std::string& msg, SubscribeMsg& out, std::string& err);

// ---- params 子解析 ----
bool parseSetConfigParams(const std::string& paramsJson,
                          std::string& key, std::string& value, std::string& err);
bool parseGetLogsParams  (const std::string& paramsJson,
                          uint64_t& sinceMs, uint32_t& limit, std::string& err);

// ---- 组装 ----
std::string makeAuthOk(const mgmt::IHostManagement::SessionInfo& session);
std::string makeAuthError(const std::string& code, const std::string& message);
std::string makeResponseOk      (const std::string& id, const std::string& dataJson);
std::string makeResponseError   (const std::string& id, const std::string& code, const std::string& message);
std::string makeResponseAccepted(const std::string& id, const std::string& message);
std::string makeEvent(const std::string& eventType, const std::string& dataJson);
std::string makePong();

// ---- 结构体 → JSON 片段（用于 response.data 或 event.data）----
std::string toJson(const mgmt::HostStatus& s);
std::string toJson(const std::vector<mgmt::Metric>& ms);
std::string toJson(const std::vector<mgmt::ConfigItem>& cs);
std::string toJson(const std::vector<mgmt::LogEntry>& ls);

// ---- 单条 event 的 data（区别于上面的数组形式）----
std::string logEventPayload   (const mgmt::LogEntry& l);
std::string metricEventPayload(const mgmt::Metric& m);

// ---- self-check（非 0 表示有失败项）----
int demo();

} // namespace proto
