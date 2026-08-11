#!/usr/bin/env python3
# WSS 端到端验证（全 action 覆盖）：login → auth → 全部数据/调用接口 → subscribe/event → ping
#   + 错误路径（未鉴权/错误 token/setConfig 只读项）+ restart 流程 + HTTP 400。
# 用法：先启动 build/miniweb，再 python3 tests/ws_client.py
import asyncio, json, ssl, sys, urllib.request, urllib.error
import websockets

HTTP = "https://localhost:8443"
WSS  = "wss://localhost:8444"
_fail = []

def check(cond, msg):
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond: _fail.append(msg)

def _noverify():
    c = ssl.create_default_context(); c.check_hostname = False; c.verify_mode = ssl.CERT_NONE; return c

def login(user, password):
    data = json.dumps({"user": user, "password": password}).encode()
    req = urllib.request.Request(HTTP + "/api/login", data=data, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, context=_noverify()) as r:
        return json.loads(r.read())["token"]

async def recv_response(ws, timeout=5):
    """循环 recv，跳过穿插的 event 帧，返回第一条非 event 消息"""
    while True:
        r = json.loads(await asyncio.wait_for(ws.recv(), timeout=timeout))
        if r.get("kind") != "event":
            return r

async def req(ws, i, action, params=None):
    msg = {"id": str(i), "kind": "request", "action": action}
    if params is not None: msg["params"] = params
    await ws.send(json.dumps(msg))
    return await recv_response(ws)

async def main():
    token = login("admin", "admin")
    check(bool(token), "HTTPS login 拿到 token")

    # HTTP 400（非 JSON body）
    try:
        bad = urllib.request.Request(HTTP + "/api/login", data=b"notjson", headers={"Content-Type": "application/json"})
        urllib.request.urlopen(bad, context=_noverify())
        check(False, "非 JSON login 应 400")
    except urllib.error.HTTPError as ex:
        check(ex.code == 400, f"非 JSON login → 400 (got {ex.code})")

    # ---- 主流程：全 action ----
    async with websockets.connect(WSS, ssl=_noverify()) as ws:
        await ws.send(json.dumps({"kind": "auth", "token": token, "proto": "v1"}))
        ao = await recv_response(ws)
        check(ao.get("kind") == "auth_ok", f"auth_ok session={ao.get('session', {}).get('user')}")

        r = await req(ws, 1, "getStatus")
        check(r.get("status") == "ok" and "state" in r.get("data", {}), f"getStatus state={r.get('data', {}).get('state')}")
        r = await req(ws, 2, "getMetrics")
        check(r.get("status") == "ok", f"getMetrics items={len(r.get('data', []))}")
        r = await req(ws, 3, "getConfig")
        check(r.get("status") == "ok" and len(r.get("data", [])) >= 1, "getConfig")
        r = await req(ws, 4, "setConfig", {"key": "log_level", "value": "debug"})
        check(r.get("status") == "ok", "setConfig 正常")
        r = await req(ws, 5, "setConfig", {"key": "version", "value": "x"})
        check(r.get("status") == "error", "setConfig 只读项失败")
        r = await req(ws, 6, "getLogs", {"sinceMs": 0, "limit": 10})
        check(r.get("status") == "ok", "getLogs")
        r = await req(ws, 7, "reloadConfig")
        check(r.get("status") == "ok", "reloadConfig")
        r = await req(ws, 8, "runDiagnostics")
        check(r.get("status") == "ok" and "checks" in r.get("data", {}), "runDiagnostics")

        # subscribe + 收到推送事件
        await ws.send(json.dumps({"id": "s1", "kind": "subscribe", "events": ["metric", "log"]}))
        check((await recv_response(ws)).get("status") == "ok", "subscribe ack")
        ev = json.loads(await asyncio.wait_for(ws.recv(), timeout=5))
        check(ev.get("kind") == "event" and ev.get("event") in ("metric", "log"), f"收到推送事件 {ev.get('event')}")

        # ping→pong（跳过积压 event）
        await ws.send(json.dumps({"kind": "ping"}))
        check((await recv_response(ws)).get("kind") == "pong", "ping→pong")

    # ---- 错误路径 1：未鉴权 ----
    async with websockets.connect(WSS, ssl=_noverify()) as ws:
        await ws.send(json.dumps({"id": "x", "kind": "request", "action": "getStatus"}))
        r = await recv_response(ws)
        check(r.get("status") == "error" and r.get("error", {}).get("code") == "PERM_DENIED", "未鉴权请求 PERM_DENIED")

    # ---- 错误路径 2：错误 token ----
    async with websockets.connect(WSS, ssl=_noverify()) as ws:
        await ws.send(json.dumps({"kind": "auth", "token": "deadbeef", "proto": "v1"}))
        r = json.loads(await asyncio.wait_for(ws.recv(), timeout=5))
        check(r.get("kind") == "error" and r.get("error", {}).get("code") == "PERM_DENIED", "错误 token 鉴权失败")

    # ---- restart 流程 ----
    async with websockets.connect(WSS, ssl=_noverify()) as ws:
        await ws.send(json.dumps({"kind": "auth", "token": token, "proto": "v1"}))
        await recv_response(ws)
        await ws.send(json.dumps({"id": "r1", "kind": "request", "action": "restart"}))
        acc = await recv_response(ws)
        check(acc.get("status") == "accepted", "restart → accepted")
        ev = json.loads(await asyncio.wait_for(ws.recv(), timeout=5))
        check(ev.get("kind") == "event" and "restart" in json.dumps(ev), "收到 restart scheduled 事件")
        try:
            await asyncio.wait_for(ws.recv(), timeout=3); closed = False
        except (websockets.ConnectionClosed, asyncio.TimeoutError):
            closed = True
        check(closed, "restart 后连接关闭")

    print(f"\n=== {'全部通过' if not _fail else f'{len(_fail)} 项失败'} ===")
    return 1 if _fail else 0

if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
