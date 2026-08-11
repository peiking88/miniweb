// HttpHandler.cpp
#include "HttpHandler.h"

#include <rapidjson/document.h>

#include <cstdio>

HttpHandler::HttpHandler(mgmt::IHostManagement& host, const std::string& webDir)
    : host_(host), webDir_(webDir), loginLimiter_(5, 30000) {}

HttpHandler::~HttpHandler() { stop(); }

bool HttpHandler::start(const std::string& certPath, const std::string& keyPath, int port) {
    svr_.reset(new httplib::SSLServer(certPath.c_str(), keyPath.c_str()));
    if (!svr_ || !svr_->is_valid()) {
        std::printf("[HTTPS] cert/binding failed (%s / %s)\n", certPath.c_str(), keyPath.c_str());
        return false;
    }
    setupRoutes();
    thread_ = std::thread([this, port] { svr_->listen("0.0.0.0", port); });
    // 等 listen 进入运行态（轮询 is_running）
    for (int i = 0; i < 50 && !svr_->is_running(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::printf("[HTTPS] listening on :%d (static + /api/health + /api/login)\n", port);
    return true;
}

void HttpHandler::stop() {
    if (svr_) svr_->stop();
    if (thread_.joinable()) thread_.join();
    svr_.reset();
}

void HttpHandler::setupRoutes() {
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
        // token 为 hex，无特殊字符，直接拼接
        res.set_content(std::string("{\"token\":\"") + token + "\"}", "application/json");
    } else {
        res.status = 401;
        res.set_content("{\"error\":\"invalid credentials\"}", "application/json");
    }
}
