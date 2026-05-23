#include "Util/PlayTime.h"

namespace F4DRP::Util {
void PlayTime::start(clock::time_point now)
{
    m_running = true;
    m_lastResumed = now;
    m_accumulatedMs = 0;
    m_sessionStartUnix =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void PlayTime::pause(clock::time_point now)
{
    if (!m_running) {
        return;
    }
    m_accumulatedMs += std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastResumed).count();
    m_running = false;
}

void PlayTime::resume(clock::time_point now)
{
    if (m_running) {
        return;
    }
    m_running = true;
    m_lastResumed = now;
}

void PlayTime::reset()
{
    m_running = false;
    m_accumulatedMs = 0;
    m_sessionStartUnix = 0;
}

std::uint64_t PlayTime::accumulatedSeconds(clock::time_point now) const
{
    std::uint64_t total = m_accumulatedMs;
    if (m_running) {
        total += std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastResumed).count();
    }
    return total / 1000U;
}
} // namespace F4DRP::Util
