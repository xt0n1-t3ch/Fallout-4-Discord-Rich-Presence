#include "Game/EventTracker.h"

namespace F4DRP::Game {
EventTracker& EventTracker::instance()
{
    static EventTracker s;
    return s;
}

void EventTracker::fireEvent(EventKind kind,
                             std::chrono::steady_clock::time_point now,
                             float durationSec,
                             bool allowOverride)
{
    std::scoped_lock lock{m_mtx};
    const bool active = m_kind.load() != EventKind::None && now < m_expiresAt;
    if (active && !allowOverride) {
        return;
    }
    m_kind.store(kind);
    m_expiresAt = now + std::chrono::milliseconds{static_cast<int>(durationSec * 1000.0F)};
}

EventKind EventTracker::activeEvent() const noexcept
{
    return m_kind.load(std::memory_order_acquire);
}

std::chrono::steady_clock::time_point EventTracker::expiresAt() const noexcept
{
    return m_expiresAt;
}

bool EventTracker::isActive(std::chrono::steady_clock::time_point now) const noexcept
{
    return m_kind.load(std::memory_order_acquire) != EventKind::None && now < m_expiresAt;
}

void EventTracker::clearIfExpired(std::chrono::steady_clock::time_point now)
{
    std::scoped_lock lock{m_mtx};
    if (m_kind.load() != EventKind::None && now >= m_expiresAt) {
        m_kind.store(EventKind::None);
    }
}
} // namespace F4DRP::Game
