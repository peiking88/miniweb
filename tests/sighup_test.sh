#!/usr/bin/env bash
# SIGHUP 证书热重载测试。用法：./tests/sighup_test.sh [miniweb 可执行路径]
set -euo pipefail
cd "$(dirname "$0")/.."
BIN="${1:-./build/miniweb}"

"$BIN" > /tmp/mw-sighup.log 2>&1 &
SRV=$!
trap 'kill $SRV 2>/dev/null; wait $SRV 2>/dev/null' EXIT
sleep 1.5

code1=$(curl -sk -o /dev/null -w "%{http_code}" https://localhost:8443/api/health)
kill -HUP "$SRV"
sleep 1.2
code2=$(curl -sk -o /dev/null -w "%{http_code}" https://localhost:8443/api/health)

if grep -q "certs reloaded" /tmp/mw-sighup.log && [ "$code1" = 200 ] && [ "$code2" = 200 ]; then
    echo "SIGHUP reload OK (health $code1 → $code2, certs reloaded)"
    exit 0
else
    echo "SIGHUP reload FAIL (health $code1 → $code2)"
    cat /tmp/mw-sighup.log
    exit 1
fi
