// tests/test_json.cpp — JsonCodec / Protocol 单元测试
//
// make* 的输出用 RapidJSON 完整解析后断言字段（取代浅 find 子串，AP10），
// 并验证 make→parse 往返不变量（序列化-反序列化对称）。
#include "test_common.h"
#include "JsonCodec.h"
#include "Protocol.h"

#include <rapidjson/document.h>

#include <climits>
#include <string>

using namespace proto;
using rapidjson::Document;

// FindMember 实现，键缺失返回默认值（不触发 RAPIDJSON_ASSERT，Release 下安全）
template <typename T>
static const char* str(const T& v, const char* k) {
    auto it = v.FindMember(k);
    return (it != v.MemberEnd() && it->value.IsString()) ? it->value.GetString() : "";
}
template <typename T>
static int64_t num(const T& v, const char* k) {
    auto it = v.FindMember(k);
    return (it != v.MemberEnd() && it->value.IsInt64()) ? it->value.GetInt64() : INT64_MIN;
}
static Document parseDoc(const std::string& s) {
    Document d;
    d.Parse(s.c_str());
    return d;
}

static void test_action_mapping() {
    CHECK(parseAction("getStatus")      == Action::getStatus);
    CHECK(parseAction("getMetrics")     == Action::getMetrics);
    CHECK(parseAction("getConfig")      == Action::getConfig);
    CHECK(parseAction("setConfig")      == Action::setConfig);
    CHECK(parseAction("getLogs")        == Action::getLogs);
    CHECK(parseAction("restart")        == Action::restart);
    CHECK(parseAction("reloadConfig")   == Action::reloadConfig);
    CHECK(parseAction("runDiagnostics") == Action::runDiagnostics);
    CHECK(parseAction("nonsense")       == Action::unknown);
    CHECK(parseAction("")               == Action::unknown);
    // 往返：parseAction → actionName 应还原原串（除 unknown）
    CHECK(std::string(actionName(parseAction("getStatus")))      == "getStatus");
    CHECK(std::string(actionName(parseAction("runDiagnostics"))) == "runDiagnostics");
    CHECK(std::string(actionName(parseAction("setConfig")))      == "setConfig");
}

static void test_parse_kind() {
    std::string k;
    CHECK(parseKind("{\"kind\":\"ping\"}", k) && k == "ping");
    CHECK(!parseKind("not json", k));
    CHECK(!parseKind("[]", k));
    CHECK(!parseKind("{}", k));                 // 无 kind 字段
    CHECK(!parseKind("{\"kind\":123}", k));     // kind 非字符串
}

static void test_parse_auth() {
    AuthMsg a; std::string e;
    CHECK(parseAuth("{\"kind\":\"auth\",\"token\":\"abc\",\"proto\":\"v2\"}", a, e));
    CHECK(a.token == "abc" && a.proto == "v2");
    CHECK(parseAuth("{\"kind\":\"auth\",\"token\":\"t\"}", a, e) && a.proto == "v1");  // proto 缺省 v1
    CHECK(!parseAuth("{\"kind\":\"request\",\"token\":\"t\"}", a, e) && !e.empty());   // 错 kind + err 非空
    CHECK(!parseAuth("{\"kind\":\"auth\"}", a, e) && e.find("token") != std::string::npos);  // 缺 token
    CHECK(!parseAuth("xxx", a, e));                                                   // 非 json
}

static void test_parse_request() {
    RequestMsg r; std::string e;
    CHECK(parseRequest("{\"id\":\"7\",\"kind\":\"request\",\"action\":\"setConfig\",\"params\":{\"key\":\"a\",\"value\":\"b\"}}", r, e));
    CHECK(r.id == "7" && r.action == Action::setConfig);
    CHECK(r.paramsJson.find("\"key\":\"a\"") != std::string::npos);
    CHECK(parseRequest("{\"id\":\"1\",\"kind\":\"request\",\"action\":\"frob\"}", r, e) && r.action == Action::unknown);
    CHECK(parseRequest("{\"id\":\"1\",\"kind\":\"request\",\"action\":\"getStatus\"}", r, e) && r.paramsJson == "{}");
    CHECK(!parseRequest("{\"id\":\"1\",\"kind\":\"request\"}", r, e) && e.find("action") != std::string::npos);  // 缺 action + err
    CHECK(!parseRequest("{\"id\":\"1\",\"kind\":\"ping\"}", r, e));   // 错 kind
}

static void test_parse_subscribe() {
    SubscribeMsg s; std::string e;
    CHECK(parseSubscribe("{\"id\":\"s\",\"kind\":\"subscribe\",\"events\":[\"log\",\"metric\",\"alert\"]}", s, e));
    CHECK(s.id == "s" && s.events.size() == 3);
    CHECK(parseSubscribe("{\"kind\":\"unsubscribe\",\"events\":[\"log\"]}", s, e) && s.events.size() == 1);
    CHECK(!parseSubscribe("{\"kind\":\"subscribe\"}", s, e) && e.find("events") != std::string::npos);  // 缺 events + err
}

static void test_parse_params() {
    std::string k, v, e;
    CHECK(parseSetConfigParams("{\"key\":\"log_level\",\"value\":\"debug\"}", k, v, e));
    CHECK(k == "log_level" && v == "debug");
    CHECK(!parseSetConfigParams("{\"key\":\"log_level\"}", k, v, e) && e.find("value") != std::string::npos);

    uint64_t since; uint32_t lim;
    CHECK(parseGetLogsParams("{\"sinceMs\":1000,\"limit\":50}", since, lim, e) && since == 1000 && lim == 50);
    CHECK(parseGetLogsParams("{}", since, lim, e) && since == 0 && lim == 100);  // 默认值
}

// ---- make* 用完整 JSON 解析断言（取代浅 find 子串）----
static void test_make_auth_ok() {
    mgmt::IHostManagement::SessionInfo sess{"admin", "admin"};
    auto d = parseDoc(makeAuthOk(sess));
    CHECK(d.IsObject() && !d.HasParseError());
    CHECK(std::string(str(d, "kind")) == "auth_ok");
    CHECK(std::string(str(d, "proto")) == "v1");
    CHECK(d.HasMember("supported") && d["supported"].IsArray() && d["supported"].Size() >= 1);
    CHECK(d.HasMember("session") && d["session"].IsObject());
    CHECK(std::string(str(d["session"], "user")) == "admin");
    CHECK(std::string(str(d["session"], "role")) == "admin");

    auto e = parseDoc(makeAuthError(E_PERM_DENIED, "bad token"));
    CHECK(std::string(str(e, "kind")) == "error");
    CHECK(std::string(str(e["error"], "code")) == "PERM_DENIED");
    CHECK(std::string(str(e["error"], "message")) == "bad token");
}

static void test_make_responses() {
    mgmt::HostStatus st{"running", 1234, "1.0.0", 42};
    auto d = parseDoc(makeResponseOk("1", toJson(st)));
    CHECK(d.IsObject() && !d.HasParseError());
    CHECK(std::string(str(d, "id")) == "1");
    CHECK(std::string(str(d, "kind")) == "response");
    CHECK(std::string(str(d, "status")) == "ok");
    CHECK(std::string(str(d["data"], "state")) == "running");
    CHECK(num(d["data"], "uptimeMs") == 1234);
    CHECK(num(d["data"], "pid") == 42);

    auto er = parseDoc(makeResponseError("2", E_BAD_REQUEST, "x"));
    CHECK(std::string(str(er, "status")) == "error");
    CHECK(std::string(str(er["error"], "code")) == "BAD_REQUEST");

    auto ac = parseDoc(makeResponseAccepted("3", "restart scheduled"));
    CHECK(std::string(str(ac, "status")) == "accepted");
    CHECK(std::string(str(ac["data"], "message")) == "restart scheduled");
}

static void test_make_event_and_payloads() {
    mgmt::LogEntry le{1690000000000ULL, "warn", "disk nearly full"};
    auto e = parseDoc(makeEvent(EV_LOG, logEventPayload(le)));
    CHECK(std::string(str(e, "kind")) == "event");
    CHECK(std::string(str(e, "event")) == "log");
    CHECK(std::string(str(e["data"], "level")) == "warn");
    CHECK(std::string(str(e["data"], "message")) == "disk nearly full");
    CHECK(num(e["data"], "timestampMs") == 1690000000000LL);

    mgmt::Metric m{"cpu", 55.5, "%"};
    auto mp = parseDoc(metricEventPayload(m));
    CHECK(std::string(str(mp, "name")) == "cpu");

    auto ms = parseDoc(toJson(std::vector<mgmt::Metric>{{"cpu", 10, "%"}, {"mem", 20, "%"}}));
    CHECK(ms.IsArray() && ms.Size() == 2 && std::string(str(ms[0], "name")) == "cpu");

    auto cs = parseDoc(toJson(std::vector<mgmt::ConfigItem>{{"k", "v", true}}));
    CHECK(cs.IsArray() && cs[0]["readOnly"].GetBool() == true);

    auto ls = parseDoc(toJson(std::vector<mgmt::LogEntry>{{1, "info", "x"}}));
    CHECK(ls.IsArray() && num(ls[0], "timestampMs") == 1);

    CHECK(makePong() == "{\"kind\":\"pong\"}");
}

// ---- 往返不变量：make 产生的响应/事件能被 JSON 解析且字段完整对称 ----
static void test_round_trip() {
    mgmt::HostStatus st{"degraded", 9999, "2.0.0", 7};
    auto d = parseDoc(makeResponseOk("42", toJson(st)));
    CHECK(std::string(str(d, "id")) == "42");
    CHECK(std::string(str(d, "status")) == "ok");
    CHECK(std::string(str(d["data"], "state")) == "degraded");
    CHECK(num(d["data"], "uptimeMs") == 9999);
    CHECK(num(d["data"], "pid") == 7);
    CHECK(std::string(str(d["data"], "version")) == "2.0.0");

    mgmt::LogEntry le{1700000000000ULL, "error", "boom"};
    auto ev = parseDoc(makeEvent(EV_LOG, logEventPayload(le)));
    CHECK(std::string(str(ev, "kind")) == "event");
    CHECK(std::string(str(ev["data"], "level")) == "error");
    CHECK(num(ev["data"], "timestampMs") == 1700000000000LL);
}

int main() {
    test_action_mapping();
    test_parse_kind();
    test_parse_auth();
    test_parse_request();
    test_parse_subscribe();
    test_parse_params();
    test_make_auth_ok();
    test_make_responses();
    test_make_event_and_payloads();
    test_round_trip();
    TEST_SUMMARY("test_json");
}
