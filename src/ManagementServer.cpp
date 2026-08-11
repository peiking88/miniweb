// ManagementServer.cpp
#include "ManagementServer.h"

#include "HttpHandler.h"
#include "WebSocketHandler.h"

#include <cstdio>

ManagementServer::ManagementServer(mgmt::IHostManagement& host, const std::string& webDir)
    : host_(host), webDir_(webDir) {}

ManagementServer::~ManagementServer() { stop(); }

bool ManagementServer::start(int httpsPort, int wssPort,
                             const std::string& certPath, const std::string& keyPath) {
    certPath_ = certPath; keyPath_ = keyPath; httpsPort_ = httpsPort;
    http_.reset(new HttpHandler(host_, webDir_));
    if (!http_->start(certPath, keyPath, httpsPort)) return false;

    ws_.reset(new WebSocketHandler(host_));
    ws_->setAllowedOrigins(allowedOrigins_);
    if (!ws_->start(certPath, keyPath, wssPort)) {
        http_->stop();
        return false;
    }

    // 宿主事件 → 广播到已订阅的 WS 客户端
    host_.setEventListener([this](const mgmt::Event& e) {
        if (ws_) ws_->publish(e);
    });
    std::printf("[mgmt] management server up (https=%d, wss=%d)\n", httpsPort, wssPort);
    return true;
}

void ManagementServer::stop() {
    if (!http_ && !ws_) return;   // 已停（幂等，允许析构重复调用）
    // 先摘掉事件监听，避免 stop 期间再向已关闭的 ws 投递
    host_.setEventListener(nullptr);
    if (ws_) ws_->stop();
    if (http_) http_->stop();
    ws_.reset();
    http_.reset();
    std::printf("[mgmt] management server stopped\n");
}

bool ManagementServer::reloadCerts() {
    if (!http_ || httpsPort_ == 0) return false;
    std::printf("[mgmt] SIGHUP: reloading certs...\n");
    http_->stop();
    bool ok = http_->start(certPath_, keyPath_, httpsPort_);
    // websocketpp 的 tls_init_handler 每连接读文件，新连接自动用新证书；旧连接不变
    std::printf("[mgmt] certs reloaded (https=%s, wss new conns use new cert)\n", ok ? "ok" : "FAILED");
    return ok;
}

void ManagementServer::setAllowedOrigins(const std::vector<std::string>& origins) {
    allowedOrigins_ = origins;
    if (ws_) ws_->setAllowedOrigins(origins);
}
