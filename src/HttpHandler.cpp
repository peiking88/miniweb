// HttpHandler.cpp
#include "HttpHandler.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <chrono>
#include <cstdio>

HttpHandler::HttpHandler(mgmt::IHostManagement& host, const std::string& webDir)
    : host_(host), webDir_(webDir), loginLimiter_(5, 30000) {}

HttpHandler::~HttpHandler() { stop(); }

bool HttpHandler::start(const std::string& certPath, const std::string& keyPath, int port,
                        const std::string& bindAddr) {
    svr_.reset(new httplib::SSLServer(certPath.c_str(), keyPath.c_str()));
    if (!svr_ || !svr_->is_valid()) {
        std::printf("[HTTPS] cert/binding failed (%s / %s)\n", certPath.c_str(), keyPath.c_str());
        return false;
    }
    setupRoutes();
    thread_ = std::thread([this, port, bindAddr] { svr_->listen(bindAddr.c_str(), port); });
    // P1: 端口绑定失败时 is_running 恒 false，须终检（否则误报已启动）
    for (int i = 0; i < 50 && !svr_->is_running(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if (!svr_->is_running()) {
        std::printf("[HTTPS] failed to bind %s:%d\n", bindAddr.c_str(), port);
        if (thread_.joinable()) thread_.join();
        svr_.reset();
        return false;
    }
    std::printf("[HTTPS] listening on %s:%d (static + /api/health + /api/login)\n", bindAddr.c_str(), port);
    return true;
}

void HttpHandler::stop() {
    if (svr_) svr_->stop();
    if (thread_.joinable()) thread_.join();
    svr_.reset();
}

void HttpHandler::setupRoutes() {
    // P2: 安全响应头（全局，含 login 防缓存 token）
    svr_->set_default_headers({
        {"Cache-Control", "no-store"},
        {"X-Content-Type-Options", "nosniff"},
        {"X-Frame-Options", "DENY"},
        {"Referrer-Policy", "no-referrer"},
    });
    // P2: POST body 上限，防 DoS
    svr_->set_payload_max_length(64 * 1024);
    // 静态资源托管
    svr_->set_mount_point("/", webDir_);

    // 健康检查（无状态，外部探活）
    svr_->Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // 登录
    svr_->Post("/api/login", [this](const httplib::Request& req, httplib::Response& res) {
        handleLogin(req, res);
    });
}

void HttpHandler::handleLogin(const httplib::Request& req, httplib::Response& res) {
    if (!loginLimiter_.allow()) {
        res.status = 429;
        res.set_content("{\"error\":\"rate limit\"}", "application/json");
        return;
    }
    rapidjson::Document doc;
    if (doc.Parse(req.body.c_str()).HasParseError() || !doc.IsObject()) {
        res.status = 400;
        res.set_content("{\"error\":\"bad request\"}", "application/json");
        return;
    }
    std::string user = (doc.HasMember("user")     && doc["user"].IsString())     ? doc["user"].GetString()     : "";
    std::string pass = (doc.HasMember("password") && doc["password"].IsString()) ? doc["password"].GetString() : "";

    std::string token, err;
    if (host_.login(user, pass, token, err)) {
        // P3: 用 RapidJSON Writer 生成，避免接口约束放宽后的 JSON 注入
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> w(sb);
        w.StartObject(); w.Key("token"); w.String(token.c_str()); w.EndObject();
        res.set_content(sb.GetString(), "application/json");
    } else {
        res.status = 401;
        res.set_content("{\"error\":\"invalid credentials\"}", "application/json");
    }
}
