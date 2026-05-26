#include "Game/EventTracker.h"

#include <RE/Fallout.h>

#include "Config/Settings.h"
#include "Constants.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
namespace {
    EventTracker::clock::time_point g_lastWorkshopFireAt{};
    constexpr std::chrono::milliseconds kWorkshopMinGap{1500};

    std::atomic_uint64_t g_terminalSinkFires{0};
    std::atomic_uint64_t g_workshopSinkFires{0};

    std::string referenceName(RE::TESObjectREFR* ref)
    {
        if (ref == nullptr) {
            return {};
        }
        const char* name = ref->GetDisplayFullName();
        return name == nullptr ? std::string{} : std::string{name};
    }

    class TerminalSink final : public RE::BSTEventSink<RE::TerminalHacked::Event>
    {
    public:
        static TerminalSink* singleton()
        {
            static TerminalSink s;
            return &s;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TerminalHacked::Event& evn,
                                              RE::BSTEventSource<RE::TerminalHacked::Event>*) override
        {
            const auto fires = g_terminalSinkFires.fetch_add(1, std::memory_order_relaxed) + 1;
            const auto now = std::chrono::steady_clock::now();
            auto terminalPtr = evn.terminal.get();
            std::string payload = referenceName(terminalPtr.get());
            F4DRP_LOG_INFO("TerminalHacked sink fired (count={}, payload='{}')", fires, payload);
            EventTracker::instance().fireEvent(
                EventKind::HackedTerminal, now, Constants::kEventStatusDurationDefault, true, std::move(payload));
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    class WorkshopSink final : public RE::BSTEventSink<RE::Workshop::ItemPlacedEvent>
    {
    public:
        static WorkshopSink* singleton()
        {
            static WorkshopSink s;
            return &s;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::Workshop::ItemPlacedEvent& evn,
                                              RE::BSTEventSource<RE::Workshop::ItemPlacedEvent>*) override
        {
            const auto now = std::chrono::steady_clock::now();
            if (now - g_lastWorkshopFireAt < kWorkshopMinGap) {
                return RE::BSEventNotifyControl::kContinue;
            }
            g_lastWorkshopFireAt = now;
            const auto fires = g_workshopSinkFires.fetch_add(1, std::memory_order_relaxed) + 1;
            std::string payload = referenceName(evn.placedItem.get());
            F4DRP_LOG_INFO("Workshop::ItemPlaced sink fired (count={}, payload='{}')", fires, payload);
            EventTracker::instance().fireEvent(
                EventKind::BuiltWorkshopObject, now, Constants::kEventStatusDurationDefault, true, std::move(payload));
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    bool g_installed = false;
} // namespace

EventTracker& EventTracker::instance()
{
    static EventTracker s;
    return s;
}

void EventTracker::fireEvent(EventKind kind,
                             std::chrono::steady_clock::time_point now,
                             float durationSec,
                             bool allowOverride,
                             std::string payload)
{
    std::scoped_lock lock{m_mtx};
    const bool active = m_kind.load() != EventKind::None && now < m_expiresAt;
    if (active && !allowOverride) {
        return;
    }
    m_kind.store(kind);
    m_payload = std::move(payload);
    m_expiresAt = now + std::chrono::milliseconds{static_cast<int>(durationSec * 1000.0F)};
}

EventKind EventTracker::activeEvent() const noexcept
{
    return m_kind.load(std::memory_order_acquire);
}

std::string EventTracker::activePayload() const
{
    std::scoped_lock lock{m_mtx};
    return m_payload;
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
        m_payload.clear();
    }
}

void installEventTrackerSinks()
{
    if (g_installed)
        return;
    bool terminalOk = false;
    bool workshopOk = false;
    if (auto* src = RE::TerminalHacked::GetEventSource()) {
        src->RegisterSink(TerminalSink::singleton());
        terminalOk = true;
    }
    else {
        F4DRP_LOG_WARN("TerminalHacked event source unavailable; skipping sink");
    }
    try {
        RE::Workshop::RegisterForItemPlaced(WorkshopSink::singleton());
        workshopOk = true;
    }
    catch (...) {
        F4DRP_LOG_WARN("Workshop::RegisterForItemPlaced threw; skipping workshop sink");
    }
    g_installed = true;
    F4DRP_LOG_INFO("EventTracker sinks installed (terminal={}, workshop={})", terminalOk, workshopOk);
}

void uninstallEventTrackerSinks()
{
    if (!g_installed)
        return;
    if (auto* src = RE::TerminalHacked::GetEventSource()) {
        src->UnregisterSink(TerminalSink::singleton());
    }
    RE::Workshop::UnregisterForItemPlaced(WorkshopSink::singleton());
    g_installed = false;
}
} // namespace F4DRP::Game
