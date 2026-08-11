#!/usr/bin/env python3
# Origin 白名单校验测试。需以 MINIWEB_ALLOWED_ORIGINS=https://good.com 启动 server。
import asyncio, ssl, sys
import websockets

async def try_connect(origin):
    c = ssl.create_default_context(); c.check_hostname = False; c.verify_mode = ssl.CERT_NONE
    async with websockets.connect("wss://localhost:8444", ssl=c,
                                  additional_headers={"Origin": origin}):
        return True

async def main():
    fails = []
    # bad origin → 应被拒（握手失败）
    try:
        await try_connect("http://evil.com")
        print("  FAIL: bad origin 被接受"); fails.append("bad-accepted")
    except Exception as e:
        print(f"  ok  : bad origin 被拒绝 ({type(e).__name__})")
    # good origin → 应被允许
    try:
        await try_connect("https://good.com")
        print("  ok  : good origin 被接受")
    except Exception as e:
        print(f"  FAIL: good origin 被拒 ({type(e).__name__})"); fails.append("good-rejected")
    print(f"\n=== {'通过' if not fails else str(len(fails)) + ' 项失败'} ===")
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
