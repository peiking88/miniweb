// tests/test_common.h — 轻量测试宏（无第三方框架依赖）。
// 每个 test_*.cpp 含独立 main，末尾 TEST_SUMMARY 返回非 0 表示有失败（CTest 据此判定）。
#pragma once
#include <cstdio>

static int g_fail = 0, g_pass = 0;

#define CHECK(cond) do { \
    if (cond) { ++g_pass; } \
    else { std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_fail; } \
} while (0)

#define TEST_SUMMARY(name) do { \
    std::printf("%s: %d passed, %d failed\n", name, g_pass, g_fail); \
    return g_fail ? 1 : 0; \
} while (0)
