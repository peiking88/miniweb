// tests/test_server_failures.cpp — dependency-failure：证书/绑定失败路径
#include "test_common.h"
#include "HttpHandler.h"
#include "ExampleHost.h"

// 证书路径不存在 → SSLServer 构造加载失败 → start 返回 false
static void test_http_bad_cert() {
    ExampleHost host;
    HttpHandler http(host, "web");
    CHECK(!http.start("/nonexistent/cert.pem", "/nonexistent/key.pem", 18443));
}

// 证书与密钥不匹配 → 加载/校验失败 → start 返回 false
static void test_http_mismatched_cert() {
    ExampleHost host;
    HttpHandler http(host, "web");
    // 用真实证书但假的 key 路径
    CHECK(!http.start("certs/cert.pem", "/nonexistent/key.pem", 18444));
}

int main() {
    test_http_bad_cert();
    test_http_mismatched_cert();
    TEST_SUMMARY("test_server_failures");
}
