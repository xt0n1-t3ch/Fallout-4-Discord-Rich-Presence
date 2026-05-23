#pragma once

#include <chrono>
#include <cstdint>

namespace F4DRP::Util {
class PlayTime
{
public:
    using clock = std::chrono::steady_clock;

    void start(clock::time_point now);
    void pause(clock::time_point now);
    void resume(clock::time_point now);
    void reset();

    std::uint64_t accumulatedSeconds(clock::time_point now) const;
    std::int64_t sessionStartUnix() const noexcept { return m_sessionStartUnix; }
    bool isRunning() const noexcept { return m_running; }

private:
    bool m_running = false;
    clock::time_point m_lastResumed{};
    std::uint64_t m_accumulatedMs = 0;
    std::int64_t m_sessionStartUnix = 0;
};
} // namespace F4DRP::Util
