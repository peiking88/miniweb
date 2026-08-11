# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

嵌入式 Web 管理服务器：为宿主 C++ 后台应用提供 HTTPS + WSS 管理通道（运维、状态监控、配置维护）。宿主仅实现 `IHostManagement`，与 HTTP/WS/JSON 解耦。设计权威：`docs/技术方案.md`（评审定稿 2026-08-11）。

## 实现状态（2026-08-11）

**后端 + 前端 + 测试 + 文档全部完成。**
- HTTPS（静态托管 + health + login）+ WSS（auth/路由/心跳/订阅/限速/事件推送/优雅关闭）+ 内存态示例宿主 `ExampleHost`。
- **前端**：纯原生 ES5 三文件（`web/index.html`+`app.js`+`style.css`），登录/状态/指标/配置/日志流/操作，含重连/心跳/超时/restart 处理。
- **测试**：C++ 单测（CTest 3/3）、WSS 端到端 `tests/ws_client.py`（全 action，19 项）、前端浏览器端到端 `tests/frontend_test.cjs`（Playwright）、覆盖率 91%。
- **文档**：README、docs/API.md、docs/技术方案.md。

**可选后续**：git 仓库初始化（含 tag/版本号/推送）。

**已实现的可选增强（M4）**：SIGHUP 证书热加载（`ManagementServer::reloadCerts`）、Origin 白名单可配置（env `MINIWEB_ALLOWED_ORIGINS` → `WebSocketHandler::setAllowedOrigins` 的 `set_validate_handler`）。覆盖率 92%。

## 构建与测试

```bash
cmake -B build -G Ninja && ninja -C build -j$(nproc)   # 编译 → build/miniweb
./scripts/gen-cert.sh                                    # 首次生成 certs/*.pem
./build/miniweb                                          # 启动 8443/8444，Ctrl-C 退出
python3 tests/ws_client.py                               # 端到端（需先启动 server）
```

**依赖**：cpp-httplib v0.15.3、websocketpp 0.8.2、asio asio-1-28-2、rapidjson（本机源树）、OpenSSL 3.x。前三个由 CMake `FetchContent` 拉取（走 ghfast 代理，直连也可）；rapidjson 因全局 git `insteadOf` 把该 URL 重写为本地不存在的裸仓，改用本机源树，用 `-DRAPIDJSON_INCLUDE_DIR=/path/to/rapidjson/include` 覆盖。C++11 直接编译通过（`std::auto_ptr` 在 C++11 仅 deprecated）。

## 核心架构（理解全项目的关键，散落在技术方案第 2/3 节）

- **双库双端口**（C++11 约束下的选择，代价已接受）：
  - `8443` HTTPS — **cpp-httplib**：托管 `web/` 静态资源 + `/api/health` + `POST /api/login`
  - `8444` WSS — **websocketpp**（standalone Asio + OpenSSL）：承载全部管理通信（JSON over WebSocket）
- **分层**：浏览器静态资源 → 服务器层（cpp-httplib + websocketpp）→ **桥接层**（JSON 协议 RapidJSON、鉴权、事件分发）→ **`IHostManagement` 抽象接口** → 宿主实现（示例：`ExampleHost`）。
- **松耦合边界**：宿主只实现 `IHostManagement`，不依赖 cpp-httplib/websocketpp/HTTP/JSON（`Event.payloadJson` 的序列化除外——JSON 是数据载体）。这是核心契约。

## 关键约束（实现已遵守）

1. **C++11**，Asio 用 standalone 版（`ASIO_STANDALONE`，非 boost）。
2. **线程契约**：`login()` 在线程 A（httplib 池）；其余接口在线程 B（Asio 单线程 io 循环），跨线程并发需加锁。**慢操作不得阻塞线程 B**（示例 `ExampleHost` 的接口都很快；真实宿主的 `runDiagnostics`/`restart` 须异步）。
3. **会话模型 = 连接级**：鉴权一次（"先连后验"），不逐条 `validateToken`；token 过期由桥接层关连接。
4. **先连后验**：token 只在加密 WS 帧内传输，不放 URL/Header。
5. **restart 流程责任在桥接层**（`WebSocketHandler`）：accepted 响应 → `restart scheduled` 事件 → 关连接 → 调 `host.restart()`。宿主只触发退出。
6. 前端纯原生 HTML/CSS/JS，零框架。

## 代码导航

- `IHostManagement.h` — 宿主接口（核心契约）。`ExampleHost.h` — 内存态实现，含每 2s 推 metric/log 事件的事件线程。
- `JsonCodec.cpp`/`Protocol.h` — WSS JSON 协议解析与组装（RapidJSON），`proto::demo()` 自检。
- `HttpHandler.cpp` — cpp-httplib SSL，静态 + health + login + login 限速（`RateLimiter.h`）。
- `WebSocketHandler.cpp` — websocketpp `asio_tls`，pImpl 隔离；on_message 按 `kind` 路由，连接级会话表，`publish()` post 到 io 线程按订阅广播。
- `ManagementServer.cpp` — 聚合双通道，注册事件监听，幂等 `stop()`。

## 账号

`admin/admin`（admin 角色，全权限）、`viewer/viewer`（viewer 角色）。
