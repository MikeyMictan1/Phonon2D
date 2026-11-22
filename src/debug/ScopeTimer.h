#pragma once

#include <chrono>
#include <cstdint>

// RAII timer, starts in the constructor and stops in the destructor
class ScopeTimer 
{
public:
    explicit ScopeTimer(int64_t &p_target) : target(p_target), start_time(std::chrono::steady_clock::now()) {}

    ~ScopeTimer() {
        std::chrono::steady_clock::duration runtime = std::chrono::steady_clock::now() - start_time;
        target += std::chrono::duration_cast<std::chrono::nanoseconds>(runtime).count();
    }

    // Prevent copies
	ScopeTimer(const ScopeTimer &) = delete;
	ScopeTimer &operator=(const ScopeTimer &) = delete;

private:
    int64_t &target;
    std::chrono::steady_clock::time_point start_time;
};