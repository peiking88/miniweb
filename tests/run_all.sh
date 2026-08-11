#!/usr/bin/env bash
# 一键全量测试：C++ 单测 → WSS 端到端 → SIGHUP → Origin → 前端（可选）。
# 退出码 = 失败数（0 表示全部通过）。
# 用法：./tests/run_all.sh
set -u
cd "$(dirname "$0")/.."

BIN=./build/miniweb
declare -i PASS=0 FAIL=0 SKIP=0

# 清理可能残留的 server 进程（避免端口占用导致后续 bind 失败）
pkill -f "build/miniweb" 2>/dev/null || true
sleep 1

record() {  # name rc   (rc 77 = skip)
    case "$2" in
        0)  echo "  >>> PASS: $1"; PASS+=1;;
        77) echo "  >>> SKIP: $1"; SKIP+=1;;
        *)  echo "  >>> FAIL: $1"; FAIL+=1;;
    esac
}
showlog() { [ "$1" -ne 0 ] && { echo "  --- 日志 ---"; tail -20 /tmp/ra.log; }; }

# ---- 前置：构建 + 证书 ----
if [ ! -x "$BIN" ]; then
    echo ">>> 未发现构建产物，开始构建..."
    { cmake -B build -G Ninja && ninja -C build -j$(nproc); } || { echo "构建失败，中止"; exit 1; }
fi
[ -f certs/cert.pem ] || ./scripts/gen-cert.sh

# ---- [1/5] C++ 单元测试（CTest，无需 server）----
echo; echo "===== [1/5] C++ 单元测试 ====="
(cd build && ctest --output-on-failure) >/tmp/ra.log 2>&1; rc=$?
showlog $rc
record "ctest (json/rate_limiter/host)" $rc

# ---- [2/5] WSS 端到端 ----
echo; echo "===== [2/5] WSS 端到端 ====="
$BIN > /tmp/ra.log 2>&1 & SRV=$!; sleep 1.5
python3 tests/ws_client.py; rc=$?
showlog $rc
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
record "ws_client (全 action)" $rc

# ---- [3/5] SIGHUP 证书热重载（脚本自带启停）----
echo; echo "===== [3/5] SIGHUP 证书热重载 ====="
./tests/sighup_test.sh >/tmp/ra.log 2>&1; rc=$?
showlog $rc
record "sighup reload" $rc

# ---- [4/5] Origin 白名单 ----
echo; echo "===== [4/5] Origin 白名单 ====="
MINIWEB_ALLOWED_ORIGINS=https://good.com $BIN > /tmp/ra.log 2>&1 & SRV=$!; sleep 1.5
python3 tests/origin_test.py; rc=$?
showlog $rc
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
record "origin (bad 拒绝 / good 放行)" $rc

# ---- [5/5] 前端浏览器端到端（需 Playwright，自动检测）----
echo; echo "===== [5/5] 前端浏览器端到端 ====="
have_pw=false
[ -d /tmp/fe-test/node_modules/playwright ] && ls ~/.cache/ms-playwright/chromium* >/dev/null 2>&1 && have_pw=true
if $have_pw; then
    cp tests/frontend_test.cjs /tmp/fe-test/
    $BIN > /tmp/ra.log 2>&1 & SRV=$!; sleep 1.5
    ( cd /tmp/fe-test && node frontend_test.cjs ); rc=$?
    showlog $rc
    kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
    record "frontend (Playwright)" $rc
else
    echo "  Playwright 未就绪，跳过（安装见 README 第 4 节）"
    record "frontend" 77
fi

# ---- 汇总 ----
echo; echo "============================="
echo "汇总：PASS=$PASS  FAIL=$FAIL  SKIP=$SKIP"
if [ $FAIL -eq 0 ]; then echo "全部通过 ✅"; else echo "有失败 ❌"; fi
exit $FAIL
