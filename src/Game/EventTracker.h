#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

#include "Game/GameState.h"

namespace F4DRP::Game {
class EventTracker
{
public:
    static EventTracker& instance();

    void fireEvent(EventKind kind,
                   std::chrono::steady_clock::time_point now,
                   float durationSec,
                   bool allowOverride,
                   std::string payload = {});

    EventKind activeEvent() const noexcept;
    std::string activePayload() const;
    std::chrono::steady_clock::time_point expiresAt() const noexcept;
    bool isActive(std::chrono::steady_clock::time_point now) const noexcept;
    void clearIfExpired(std::chrono::steady_clock::time_point now);

private:
    EventTracker() = default;
    std::atomic<EventKind> m_kind{EventKind::None};
    std::chrono::steady_clock::time_point m_expiresAt{};
    std::string m_payload;
    mutable std::mutex m_mtx;

public:
    using clock = std::chrono::steady_clock;
};

void installEventTrackerSinks();
void uninstallEventTrackerSinks();
} // namespace F4DRP::Game
