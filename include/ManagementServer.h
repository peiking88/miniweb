// ManagementServer.h
//
// 聚合 HTTPS + WSS 双通道，向宿主注册事件监听，统一 start/stop。
// 线程 A：cpp-httplib（HTTPS）；线程 B：asio（WSS）。详见方案第 3 节。
#pragma once
#include "IHostManagement.h"

#include <memory>
#include <string>
#include <vector>

class HttpHandler;
class WebSocketHandler;

class ManagementServer {
public:
    ManagementServer(mgmt::IHostManagement& host, const std::string& webDir);
    ~ManagementServer();

    // 启动双通道并注册事件监听。bindAddr 默认 loopback。任一通道失败返回 false。
    bool start(int httpsPort, int wssPort,
               const std::string& certPath, const std::string& keyPath,
               const std::string& bindAddr = "127.0.0.1");
    void stop();

    // SIGHUP 触发：HTTPS 重建用新证书；WSS 新连接由 tls_init_handler 读新证书。
    bool reloadCerts();
    // Origin 白名单透传给 WSS（须在 start 前调；start 时也会应用）。
    void setAllowedOrigins(const std::vector<std::string>& origins);

private:
    mgmt::IHostManagement& host_;
    std::string webDir_;
    std::unique_ptr<HttpHandler> http_;
    std::unique_ptr<WebSocketHandler> ws_;
    std::string certPath_, keyPath_, bindAddr_;
    std::vector<std::string> allowedOrigins_;
    int httpsPort_ = 0;
};
