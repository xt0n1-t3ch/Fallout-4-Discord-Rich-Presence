#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include "Game/GameState.h"

namespace F4DRP::Game {
class EventTracker
{
public:
    static EventTracker& instance();

    void fireEvent(EventKind kind, std::chrono::steady_clock::time_point now, float durationSec, bool allowOverride);

    EventKind activeEvent() const noexcept;
    std::chrono::steady_clock::time_point expiresAt() const noexcept;
    bool isActive(std::chrono::steady_clock::time_point now) const noexcept;
    void clearIfExpired(std::chrono::steady_clock::time_point now);

private:
    EventTracker() = default;
    std::atomic<EventKind> m_kind{EventKind::None};
    std::chrono::steady_clock::time_point m_expiresAt{};
    std::mutex m_mtx;
};
} // namespace F4DRP::Game
