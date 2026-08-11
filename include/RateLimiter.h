// RateLimiter.h
//
// 滑动窗口计数限速器：maxCount 次 / windowMs 毫秒。线程安全。
// HTTPS 的 login 与 WSS 的 restart/setConfig 各自持有一个实例（方案 7.5）。
#pragma once
#include <chrono>
#include <deque>
#include <mutex>

class RateLimiter {
public:
    RateLimiter(int maxCount, int windowMs) : max_(maxCount), window_(windowMs) {}

    // 返回 true 允许通过（并计入），false 表示超限。
    bool allow() {
        std::lock_guard<std::mutex> lk(mu_);
        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - std::chrono::milliseconds(window_);
        while (!times_.empty() && times_.front() < cutoff) times_.pop_front();
        if (static_cast<int>(times_.size()) >= max_) return false;
        times_.push_back(now);
        return true;
    }

private:
    std::mutex mu_;
    int max_;
    int window_;
    std::deque<std::chrono::steady_clock::time_point> times_;
};
