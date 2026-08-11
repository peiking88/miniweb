// WebSocketHandler.h
//
// WSS 通道（websocketpp + standalone asio + OpenSSL TLS）。
// 端口 8444。承载全部管理通信：鉴权、协议路由、心跳、订阅、事件广播。
// 用 pImpl 隔离 websocketpp，避免污染外部头文件。
#pragma once
#include "IHostManagement.h"

#include <memory>
#include <string>
#include <vector>

class WebSocketHandler {
public:
    WebSocketHandler(mgmt::IHostManagement& host);
    ~WebSocketHandler();

    // 加载证书并开始监听（内部 io 线程）。bindAddr 默认 loopback。返回 false 表示失败。
    bool start(const std::string& certPath, const std::string& keyPath, int port,
               const std::string& bindAddr = "127.0.0.1");
    void stop();

    // 设置 Origin 白名单（CSWSH 防护）。空=不校验（默认）；非空=只允许列表内 Origin，
    // 空 Origin（非浏览器客户端）始终放行。须在 start 前调用。
    void setAllowedOrigins(const std::vector<std::string>& origins);

    // 向已订阅该事件类型的已鉴权连接广播（线程安全，投递到 io 线程执行）。
    void publish(const mgmt::Event& e);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
