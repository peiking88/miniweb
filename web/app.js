// miniweb 管理控制台 — 纯原生 ES5，零依赖。
// 连 HTTPS(:8443) 登录、WSS(:8444) 管理通信；含重连/心跳/超时/restart 处理。
"use strict";

var HTTP_BASE = "https://" + location.hostname + ":8443";
var WS_BASE   = "wss://"   + location.hostname + ":8444";
var SUBS = ["metric", "log", "state", "alert"];

var token = localStorage.getItem("miniweb_token");
var session = null;
var ws = null;
var wsReady = false;
var reconnectDelay = 1000;
var reconnectTimer = null;
var heartbeatTimer = null;
var pending = {};          // id → {cb, timer}
var nextId = 1;
var afterRestart = false;

// ---------- 工具 ----------
function $(id) { return document.getElementById(id); }
function esc(s) {
    return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
                     .replace(/"/g, "&quot;");
}
function fmtUptime(ms) {
    var s = Math.floor(ms / 1000);
    var d = Math.floor(s / 86400); s %= 86400;
    var h = Math.floor(s / 3600); s %= 3600;
    var m = Math.floor(s / 60); s %= 60;
    return (d ? d + "d " : "") + (h ? h + "h " : "") + (m ? m + "m " : "") + s + "s";
}
function fmtTime(d) {
    function p(n) { return n < 10 ? "0" + n : n; }
    return p(d.getHours()) + ":" + p(d.getMinutes()) + ":" + p(d.getSeconds());
}

// ---------- HTTP（登录） ----------
function xhr(method, url, body, cb) {
    var x = new XMLHttpRequest();
    x.open(method, HTTP_BASE + url, true);
    x.setRequestHeader("Content-Type", "application/json");
    x.onreadystatechange = function () {
        if (x.readyState === 4) {
            var data = null;
            try { data = JSON.parse(x.responseText); } catch (e) {}
            cb(x.status, data);
        }
    };
    x.send(body ? JSON.stringify(body) : null);
}

function doLogin() {
    var user = $("user").value.trim();
    var pass = $("pass").value;
    $("login-error").textContent = "";
    xhr("POST", "/api/login", { user: user, password: pass }, function (status, data) {
        if (status === 200 && data && data.token) {
            token = data.token;
            localStorage.setItem("miniweb_token", token);
            enterMain();
        } else if (status === 429) {
            $("login-error").textContent = "尝试过于频繁，请稍候";
        } else {
            $("login-error").textContent = "登录失败（" + status + "）";
        }
    });
}

function doLogout() {
    token = null;
    localStorage.removeItem("miniweb_token");
    if (ws) { ws.onclose = null; try { ws.close(); } catch (e) {} ws = null; }
    stopHeartbeat();
    showLogin();
}

// ---------- 视图 ----------
function showLogin() {
    $("login-view").style.display = "";
    $("main-view").style.display = "none";
}
function enterMain() {
    $("login-view").style.display = "none";
    $("main-view").style.display = "";
    connect();
}
function setConn(text, cls) {
    var el = $("conn-status");
    el.textContent = text;
    el.className = "badge " + (cls || "");
}

// ---------- WS 连接 ----------
function connect() {
    setConn("连接中…", "warn");
    wsReady = false;
    try {
        ws = new WebSocket(WS_BASE);
    } catch (e) {
        setConn("无法建立连接", "err");
        scheduleReconnect();
        return;
    }
    ws.onopen = function () {
        reconnectDelay = 1000;
        ws.send(JSON.stringify({ kind: "auth", token: token, proto: "v1" }));
    };
    ws.onmessage = onMessage;
    ws.onclose = function () {
        wsReady = false;
        stopHeartbeat();
        if (afterRestart) {
            setConn("重启中…等待进程恢复", "warn");
            pollHealth();
            return;
        }
        setConn("已断开，重连中…", "err");
        scheduleReconnect();
    };
    ws.onerror = function () {};
}

function scheduleReconnect() {
    if (reconnectTimer) return;
    if (!navigator.onLine) { setConn("离线，等待网络…", "warn"); return; }
    var delay = reconnectDelay;
    reconnectDelay = Math.min(reconnectDelay * 2, 30000);   // 指数退避，上限 30s
    reconnectTimer = setTimeout(function () {
        reconnectTimer = null;
        connect();
    }, delay);
}

function startHeartbeat() {
    stopHeartbeat();
    heartbeatTimer = setInterval(function () {
        if (ws && ws.readyState === 1) ws.send(JSON.stringify({ kind: "ping" }));
    }, 30000);
}
function stopHeartbeat() {
    if (heartbeatTimer) { clearInterval(heartbeatTimer); heartbeatTimer = null; }
}

function onMessage(evt) {
    var msg;
    try { msg = JSON.parse(evt.data); } catch (e) { return; }
    switch (msg.kind) {
        case "auth_ok":
            session = msg.session;
            wsReady = true;
            setConn("已连接", "ok");
            $("user-info").textContent = session.user + "（" + session.role + "）";
            startHeartbeat();
            ws.send(JSON.stringify({ id: String(nextId++), kind: "subscribe", events: SUBS }));
            refreshAll();
            break;
        case "response":
            handleResponse(msg);
            break;
        case "event":
            handleEvent(msg);
            break;
        case "pong":
            break;
        case "error":
            // 鉴权失败等：回登录页
            if (ws) { ws.onclose = null; ws.close(); }
            token = null;
            localStorage.removeItem("miniweb_token");
            showLogin();
            $("login-error").textContent = (msg.error && msg.error.message) || "会话失效，请重新登录";
            break;
    }
}

// ---------- 请求 / 响应 ----------
function request(action, params, cb) {
    if (!wsReady) { cb(new Error("未连接")); return; }
    var id = String(nextId++);
    var entry = { cb: cb };
    entry.timer = setTimeout(function () {     // 10s 超时
        if (pending[id]) { delete pending[id]; cb(new Error("请求超时: " + action)); }
    }, 10000);
    pending[id] = entry;
    ws.send(JSON.stringify({ id: id, kind: "request", action: action, params: params || {} }));
}
function handleResponse(msg) {
    var entry = pending[msg.id];
    if (!entry) return;
    clearTimeout(entry.timer);
    delete pending[msg.id];
    if (msg.status === "ok" || msg.status === "accepted") entry.cb(null, msg.data);
    else entry.cb(new Error((msg.error && msg.error.message) || msg.status));
}

// ---------- 事件 ----------
function handleEvent(msg) {
    if (msg.event === "metric") updateMetric(msg.data);
    else if (msg.event === "log") appendLog(msg.data);
    else if (msg.event === "alert") appendLog({ level: "warn", message: "[alert] " + JSON.stringify(msg.data), timestampMs: Date.now() });
    else if (msg.event === "state") appendLog({ level: "info", message: "[state] " + JSON.stringify(msg.data), timestampMs: Date.now() });
}

// ---------- 数据刷新与渲染 ----------
function refreshAll() {
    request("getStatus", {}, function (e, d) { if (!e) renderStatus(d); });
    request("getMetrics", {}, function (e, d) { if (!e) renderMetrics(d); });
    request("getConfig", {}, function (e, d) { if (!e) renderConfig(d); });
    request("getLogs", { sinceMs: 0, limit: 50 }, function (e, d) { if (!e) renderLogs(d); });
}

function renderStatus(s) {
    $("status").innerHTML =
        '<div>状态：<b>' + esc(s.state) + '</b></div>' +
        '<div>运行时长：' + fmtUptime(s.uptimeMs) + '</div>' +
        '<div>版本：' + esc(s.version) + '</div>' +
        '<div>PID：' + s.pid + '</div>';
}

function renderMetrics(ms) {
    if (!ms || !ms.length) { $("metrics").innerHTML = '<i>无</i>'; return; }
    var html = "";
    for (var i = 0; i < ms.length; i++) {
        html += '<div class="metric"><span>' + esc(ms[i].name) + '</span>' +
                '<b>' + Number(ms[i].value).toFixed(2) + '</b><small>' + esc(ms[i].unit) + '</small></div>';
    }
    $("metrics").innerHTML = html;
}

function updateMetric(m) {
    var els = $("metrics").getElementsByClassName("metric");
    for (var i = 0; i < els.length; i++) {
        var span = els[i].getElementsByTagName("span")[0];
        if (span && span.textContent === m.name) {
            els[i].getElementsByTagName("b")[0].textContent = Number(m.value).toFixed(2);
        }
    }
}

function renderConfig(cs) {
    if (!cs || !cs.length) { $("config").innerHTML = '<i>无</i>'; return; }
    var html = "";
    for (var i = 0; i < cs.length; i++) {
        var c = cs[i];
        html += '<div class="cfg' + (c.readOnly ? " ro" : "") + '">' +
                '<span>' + esc(c.key) + '</span>' +
                (c.readOnly
                    ? '<input value="' + esc(c.value) + '" disabled>'
                    : '<input value="' + esc(c.value) + '" data-key="' + esc(c.key) + '" onchange="saveConfig(this)">') +
                '</div>';
    }
    $("config").innerHTML = html;
}

function saveConfig(input) {
    request("setConfig", { key: input.dataset.key, value: input.value }, function (e) {
        alert(e ? "保存失败: " + e.message : "已保存 " + input.dataset.key);
    });
}

function renderLogs(ls) {
    $("logs").innerHTML = "";
    if (!ls) return;
    for (var i = 0; i < ls.length; i++) appendLog(ls[i]);
}

function appendLog(l) {
    var box = $("logs");
    var div = document.createElement("div");
    div.className = "log " + (l.level || "info");
    var t = new Date(l.timestampMs || Date.now());
    div.textContent = "[" + fmtTime(t) + "] " + (l.level || "").toUpperCase() + " " + l.message;
    box.appendChild(div);
    while (box.childNodes.length > 200) box.removeChild(box.firstChild);   // 限 200 条
    box.scrollTop = box.scrollHeight;
}

// ---------- 操作 ----------
function doRestart() {
    if (!confirm("确认重启宿主应用？连接将断开。")) return;
    afterRestart = true;
    request("restart", {}, function (e) {
        if (e) { afterRestart = false; alert("重启请求失败: " + e.message); }
    });
}
function doReload() {
    request("reloadConfig", {}, function (e) {
        alert(e ? "失败: " + e.message : "配置已重载");
        if (!e) refreshAll();
    });
}
function doDiag() {
    request("runDiagnostics", {}, function (e, d) {
        $("diag-result").textContent = e ? "失败: " + e.message : JSON.stringify(d, null, 2);
    });
}

// restart 后轮询 health，恢复后回登录页（旧 token 已失效）
function pollHealth() {
    setTimeout(function () {
        xhr("GET", "/api/health", null, function (status) {
            if (status === 200 && !afterRestart) return;     // 已处理
            if (status === 200) {
                afterRestart = false;
                token = null;
                localStorage.removeItem("miniweb_token");
                showLogin();
                $("login-error").textContent = "宿主已重启，请重新登录";
            } else {
                pollHealth();
            }
        });
    }, 2000);
}

// ---------- 启动 ----------
window.onload = function () {
    $("login-btn").onclick = doLogin;
    $("logout-btn").onclick = doLogout;
    $("btn-restart").onclick = doRestart;
    $("btn-reload").onclick = doReload;
    $("btn-diag").onclick = doDiag;
    $("pass").addEventListener("keydown", function (e) { if (e.keyCode === 13) doLogin(); });
    window.addEventListener("online", function () {
        if (token && !wsReady) { reconnectDelay = 1000; scheduleReconnect(); }
    });
    if (token) enterMain(); else showLogin();
};
