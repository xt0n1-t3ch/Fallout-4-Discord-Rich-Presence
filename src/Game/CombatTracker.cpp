#include "Game/CombatTracker.h"

#include <RE/Fallout.h>

#include "Constants.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
namespace {
    class CombatSink final : public RE::BSTEventSink<RE::TESCombatEvent>
    {
    public:
        static CombatSink* singleton()
        {
            static CombatSink s;
            return &s;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent& evn,
                                              RE::BSTEventSource<RE::TESCombatEvent>*) override
        {
            auto actor = evn.actor.get();
            auto target = evn.targetActor.get();
            if (actor == nullptr) {
                return RE::BSEventNotifyControl::kContinue;
            }
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (player == nullptr) {
                return RE::BSEventNotifyControl::kContinue;
            }
            if (actor.get() != player && target.get() != player) {
                return RE::BSEventNotifyControl::kContinue;
            }
            auto* hostile = (actor.get() == player) ? target.get() : actor.get();
            if (hostile == nullptr) {
                return RE::BSEventNotifyControl::kContinue;
            }
            const auto state = static_cast<std::uint8_t>(evn.newState.get());
            const bool entering = (state != 0);
            std::string name;
            if (const char* n = hostile->GetDisplayFullName(); n != nullptr) {
                name = n;
            }
            CombatTracker::instance().onCombatChange(hostile->formID, std::move(name), entering);
            return RE::BSEventNotifyControl::kContinue;
        }
    };
} // namespace

CombatTracker& CombatTracker::instance()
{
    static CombatTracker s;
    return s;
}

void CombatTracker::install()
{
    if (m_installed)
        return;
    if (auto* src = RE::ScriptEventSourceHolder::GetSingleton()) {
        src->AddEventSink<RE::TESCombatEvent>(CombatSink::singleton());
        m_installed = true;
        F4DRP_LOG_INFO("CombatTracker installed");
    }
    else {
        F4DRP_LOG_ERR("ScriptEventSourceHolder unavailable");
    }
}

void CombatTracker::uninstall()
{
    if (!m_installed)
        return;
    if (auto* src = RE::ScriptEventSourceHolder::GetSingleton()) {
        src->RemoveEventSink<RE::TESCombatEvent>(CombatSink::singleton());
    }
    m_installed = false;
}

void CombatTracker::onCombatChange(std::uint32_t targetFormID, std::string name, bool entering)
{
    std::scoped_lock lock{m_mtx};
    if (entering) {
        if (m_hostiles.size() >= Constants::kCombatTargetSampleMax) {
            return;
        }
        m_hostiles[targetFormID] = std::move(name);
    }
    else {
        m_hostiles.erase(targetFormID);
    }
}

std::vector<std::string> CombatTracker::snapshotTargetNames()
{
    std::scoped_lock lock{m_mtx};
    std::vector<std::string> out;
    out.reserve(m_hostiles.size());
    for (const auto& [_, name] : m_hostiles) {
        if (!name.empty()) {
            out.push_back(name);
        }
    }
    return out;
}

bool CombatTracker::anyHostile()
{
    std::scoped_lock lock{m_mtx};
    return !m_hostiles.empty();
}
} // namespace F4DRP::Game
