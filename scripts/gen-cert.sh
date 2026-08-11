#!/usr/bin/env bash
# 生成自签名 TLS 证书（开发用）。产出 certs/cert.pem 与 certs/key.pem。
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)/certs"
mkdir -p "$DIR"

if [[ -f "$DIR/cert.pem" && -f "$DIR/key.pem" ]]; then
    echo "certs already exist at $DIR, skip"
    exit 0
fi

openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "$DIR/key.pem" -out "$DIR/cert.pem" -days 365 \
    -subj "/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

echo "certs written to $DIR"
