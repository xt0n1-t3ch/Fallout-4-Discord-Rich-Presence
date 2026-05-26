#include "Game/MenuTracker.h"

#include <RE/Fallout.h>

#include "Game/MenuCatalog.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
MenuTracker& MenuTracker::instance()
{
    static MenuTracker s;
    return s;
}

void MenuTracker::install()
{
    if (m_installed) {
        return;
    }
    m_installed = true;
    F4DRP_LOG_INFO("MenuTracker installed (polling-only — {} menus)", kMenuCatalog.size());
}

void MenuTracker::uninstall()
{
    m_installed = false;
}

void MenuTracker::poll(RE::UI* ui)
{
    if (ui == nullptr) {
        m_current.store(MenuKind::None, std::memory_order_release);
        m_showLocation.store(false, std::memory_order_release);
        return;
    }

    if (ui->GetMenuOpen(RE::BSFixedString{kLoadingMenuName.data()})) {
        m_current.store(MenuKind::LoadingMenu, std::memory_order_release);
        m_showLocation.store(false, std::memory_order_release);
        return;
    }

    for (const auto& m : kMenuCatalog) {
        if (ui->GetMenuOpen(RE::BSFixedString{m.engineName.data()})) {
            F4DRP_LOG_DBG("MenuTracker: open menu detected = '{}'", m.engineName);
            m_current.store(m.kind, std::memory_order_release);
            m_showLocation.store(m.showsLocation, std::memory_order_release);
            return;
        }
    }

    m_current.store(MenuKind::None, std::memory_order_release);
    m_showLocation.store(false, std::memory_order_release);
}
} // namespace F4DRP::Game
