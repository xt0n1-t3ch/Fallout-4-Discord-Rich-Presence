#pragma once

#include <chrono>
#include <cstdint>
#include <deque>

namespace F4DRP::Discord {
class RateLimiter
{
public:
    using clock = std::chrono::steady_clock;

    struct Decision
    {
        bool send;
        std::chrono::milliseconds suggestedRetryAfter;
    };

    Decision admit(std::uint64_t presenceHash, clock::time_point now, int updateIntervalSec);
    void reset();

private:
    std::deque<clock::time_point> m_window;
    std::uint64_t m_lastHash = 0;
    clock::time_point m_lastSent{};
    bool m_haveSent = false;
};
} // namespace F4DRP::Discord
