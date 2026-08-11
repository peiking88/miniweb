// HttpHandler.h
//
// HTTPS 通道（cpp-httplib SSLServer）：静态托管 + 健康检查 + 登录。
// 端口 8443。详见 docs/技术方案.md 第 3 节"双通道功能划分"。
#pragma once
#include "IHostManagement.h"
#include "RateLimiter.h"

#include <httplib.h>

#include <memory>
#include <string>
#include <thread>

class HttpHandler {
public:
    HttpHandler(mgmt::IHostManagement& host, const std::string& webDir);
    ~HttpHandler();
    // C.21：自定义析构，显式禁用拷贝/移动（含 unique_ptr + thread，移动语义不安全）
    HttpHandler(const HttpHandler&) = delete;
    HttpHandler& operator=(const HttpHandler&) = delete;
    HttpHandler(HttpHandler&&) = delete;
    HttpHandler& operator=(HttpHandler&&) = delete;

    // 加载证书并开始监听（内部线程）。bindAddr 默认 loopback。返回 false 表示失败。
    bool start(const std::string& certPath, const std::string& keyPath, int port,
               const std::string& bindAddr = "127.0.0.1");
    void stop();

private:
    void setupRoutes();
    void handleLogin(const httplib::Request& req, httplib::Response& res);

    mgmt::IHostManagement& host_;
    std::string webDir_;
    std::unique_ptr<httplib::SSLServer> svr_;
    RateLimiter loginLimiter_;          // 5 次 / 30s，防暴力破解（方案 7.5）
    std::thread thread_;
};
