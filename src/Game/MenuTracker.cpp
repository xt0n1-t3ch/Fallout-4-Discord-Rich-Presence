#include "Game/MenuTracker.h"

#include <algorithm>
#include <array>
#include <string>

#include <RE/Fallout.h>

#include "Util/Logger.h"

namespace F4DRP::Game {
namespace {
    struct MenuMapping
    {
        std::string_view menuName;
        MenuKind kind;
    };

    constexpr std::array<MenuMapping, 18> kMenuTable{{
        {"MainMenu", MenuKind::MainMenu},
        {"PauseMenu", MenuKind::PauseMenu},
        {"PipboyMenu", MenuKind::PipBoy},
        {"SleepWaitMenu", MenuKind::Sleep},
        {"LevelUpMenu", MenuKind::LevelUp},
        {"DialogueMenu", MenuKind::Dialogue},
        {"LockpickingMenu", MenuKind::Lockpicking},
        {"TerminalMenu", MenuKind::Terminal},
        {"BarterMenu", MenuKind::Barter},
        {"Crafting Menu", MenuKind::Crafting},
        {"CraftingMenu", MenuKind::Crafting},
        {"VATSMenu", MenuKind::Vats},
        {"CookingMenu", MenuKind::Cooking},
        {"WorkshopMenu", MenuKind::Workshop},
        {"LoadingMenu", MenuKind::LoadingMenu},
        {"FaderMenu", MenuKind::LoadingMenu},
        {"ContainerMenu", MenuKind::Barter},
        {"ExamineMenu", MenuKind::PipBoy},
    }};

    class MenuEventSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
        static MenuEventSink* singleton()
        {
            static MenuEventSink s;
            return &s;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& evn,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            MenuTracker::instance().onMenuOpenClose(std::string_view{evn.menuName.c_str()}, evn.opening);
            return RE::BSEventNotifyControl::kContinue;
        }
    };
} // namespace

MenuTracker& MenuTracker::instance()
{
    static MenuTracker s;
    return s;
}

MenuKind MenuTracker::classify(std::string_view menuName) noexcept
{
    for (const auto& m : kMenuTable) {
        if (m.menuName == menuName) {
            return m.kind;
        }
    }
    return MenuKind::None;
}

void MenuTracker::install()
{
    if (m_installed) {
        return;
    }
    auto* ui = RE::UI::GetSingleton();
    if (ui == nullptr) {
        F4DRP_LOG_ERR("RE::UI singleton unavailable, MenuTracker cannot install");
        return;
    }
    ui->RegisterSink<RE::MenuOpenCloseEvent>(MenuEventSink::singleton());
    m_installed = true;
    F4DRP_LOG_INFO("MenuTracker installed");
}

void MenuTracker::uninstall()
{
    if (!m_installed) {
        return;
    }
    if (auto* ui = RE::UI::GetSingleton()) {
        ui->UnregisterSink<RE::MenuOpenCloseEvent>(MenuEventSink::singleton());
    }
    m_installed = false;
}

void MenuTracker::onMenuOpenClose(std::string_view menuName, bool opening)
{
    const auto kind = classify(menuName);
    if (kind == MenuKind::None) {
        return;
    }
    if (opening) {
        m_current.store(kind, std::memory_order_release);
        if (kind == MenuKind::LoadingMenu) {
            m_newGameFlag.store(false, std::memory_order_release);
        }
    }
    else if (m_current.load(std::memory_order_acquire) == kind) {
        m_current.store(MenuKind::None, std::memory_order_release);
    }
}

void MenuTracker::resetSession()
{
    m_current.store(MenuKind::MainMenu, std::memory_order_release);
    m_newGameFlag.store(false, std::memory_order_release);
}
} // namespace F4DRP::Game
