// JsonCodec.cpp — 基于 RapidJSON 的协议消息解析与组装
#include "JsonCodec.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <cassert>
#include <cstdio>

using rapidjson::Document;
using rapidjson::Value;
using rapidjson::StringBuffer;
using rapidjson::Writer;
using rapidjson::kObjectType;

namespace proto {

// ============================================================
// action 枚举
// ============================================================
Action parseAction(const std::string& s) {
    if (s == "getStatus")      return Action::getStatus;
    if (s == "getMetrics")     return Action::getMetrics;
    if (s == "getConfig")      return Action::getConfig;
    if (s == "setConfig")      return Action::setConfig;
    if (s == "getLogs")        return Action::getLogs;
    if (s == "restart")        return Action::restart;
    if (s == "reloadConfig")   return Action::reloadConfig;
    if (s == "runDiagnostics") return Action::runDiagnostics;
    return Action::unknown;
}
const char* actionName(Action a) {
    switch (a) {
        case Action::getStatus:      return "getStatus";
        case Action::getMetrics:     return "getMetrics";
        case Action::getConfig:      return "getConfig";
        case Action::setConfig:      return "setConfig";
        case Action::getLogs:        return "getLogs";
        case Action::restart:        return "restart";
        case Action::reloadConfig:   return "reloadConfig";
        case Action::runDiagnostics: return "runDiagnostics";
        default:                     return "unknown";
    }
}

// ============================================================
// 解析
// ============================================================
static bool isObj(const std::string& msg, Document& doc, std::string& err) {
    if (doc.Parse(msg.c_str()).HasParseError() || !doc.IsObject()) {
        err = "invalid json object";
        return false;
    }
    return true;
}

bool parseKind(const std::string& msg, std::string& kind) {
    Document doc;
    if (doc.Parse(msg.c_str()).HasParseError() || !doc.IsObject()) return false;
    if (!doc.HasMember("kind") || !doc["kind"].IsString())         return false;
    kind = doc["kind"].GetString();
    return true;
}

bool parseAuth(const std::string& msg, AuthMsg& out, std::string& err) {
    Document doc;
    if (!isObj(msg, doc, err)) return false;
    std::string k = doc.HasMember("kind") && doc["kind"].IsString() ? doc["kind"].GetString() : "";
    if (k != K_AUTH) { err = "not auth"; return false; }
    if (!doc.HasMember("token") || !doc["token"].IsString()) { err = "missing token"; return false; }
    out.token = doc["token"].GetString();
    out.proto = (doc.HasMember("proto") && doc["proto"].IsString()) ? doc["proto"].GetString() : PROTO_VER;
    return true;
}

bool parseRequest(const std::string& msg, RequestMsg& out, std::string& err) {
    Document doc;
    if (!isObj(msg, doc, err)) return false;
    std::string k = doc.HasMember("kind") && doc["kind"].IsString() ? doc["kind"].GetString() : "";
    if (k != K_REQUEST) { err = "not request"; return false; }
    if (!doc.HasMember("id") || !doc["id"].IsString()) { err = "missing id"; return false; }
    out.id = doc["id"].GetString();
    if (!doc.HasMember("action") || !doc["action"].IsString()) { err = "missing action"; return false; }
    out.action = parseAction(doc["action"].GetString());
    // params 原样保留为 JSON 文本（供子解析器使用）
    if (doc.HasMember("params") && doc["params"].IsObject()) {
        StringBuffer sb; Writer<StringBuffer> w(sb);
        doc["params"].Accept(w);
        out.paramsJson = sb.GetString();
    } else {
        out.paramsJson = "{}";
    }
    return true;
}

bool parseSubscribe(const std::string& msg, SubscribeMsg& out, std::string& err) {
    out.id.clear();
    out.events.clear();      // 解析函数契约：先清空输出参数（避免复用累积）
    Document doc;
    if (!isObj(msg, doc, err)) return false;
    std::string k = doc.HasMember("kind") && doc["kind"].IsString() ? doc["kind"].GetString() : "";
    if (k != K_SUBSCRIBE && k != K_UNSUBSCRIBE) { err = "not subscribe"; return false; }
    if (doc.HasMember("id") && doc["id"].IsString()) out.id = doc["id"].GetString();
    if (!doc.HasMember("events") || !doc["events"].IsArray()) { err = "missing events"; return false; }
    for (const auto& e : doc["events"].GetArray()) {
        if (e.IsString()) out.events.push_back(e.GetString());
    }
    return true;
}

bool parseSetConfigParams(const std::string& paramsJson,
                          std::string& key, std::string& value, std::string& err) {
    Document doc;
    if (!isObj(paramsJson, doc, err)) return false;
    if (!doc.HasMember("key") || !doc["key"].IsString()) { err = "missing key"; return false; }
    if (!doc.HasMember("value") || !doc["value"].IsString()) { err = "missing value"; return false; }
    key = doc["key"].GetString();
    value = doc["value"].GetString();
    return true;
}

bool parseGetLogsParams(const std::string& paramsJson,
                        uint64_t& sinceMs, uint32_t& limit, std::string& err) {
    Document doc;
    if (!isObj(paramsJson, doc, err)) return false;
    sinceMs = (doc.HasMember("sinceMs") && doc["sinceMs"].IsUint64()) ? doc["sinceMs"].GetUint64() : 0;
    limit   = (doc.HasMember("limit")   && doc["limit"].IsUint())     ? doc["limit"].GetUint()     : 100;
    (void)err;
    return true;
}

// ============================================================
// 组装
// ============================================================
std::string makeAuthOk(const mgmt::IHostManagement::SessionInfo& session) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartObject();
    w.Key("kind");   w.String(K_AUTH_OK);
    w.Key("proto");  w.String(PROTO_VER);
    w.Key("supported"); w.StartArray(); w.String(PROTO_VER); w.EndArray();
    w.Key("session"); w.StartObject();
    w.Key("user"); w.String(session.user.c_str());
    w.Key("role"); w.String(session.role.c_str());
    w.EndObject();
    w.EndObject();
    return sb.GetString();
}

std::string makeAuthError(const std::string& code, const std::string& message) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartObject();
    w.Key("proto"); w.String(PROTO_VER);
    w.Key("kind");  w.String(K_ERROR);
    w.Key("error"); w.StartObject();
    w.Key("code");    w.String(code.c_str());
    w.Key("message"); w.String(message.c_str());
    w.EndObject();
    w.EndObject();
    return sb.GetString();
}

static void writeResponseHeader(Writer<StringBuffer>& w, const std::string& id, const std::string& status) {
    w.StartObject();
    w.Key("id");     w.String(id.c_str());
    w.Key("proto");  w.String(PROTO_VER);
    w.Key("kind");   w.String(K_RESPONSE);
    w.Key("status"); w.String(status.c_str());
}

std::string makeResponseOk(const std::string& id, const std::string& dataJson) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    writeResponseHeader(w, id, S_OK);
    w.Key("data"); w.RawValue(dataJson.c_str(), dataJson.size(), kObjectType);
    w.EndObject();
    return sb.GetString();
}

std::string makeResponseError(const std::string& id, const std::string& code, const std::string& message) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    writeResponseHeader(w, id, S_ERROR);
    w.Key("error"); w.StartObject();
    w.Key("code");    w.String(code.c_str());
    w.Key("message"); w.String(message.c_str());
    w.EndObject();
    w.EndObject();
    return sb.GetString();
}

std::string makeResponseAccepted(const std::string& id, const std::string& message) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    writeResponseHeader(w, id, S_ACCEPTED);
    w.Key("data"); w.StartObject();
    w.Key("message"); w.String(message.c_str());
    w.EndObject();
    w.EndObject();
    return sb.GetString();
}

std::string makeEvent(const std::string& eventType, const std::string& dataJson) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartObject();
    w.Key("proto"); w.String(PROTO_VER);
    w.Key("kind");  w.String(K_EVENT);
    w.Key("event"); w.String(eventType.c_str());
    w.Key("data");  w.RawValue(dataJson.c_str(), dataJson.size(), kObjectType);
    w.EndObject();
    return sb.GetString();
}

std::string makePong() {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartObject();
    w.Key("kind"); w.String(K_PONG);
    w.EndObject();
    return sb.GetString();
}

// ============================================================
// 结构体 → JSON 片段
// ============================================================
std::string toJson(const mgmt::HostStatus& s) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartObject();
    w.Key("state");    w.String(s.state.c_str());
    w.Key("uptimeMs"); w.Uint64(s.uptimeMs);
    w.Key("version");  w.String(s.version.c_str());
    w.Key("pid");      w.Int(s.pid);
    w.EndObject();
    return sb.GetString();
}

std::string toJson(const std::vector<mgmt::Metric>& ms) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartArray();
    for (const auto& m : ms) {
        w.StartObject();
        w.Key("name");  w.String(m.name.c_str());
        w.Key("value"); w.Double(m.value);
        w.Key("unit");  w.String(m.unit.c_str());
        w.EndObject();
    }
    w.EndArray();
    return sb.GetString();
}

std::string toJson(const std::vector<mgmt::ConfigItem>& cs) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartArray();
    for (const auto& c : cs) {
        w.StartObject();
        w.Key("key");      w.String(c.key.c_str());
        w.Key("value");    w.String(c.value.c_str());
        w.Key("readOnly"); w.Bool(c.readOnly);
        w.EndObject();
    }
    w.EndArray();
    return sb.GetString();
}

std::string toJson(const std::vector<mgmt::LogEntry>& ls) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartArray();
    for (const auto& l : ls) {
        w.StartObject();
        w.Key("timestampMs"); w.Uint64(l.timestampMs);
        w.Key("level");       w.String(l.level.c_str());
        w.Key("message");     w.String(l.message.c_str());
        w.EndObject();
    }
    w.EndArray();
    return sb.GetString();
}

std::string logEventPayload(const mgmt::LogEntry& l) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartObject();
    w.Key("timestampMs"); w.Uint64(l.timestampMs);
    w.Key("level");       w.String(l.level.c_str());
    w.Key("message");     w.String(l.message.c_str());
    w.EndObject();
    return sb.GetString();
}

std::string metricEventPayload(const mgmt::Metric& m) {
    StringBuffer sb; Writer<StringBuffer> w(sb);
    w.StartObject();
    w.Key("name");  w.String(m.name.c_str());
    w.Key("value"); w.Double(m.value);
    w.Key("unit");  w.String(m.unit.c_str());
    w.EndObject();
    return sb.GetString();
}

// ============================================================
// self-check
// ============================================================
int demo() {
    int fail = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("  FAIL line %d: %s\n", __LINE__, #cond); ++fail; } } while (0)

    // auth 往返
    {
        AuthMsg am; std::string e;
        CHECK(parseAuth("{\"kind\":\"auth\",\"token\":\"abc\",\"proto\":\"v1\"}", am, e));
        CHECK(am.token == "abc" && am.proto == "v1");
    }
    // request 往返（params 原样保留）
    {
        RequestMsg rm; std::string e;
        CHECK(parseRequest("{\"id\":\"1\",\"proto\":\"v1\",\"kind\":\"request\",\"action\":\"getStatus\",\"params\":{\"a\":1}}", rm, e));
        CHECK(rm.id == "1" && rm.action == Action::getStatus);
        CHECK(rm.paramsJson == "{\"a\":1}");
    }
    // setConfig / getLogs params
    {
        std::string k, v, e;
        CHECK(parseSetConfigParams("{\"key\":\"log_level\",\"value\":\"debug\"}", k, v, e));
        CHECK(k == "log_level" && v == "debug");
    }
    {
        uint64_t since; uint32_t lim; std::string e;
        CHECK(parseGetLogsParams("{\"sinceMs\":1000,\"limit\":50}", since, lim, e));
        CHECK(since == 1000 && lim == 50);
    }
    // subscribe
    {
        SubscribeMsg sm; std::string e;
        CHECK(parseSubscribe("{\"id\":\"sub-1\",\"kind\":\"subscribe\",\"events\":[\"log\",\"metric\"]}", sm, e));
        CHECK(sm.id == "sub-1" && sm.events.size() == 2);
    }
    // auth_ok 字段齐全
    {
        mgmt::IHostManagement::SessionInfo sess{"admin", "admin"};
        std::string s = makeAuthOk(sess);
        CHECK(s.find("\"kind\":\"auth_ok\"") != std::string::npos);
        CHECK(s.find("\"supported\"")       != std::string::npos);
        CHECK(s.find("\"user\":\"admin\"")  != std::string::npos);
    }
    // response ok 含状态数据
    {
        mgmt::HostStatus st{"running", 1234, "1.0.0", 42};
        std::string r = makeResponseOk("1", toJson(st));
        CHECK(r.find("\"status\":\"ok\"")       != std::string::npos);
        CHECK(r.find("\"state\":\"running\"")   != std::string::npos);
        CHECK(r.find("\"pid\":42")              != std::string::npos);
        CHECK(r.find("\"uptimeMs\":1234")       != std::string::npos);
    }
    // response error / accepted
    {
        CHECK(makeResponseError("2", E_PERM_DENIED, "x").find("\"code\":\"PERM_DENIED\"") != std::string::npos);
        CHECK(makeResponseAccepted("3", "restart scheduled").find("\"status\":\"accepted\"") != std::string::npos);
    }
    // event
    {
        mgmt::LogEntry le{1690000000000ULL, "warn", "hi"};
        std::string ev = makeEvent(EV_LOG, logEventPayload(le));
        CHECK(ev.find("\"kind\":\"event\"")  != std::string::npos);
        CHECK(ev.find("\"event\":\"log\"")   != std::string::npos);
        CHECK(ev.find("\"level\":\"warn\"")  != std::string::npos);
    }
    // pong + kind 解析
    {
        std::string pong = makePong();
        CHECK(pong == "{\"kind\":\"pong\"}");
        std::string k;
        CHECK(parseKind(pong, k) && k == "pong");
    }
#undef CHECK
    if (fail == 0) std::printf("JsonCodec demo: all checks passed\n");
    else           std::printf("JsonCodec demo: %d FAILED\n", fail);
    return fail ? 1 : 0;
}

} // namespace proto
