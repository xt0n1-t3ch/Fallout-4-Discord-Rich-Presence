#include "Discord/RateLimiter.h"

#include <algorithm>

#include "Constants.h"

namespace F4DRP::Discord {
RateLimiter::Decision RateLimiter::admit(std::uint64_t presenceHash, clock::time_point now, int updateIntervalSec)
{
    const int floored = std::max(updateIntervalSec, Constants::kUpdateIntervalFloorSec);
    const auto minBetween = std::chrono::seconds{floored};
    const bool stateChanged = (presenceHash != m_lastHash);

    if (m_haveSent && now - m_lastSent < minBetween && !stateChanged) {
        const auto retry = std::chrono::duration_cast<std::chrono::milliseconds>(minBetween - (now - m_lastSent));
        return Decision{false, retry};
    }

    const auto window = std::chrono::seconds{Constants::kRateLimitWindowSec};
    while (!m_window.empty() && now - m_window.front() > window) {
        m_window.pop_front();
    }

    if (static_cast<int>(m_window.size()) >= Constants::kRateLimitMaxPerWindow) {
        const auto oldest = m_window.front();
        const auto retry = std::chrono::duration_cast<std::chrono::milliseconds>(window - (now - oldest) +
                                                                                 std::chrono::milliseconds{100});
        return Decision{false, retry};
    }

    m_window.push_back(now);
    m_lastHash = presenceHash;
    m_lastSent = now;
    m_haveSent = true;
    return Decision{true, std::chrono::milliseconds{0}};
}

void RateLimiter::reset()
{
    m_window.clear();
    m_lastHash = 0;
    m_haveSent = false;
}
} // namespace F4DRP::Discord
