// WebSocketHandler.cpp
//
// websocketpp WSS 服务器 + 协议路由。所有 WS 回调在 io 线程（线程B）执行；
// publish() 从宿主线程投递 post 到 io 线程，单线程访问连接表（仍加 mutex 防御）。
#include "WebSocketHandler.h"

#include "JsonCodec.h"
#include "Protocol.h"
#include "RateLimiter.h"

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include <asio/ssl.hpp>

#include <algorithm>
#include <cctype>
#include <future>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
#include <cstdio>

struct WebSocketHandler::Impl {
    typedef websocketpp::server<websocketpp::config::asio_tls> WsServer;

    mgmt::IHostManagement& host_;
    WsServer ws_;
    std::mutex mu_;

    struct Ctx {
        bool authed = false;
        mgmt::IHostManagement::SessionInfo session;
        std::set<std::string> subs;
    };
    std::map<websocketpp::connection_hdl, Ctx, std::owner_less<websocketpp::connection_hdl>> conns_;

    RateLimiter restartLimiter_;
    RateLimiter setConfigLimiter_;
    std::string certPath_, keyPath_;
    std::vector<std::string> allowedOrigins_;
    std::thread ioThread_;
    bool asioReady_ = false;   // start 未成功/未调用时 stop 不访问 io_service

    Impl(mgmt::IHostManagement& h)
        : host_(h), restartLimiter_(1, 60000), setConfigLimiter_(10, 1000) {
        // 默认不强制 Origin 白名单（便于端到端测试）；生产应通过配置启用（方案 7.6）。
        ws_.clear_access_channels(websocketpp::log::alevel::all);
    }

    // ---- 发送 / 关闭（须在 io 线程调用）----
    void sendStr(websocketpp::connection_hdl hdl, const std::string& s) {
        websocketpp::lib::error_code ec;
        ws_.send(hdl, s, websocketpp::frame::opcode::text, ec);
        if (ec) std::printf("[WSS] send failed: %s\n", ec.message().c_str());
    }
    void closeHdl(websocketpp::connection_hdl hdl) {
        websocketpp::lib::error_code ec;
        ws_.close(hdl, websocketpp::close::status::normal, "", ec);
        if (ec) std::printf("[WSS] close failed: %s\n", ec.message().c_str());
    }

    // ---- 回调 ----
    void onOpen(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lk(mu_);
        conns_[hdl] = Ctx();
    }
    void onClose(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lk(mu_);
        conns_.erase(hdl);
    }
    void onMessage(websocketpp::connection_hdl hdl, WsServer::message_ptr msg) {
        const std::string& payload = msg->get_payload();
        std::string kind;
        if (!proto::parseKind(payload, kind)) {
            sendStr(hdl, proto::makeAuthError(proto::E_BAD_REQUEST, "invalid json"));
            return;
        }
        if      (kind == proto::K_AUTH)        handleAuth(hdl, payload);
        else if (kind == proto::K_REQUEST)     handleRequest(hdl, payload);
        else if (kind == proto::K_PING)        sendStr(hdl, proto::makePong());
        else if (kind == proto::K_SUBSCRIBE ||
                 kind == proto::K_UNSUBSCRIBE) handleSubscribe(hdl, payload, kind);
        else sendStr(hdl, proto::makeResponseError("", proto::E_BAD_REQUEST, "unknown kind"));
    }

    // ---- 鉴权（先连后验）----
    void handleAuth(websocketpp::connection_hdl hdl, const std::string& payload) {
        proto::AuthMsg am; std::string err;
        if (!proto::parseAuth(payload, am, err)) {
            sendStr(hdl, proto::makeAuthError(proto::E_BAD_REQUEST, err));
            closeHdl(hdl); return;
        }
        mgmt::IHostManagement::SessionInfo sess;
        if (!host_.validateToken(am.token, sess, err)) {
            sendStr(hdl, proto::makeAuthError(proto::E_PERM_DENIED, err));
            closeHdl(hdl); return;
        }
        {
            std::lock_guard<std::mutex> lk(mu_);
            conns_[hdl].authed = true;
            conns_[hdl].session = sess;
        }
        sendStr(hdl, proto::makeAuthOk(sess));
    }

    bool isAuthed(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = conns_.find(hdl);
        return it != conns_.end() && it->second.authed;
    }

    // ---- 请求路由 ----
    void handleRequest(websocketpp::connection_hdl hdl, const std::string& payload) {
        proto::RequestMsg rm; std::string err;
        if (!proto::parseRequest(payload, rm, err)) {
            sendStr(hdl, proto::makeResponseError("", proto::E_BAD_REQUEST, err));
            return;
        }
        if (!isAuthed(hdl)) {
            sendStr(hdl, proto::makeResponseError(rm.id, proto::E_PERM_DENIED, "not authenticated"));
            return;
        }
        switch (rm.action) {
        case proto::Action::getStatus:
            sendStr(hdl, proto::makeResponseOk(rm.id, proto::toJson(host_.getStatus()))); break;
        case proto::Action::getMetrics:
            sendStr(hdl, proto::makeResponseOk(rm.id, proto::toJson(host_.getMetrics()))); break;
        case proto::Action::getConfig:
            sendStr(hdl, proto::makeResponseOk(rm.id, proto::toJson(host_.getConfig()))); break;
        case proto::Action::setConfig: {
            if (!setConfigLimiter_.allow()) {
                sendStr(hdl, proto::makeResponseError(rm.id, proto::E_RATE_LIMIT, "setConfig too frequent")); break;
            }
            std::string k, v;
            if (!proto::parseSetConfigParams(rm.paramsJson, k, v, err)) {
                sendStr(hdl, proto::makeResponseError(rm.id, proto::E_BAD_REQUEST, err)); break;
            }
            if (!host_.setConfig(k, v, err)) {
                sendStr(hdl, proto::makeResponseError(rm.id, proto::E_INTERNAL, err)); break;
            }
            sendStr(hdl, proto::makeResponseOk(rm.id, "{}")); break;
        }
        case proto::Action::getLogs: {
            uint64_t since = 0; uint32_t lim = 100;
            proto::parseGetLogsParams(rm.paramsJson, since, lim, err);
            sendStr(hdl, proto::makeResponseOk(rm.id, proto::toJson(host_.getLogs(since, lim)))); break;
        }
        case proto::Action::reloadConfig:
            sendStr(hdl, host_.reloadConfig(err)
                        ? proto::makeResponseOk(rm.id, "{}")
                        : proto::makeResponseError(rm.id, proto::E_INTERNAL, err));
            break;
        case proto::Action::runDiagnostics: {
            std::string resultJson;
            sendStr(hdl, host_.runDiagnostics(resultJson, err)
                        ? proto::makeResponseOk(rm.id, resultJson)
                        : proto::makeResponseError(rm.id, proto::E_INTERNAL, err));
            break;
        }
        case proto::Action::restart: {
            if (!restartLimiter_.allow()) {
                sendStr(hdl, proto::makeResponseError(rm.id, proto::E_RATE_LIMIT, "restart too frequent")); break;
            }
            // 方案 4.3：accepted → restart scheduled 事件 → 关连接 → 调 restart
            sendStr(hdl, proto::makeResponseAccepted(rm.id, "restart scheduled, connection will close"));
            sendStr(hdl, proto::makeEvent(proto::EV_STATE, "{\"message\":\"restart scheduled\"}"));
            closeHdl(hdl);
            host_.restart(err);   // 示例宿主不真退出；真实宿主在此触发进程退出
            break;
        }
        default:
            sendStr(hdl, proto::makeResponseError(rm.id, proto::E_NOT_FOUND, "unknown action"));
        }
    }

    void handleSubscribe(websocketpp::connection_hdl hdl, const std::string& payload, const std::string& kind) {
        proto::SubscribeMsg sm; std::string err;
        if (!proto::parseSubscribe(payload, sm, err)) {
            sendStr(hdl, proto::makeResponseError(sm.id, proto::E_BAD_REQUEST, err)); return;
        }
        if (!isAuthed(hdl)) {   // P2: 与 handleRequest 一致，订阅要求鉴权
            sendStr(hdl, proto::makeResponseError(sm.id, proto::E_PERM_DENIED, "not authenticated")); return;
        }
        {
            std::lock_guard<std::mutex> lk(mu_);
            auto& c = conns_[hdl];
            for (const auto& ev : sm.events) {
                if (kind == proto::K_SUBSCRIBE) c.subs.insert(ev);
                else                            c.subs.erase(ev);
            }
        }
        sendStr(hdl, proto::makeResponseOk(sm.id, "{}"));
    }

    // ---- 事件广播（post 到 io 线程执行）----
    void publish(const mgmt::Event& e) {
        ws_.get_io_service().post([this, e] {
            const std::string msg = proto::makeEvent(e.type, e.payloadJson);
            std::vector<websocketpp::connection_hdl> targets;
            {
                std::lock_guard<std::mutex> lk(mu_);
                for (const auto& kv : conns_) {
                    if (kv.second.authed && kv.second.subs.count(e.type))
                        targets.push_back(kv.first);
                }
            }
            for (const auto& hdl : targets) {
                websocketpp::lib::error_code ec;
                ws_.send(hdl, msg, websocketpp::frame::opcode::text, ec);
            }
        });
    }

    bool start(const std::string& cert, const std::string& key, int port, const std::string& bindAddr) {
        certPath_ = cert; keyPath_ = key;
        try {
            ws_.init_asio();
            asioReady_ = true;
            ws_.set_reuse_addr(true);
            ws_.set_max_message_size(64 * 1024);   // P2: WS 消息上限，防放大 DoS
            ws_.set_open_handler([this](websocketpp::connection_hdl h) { onOpen(h); });
            ws_.set_close_handler([this](websocketpp::connection_hdl h) { onClose(h); });
            ws_.set_message_handler([this](websocketpp::connection_hdl h, WsServer::message_ptr m) { onMessage(h, m); });
            ws_.set_validate_handler([this](websocketpp::connection_hdl h) -> bool {
                websocketpp::lib::error_code ec;
                auto con = ws_.get_con_from_hdl(h, ec);
                if (ec) return false;
                std::string origin = con->get_origin();
                if (origin.empty()) return true;                  // 非浏览器客户端放行
                // P2: scheme/host_ 小写化比较（RFC 6454）
                std::string lo = origin;
                std::transform(lo.begin(), lo.end(), lo.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                std::vector<std::string> origins;
                { std::lock_guard<std::mutex> lk(mu_); origins = allowedOrigins_; }   // P2: 读加锁
                if (!origins.empty()) {
                    for (const auto& a : origins) {
                        std::string la = a;
                        std::transform(la.begin(), la.end(), la.begin(),
                                       [](unsigned char c) { return std::tolower(c); });
                        if (la == lo) return true;
                    }
                    return false;                                 // 配置了白名单 → 严格匹配
                }
                // 默认本地策略——仅放行 localhost / 127.0.0.1 来源
                return lo.find("localhost") != std::string::npos
                    || lo.find("127.0.0.1") != std::string::npos;
            });
            ws_.set_tls_init_handler([this](websocketpp::connection_hdl) -> websocketpp::lib::shared_ptr<asio::ssl::context> {
                auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);
                // P2: TLS 加固——禁旧协议/压缩、单 DH；限定强 cipher
                ctx->set_options(asio::ssl::context::default_workarounds
                               | asio::ssl::context::no_sslv2
                               | asio::ssl::context::no_sslv3
                               | asio::ssl::context::no_tlsv1
                               | asio::ssl::context::no_compression
                               | asio::ssl::context::single_dh_use);
                SSL_CTX_set_cipher_list(ctx->native_handle(),
                    "ECDHE+AESGCM:ECDHE+CHACHA20:!aNULL:!MD5:!DSS:!3DES:!RC4");
                ctx->use_certificate_chain_file(certPath_);
                ctx->use_private_key_file(keyPath_, asio::ssl::context::pem);
                return ctx;
            });
            ws_.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string(bindAddr), port));
            ws_.start_accept();
        } catch (const std::exception& e) {
            std::printf("[WSS] start failed: %s\n", e.what());
            return false;
        }
        ioThread_ = std::thread([this] { ws_.run(); });
        std::printf("[WSS] listening on %s:%d\n", bindAddr.c_str(), port);
        return true;
    }

    void stop() {
        if (!asioReady_) return;   // start 未成功/未调用 → 不访问 io_service
        // P1: 整个关闭流程 post 到 io 线程（避免与 acceptor 跨线程争用）；
        //     ws_.stop() 在 lambda 末尾于 io 线程内调用，run 完成 lambda 后退出，外层 join。
        //     不用 future——避免 run 已自然退出时 post 不派发导致 fut.wait 永久阻塞。
        ws_.get_io_service().post([this] {
            websocketpp::lib::error_code ec;
            ws_.stop_listening(ec);
            std::lock_guard<std::mutex> lk(mu_);
            const std::string shutdownMsg =
                proto::makeEvent(proto::EV_ALERT, "{\"message\":\"server shutting down\"}");
            for (const auto& kv : conns_) {
                websocketpp::lib::error_code e;
                ws_.send(kv.first, shutdownMsg, websocketpp::frame::opcode::text, e);
                ws_.close(kv.first, websocketpp::close::status::going_away, "shutdown", e);
            }
            conns_.clear();
            ws_.stop();   // io 线程内停止 io_service，run 退出
        });
        if (ioThread_.joinable()) ioThread_.join();
    }
};

// ============================================================
// WebSocketHandler 转发
// ============================================================
WebSocketHandler::WebSocketHandler(mgmt::IHostManagement& host_)
    : impl_(new Impl(host_)) {}
WebSocketHandler::~WebSocketHandler() { stop(); }
bool WebSocketHandler::start(const std::string& certPath_, const std::string& keyPath_, int port,
                             const std::string& bindAddr) {
    return impl_->start(certPath_, keyPath_, port, bindAddr);
}
void WebSocketHandler::stop() { if (impl_) impl_->stop(); }
void WebSocketHandler::publish(const mgmt::Event& e) { impl_->publish(e); }
void WebSocketHandler::setAllowedOrigins(const std::vector<std::string>& origins) {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    impl_->allowedOrigins_ = origins;
}
