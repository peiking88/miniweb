# miniweb API 文档

宿主与管理服务器之间的接口契约、HTTPS 端点与 WSS 协议定义。权威设计见 [技术方案.md](技术方案.md) 第 4、5 节；本文档与当前实现一致。

---

## 1. IHostManagement 接口（宿主实现）

宿主实现此抽象接口（[include/IHostManagement.h](../include/IHostManagement.h)），管理服务器反向调用。宿主不依赖任何 Web 库。

### 1.1 数据接口（WSS 通道）

| 方法 | 入参 | 出参 | 说明 |
|------|------|------|------|
| `getStatus()` | — | `HostStatus{state,uptimeMs,version,pid}` | 运行状态 |
| `getMetrics()` | — | `vector<Metric>{name,value,unit}` | 关键指标 |
| `getConfig()` | — | `vector<ConfigItem>{key,value,readOnly}` | 当前配置项 |
| `setConfig(key,value,&err)` | key, value | `bool` | 修改可写配置项，宿主负责持久化 |
| `getLogs(sinceMs,limit)` | 时间戳, 上限 | `vector<LogEntry>{timestampMs,level,message}` | 增量拉取日志 |

### 1.2 调用接口（WSS 通道）

| 方法 | 出参 | 说明 |
|------|------|------|
| `restart(&err)` | `bool` | **宿主只触发进程退出**，不推送事件、不发响应。完整流程由桥接层负责（见 §3.7） |
| `reloadConfig(&err)` | `bool` | 重新加载配置 |
| `runDiagnostics(&resultJson,&err)` | `bool` | 执行诊断，返回结果 JSON |

### 1.3 认证 / 事件（跨通道）

| 方法 | 通道 | 说明 |
|------|------|------|
| `login(user,password,&token,&err)` | HTTPS | 校验账号密码，颁发会话令牌 |
| `validateToken(token,&session,&err)` | WSS（鉴权阶段） | 校验 token，返回 `SessionInfo{user,role}` |
| `setEventListener(cb)` | 宿主→桥接 | 注册事件回调，宿主产生事件时调用 |

### 1.4 线程契约（实现必须遵守）

- `login()` 在 **线程 A**（cpp-httplib 线程池，多线程）执行。
- 其余接口在 **线程 B**（Asio 单线程事件循环）执行。
- **跨线程并发存在**（`login` 与 WSS 接口可能同时进入），共享状态必须加锁。
- **慢操作不得阻塞线程 B**：`runDiagnostics`、`restart` 等须投递到独立工作线程异步执行，否则卡死心跳与全部 WS I/O。

---

## 2. HTTPS 端点（:8443）

短连接、无状态、可缓存。

| 路径 | 方法 | 作用 |
|------|------|------|
| `/`、静态资源 | GET | 托管 `web/` 目录 |
| `/api/health` | GET | 健康检查 → `{"status":"ok"}` |
| `/api/login` | POST | 登录认证 |

### POST /api/login

请求：
```json
{ "user": "admin", "password": "admin" }
```
成功（200）：
```json
{ "token": "754620b7fa685acec6aa7de56c858dda" }
```
失败：`400`（非 JSON）/ `401`（凭证错误）/ `429`（速率限制：5 次 / 30s）。

---

## 3. WSS 协议（:8444）

承载全部管理通信。消息为 JSON 文本帧。协议版本字段 `proto`（当前 `v1`）。

### 3.1 鉴权（先连后验）

连接建立后**首条消息**：
```json
{ "kind": "auth", "token": "...", "proto": "v1" }
```
成功 →
```json
{ "kind": "auth_ok", "proto": "v1", "supported": ["v1"], "session": { "user": "admin", "role": "admin" } }
```
失败 →
```json
{ "proto": "v1", "kind": "error", "error": { "code": "PERM_DENIED", "message": "..." } }
```
随后连接被关闭。

> **先连后验**：浏览器 WebSocket API 不支持自定义 Header，token 若放 query 会泄露到日志。故 token 仅在加密 WS 帧内传输。会话为**连接级**，鉴权一次后复用，不逐条请求校验。

### 3.2 请求 / 响应

请求（Web → 宿主）：
```json
{ "id": "1", "kind": "request", "action": "getStatus", "params": {} }
```
响应（宿主 → Web），`id` 配对：
```json
{ "id": "1", "kind": "response", "status": "ok", "data": { ... } }
```
错误：
```json
{ "id": "1", "kind": "response", "status": "error", "error": { "code": "...", "message": "..." } }
```
未鉴权的请求返回 `PERM_DENIED`。

### 3.3 事件推送（宿主 → Web）

```json
{ "proto": "v1", "kind": "event", "event": "metric", "data": { "name": "cpu", "value": 42.5, "unit": "%" } }
```
事件类型：`log` / `metric` / `state` / `alert`。仅推送给已订阅该类型的已鉴权连接。

### 3.4 心跳

```json
{ "kind": "ping" } → { "kind": "pong" }
```

### 3.5 订阅

连接建立后默认不订阅任何事件，须显式订阅：
```json
{ "id": "sub-1", "kind": "subscribe", "events": ["log", "metric"] }
{ "id": "sub-1", "kind": "unsubscribe", "events": ["metric"] }
```

### 3.6 版本协商

客户端在 `auth.proto` 声明期望版本，服务端在 `auth_ok.proto` 返回选定版本，`auth_ok.supported` 给出支持列表。

### 3.7 restart 流程（责任在桥接层）

```
1. 桥接层收到 restart 请求
2. 回 {"status":"accepted","data":{"message":"restart scheduled, connection will close"}}
3. 推送 {"kind":"event","event":"state","data":{"message":"restart scheduled"}}
4. 关闭该 WS 连接
5. 调用 IHostManagement::restart()，宿主触发进程退出
```
宿主**不**在 `restart()` 中推送事件或发送响应。

### 3.8 action 路由表

| action | 接口 | 类型 | 备注 |
|--------|------|------|------|
| getStatus | getStatus | 数据 | |
| getMetrics | getMetrics | 数据 | |
| getConfig | getConfig | 数据 | |
| setConfig | setConfig | 数据 | params: `{key,value}`；限速 10/s |
| getLogs | getLogs | 数据 | params: `{sinceMs,limit}` |
| restart | restart | 调用 | 限速 1/60s；走 §3.7 流程 |
| reloadConfig | reloadConfig | 调用 | |
| runDiagnostics | runDiagnostics | 调用 | data 为宿主返回的 resultJson |

---

## 4. 错误码

| code | 含义 |
|------|------|
| `PERM_DENIED` | 未鉴权 / token 无效 / 权限不足 |
| `BAD_REQUEST` | 协议格式错误、缺字段、未知 action |
| `INTERNAL` | 宿主接口返回失败（setConfig/reload 等） |
| `NOT_FOUND` | 未知 action |
| `RATE_LIMIT` | 触发速率限制 |

---

## 5. 安全

- **传输**：全链路 TLS（HTTPS + WSS），自签证书开发用，生产用正式证书。
- **证书热加载**：`kill -HUP <pid>` 触发——HTTPS 重建用新证书，WSS 新连接由 `tls_init_handler` 读新证书（旧连接不变），无需重启进程。
- **会话**：token 为 ≥128bit 随机串，默认有效期 24h，连接级、过期由桥接层关连接。
- **速率限制**：login 5/30s、restart 1/60s、setConfig 10/s（桥接层集中、线程安全）。
- **来源校验**：环境变量 `MINIWEB_ALLOWED_ORIGINS`（逗号分隔）配置 Origin 白名单防 CSWSH。默认空=不校验；非空=只允许列表内 Origin，空 Origin（非浏览器客户端）放行。
