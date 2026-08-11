// tests/test_rate_limiter.cpp — RateLimiter 滑动窗口边界
#include "test_common.h"
#include "RateLimiter.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

static void test_allows_up_to_max() {
    RateLimiter r(3, 1000);   // 3 次 / 1s
    CHECK(r.allow());
    CHECK(r.allow());
    CHECK(r.allow());
    CHECK(!r.allow());        // 第 4 次拒绝
    CHECK(!r.allow());        // 继续拒绝
}

static void test_recovers_after_window() {
    RateLimiter r(2, 200);    // 2 次 / 200ms
    CHECK(r.allow());
    CHECK(r.allow());
    CHECK(!r.allow());
    // sleep_for 保证下限，300ms > 200ms 窗口确定过期（消除 AP13 flaky 风险）
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(r.allow());         // 窗口过期，恢复
    CHECK(r.allow());
    CHECK(!r.allow());
}

static void test_sliding_eviction() {
    RateLimiter r(2, 400);
    CHECK(r.allow());
    // 300ms < 400ms 窗口：仍在窗口内累积第 2 条
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(r.allow());         // 第 2 次
    CHECK(!r.allow());
    // 再 300ms（累计 600ms）：第 1 条（t=0）已滑出窗口（cutoff=200），腾出名额
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(r.allow());
}

static void test_concurrent_safety() {
    const int THREADS = 8, PER = 200;
    RateLimiter r(10, 1000);   // 10 次 / 1s
    std::atomic<int> allowed{0};
    std::vector<std::thread> ts;
    for (int i = 0; i < THREADS; ++i)
        ts.emplace_back([&] { for (int j = 0; j < PER; ++j) if (r.allow()) ++allowed; });
    for (auto& t : ts) t.join();
    // 线程安全 + 窗口上限：1s 内最多 10 次允许；无锁竞态会 >10
    CHECK(allowed.load() <= 10);
    CHECK(allowed.load() > 0);
}

int main() {
    test_allows_up_to_max();
    test_recovers_after_window();
    test_sliding_eviction();
    test_concurrent_safety();
    TEST_SUMMARY("test_rate_limiter");
}
