# miniweb — 嵌入式 Web 管理服务器

为 C++ 宿主应用提供 HTTPS + WSS 管理通道的嵌入式 Web 管理服务器。宿主仅实现 `IHostManagement` 接口，与 HTTP / WebSocket / JSON 彻底解耦，可独立演进。

- 完整设计：[docs/技术方案.md](docs/技术方案.md)（评审定稿）
- 接口与协议契约：[docs/API.md](docs/API.md)
- 开发指引：[CLAUDE.md](CLAUDE.md)

## 功能

- **HTTPS（:8443）** — cpp-httplib：静态资源托管、健康检查、登录认证
- **WSS（:8444）** — websocketpp：实时双向管理通信（状态/指标/配置/日志查询、重启/重载/诊断调用、事件推送）
- **松耦合** — 宿主实现 `IHostManagement` 抽象接口，不依赖任何 Web 库
- **安全** — 全链路 TLS、token 会话鉴权（先连后验）、高危操作速率限制

## 架构

```
浏览器 ──HTTPS:8443──▶ cpp-httplib（静态 / health / login）
       └──WSS:8444───▶ websocketpp（管理通信）
                              │
                      桥接层（RapidJSON 协议 / 鉴权 / 事件分发）
                              │  IHostManagement*
                      ┌───────▼────────┐
                      │ 宿主实现        │ （示例：ExampleHost）
                      └────────────────┘
```

## 依赖

C++11。三方库 header-only（CMake `FetchContent` 自动拉取）：cpp-httplib v0.15.3、websocketpp 0.8.2、standalone asio asio-1-28-2、RapidJSON、OpenSSL 3.x（系统库）。

## 构建

```bash
cmake -B build -G Ninja && ninja -C build -j$(nproc)   # 生成 build/miniweb
./scripts/gen-cert.sh                                    # 首次生成自签证书 certs/*.pem
```

> **RapidJSON**：本机因全局 git `insteadOf` 把 `github.com/Tencent/rapidjson` 重写为本地不存在的裸仓，默认复用本机源树 `/home/li/peiking88/tdx-cpp/external/rapidjson/include`。其它机器用 `-DRAPIDJSON_INCLUDE_DIR=/path/to/rapidjson/include` 指定。

## 运行

```bash
./build/miniweb          # 启动，Ctrl-C 优雅关闭
```

快速验证：
```bash
curl -k https://localhost:8443/api/health
curl -k -X POST https://localhost:8443/api/login \
     -H 'Content-Type: application/json' \
     -d '{"user":"admin","password":"admin"}'
python3 tests/ws_client.py    # 端到端（需先启动 server；pip 装 websockets）
```

## 测试

```bash
(cd build && ctest --output-on-failure)   # C++ 单测：JsonCodec/Protocol/RateLimiter/ExampleHost
python3 tests/ws_client.py                 # WSS 端到端：全 action + 错误路径 + restart
```

**运维 / 安全测试**：
```bash
./tests/sighup_test.sh                                              # SIGHUP 证书热重载
MINIWEB_ALLOWED_ORIGINS=https://good.com ./build/miniweb &          # 起带白名单的 server
python3 tests/origin_test.py                                        # Origin 白名单（bad 拒绝 / good 放行）
```

**前端浏览器端到端**（可选，需 Node + Playwright）：
```bash
mkdir -p /tmp/fe-test && cd /tmp/fe-test && npm init -y
npm install playwright --registry=https://registry.npmmirror.com
PLAYWRIGHT_DOWNLOAD_HOST=https://cdn.npmmirror.com/binaries/playwright npx playwright install chromium
cd /home/li/peiking88/miniweb && ./build/miniweb &   # 先启动 server
cd /tmp/fe-test && cp /home/li/peiking88/miniweb/tests/frontend_test.cjs . && node frontend_test.cjs
```

**覆盖率**（当前 91%，排除三方库）：
```bash
cmake -B build-cov -G Ninja -DMINIWEB_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
ninja -C build-cov -j$(nproc) && (cd build-cov && ctest)
./build-cov/miniweb & sleep 1; python3 tests/ws_client.py; kill %1
python3 -m venv /tmp/gcovr-env && /tmp/gcovr-env/bin/pip install -q gcovr \
    -i https://pypi.tuna.tsinghua.edu.cn/simple
/tmp/gcovr-env/bin/gcovr --root . --filter 'src/' --filter 'include/' --txt-metric line
```

## 配置

| 项 | 默认 | 说明 |
|----|------|------|
| 绑定地址 | `127.0.0.1` | env `MINIWEB_BIND_ADDR`；默认仅 loopback，生产可设 `0.0.0.0`（配合防火墙 + Origin 白名单） |
| HTTPS 端口 | 8443 | 静态 + `/api/health` + `/api/login` |
| WSS 端口 | 8444 | 全部管理通信 |
| 证书 | `certs/cert.pem`、`certs/key.pem` | `scripts/gen-cert.sh` 生成（自签，开发用） |
| 账号 | `admin` / `viewer` | env `MINIWEB_ADMIN_PASS`、`MINIWEB_VIEWER_PASS`（默认 `admin` / `viewer`） |
| Origin 白名单 | 空 = 本地策略 | env `MINIWEB_ALLOWED_ORIGINS`（逗号分隔）；空 = 仅放行 localhost/127.0.0.1，非空 = 严格匹配（大小写不敏感），空 Origin 放行 |
| 证书热重载 | — | `kill -HUP <pid>`（5s 冷却）；HTTPS 重建，WSS 新连接用新证书 |

端口与账号当前硬编码于 `src/main.cpp` 与 `src/ExampleHost.h`；真实宿主自行实现 `IHostManagement` 决定账号逻辑。

## 协议速查

WSS 消息为 JSON 文本帧：

```
鉴权：{"kind":"auth","token":"...","proto":"v1"} → {"kind":"auth_ok","session":{...}}
请求：{"id":"1","kind":"request","action":"getStatus","params":{}} → {"id":"1","kind":"response","status":"ok","data":{...}}
事件：{"kind":"event","event":"metric","data":{...}}    （服务器推送）
心跳：{"kind":"ping"} → {"kind":"pong"}
```

action：`getStatus` / `getMetrics` / `getConfig` / `setConfig` / `getLogs` / `restart` / `reloadConfig` / `runDiagnostics`。详见 [docs/API.md](docs/API.md)。

## 现状

后端 + 前端 + 测试 + 文档均已完成并通过验证：C++ 单测（CTest）、WSS 端到端（全 action）、**前端浏览器端到端（Playwright）**、覆盖率 91%。证书热加载、Origin 严格校验为可选后续工作。

浏览器打开 `https://localhost:8443`，用 `admin/admin` 登录即可使用管理控制台（状态/指标/配置/日志流/操作）。
