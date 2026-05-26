#pragma once

#include <atomic>

#include "Game/GameState.h"

namespace RE {
class UI;
}

namespace F4DRP::Game {
class MenuTracker
{
public:
    static MenuTracker& instance();

    void install();
    void uninstall();

    void poll(RE::UI* ui);

    MenuKind currentMenu() const noexcept { return m_current.load(std::memory_order_acquire); }
    bool showsLocation() const noexcept { return m_showLocation.load(std::memory_order_acquire); }

private:
    MenuTracker() = default;

    std::atomic<MenuKind> m_current{MenuKind::None};
    std::atomic_bool m_showLocation{false};
    bool m_installed = false;
};
} // namespace F4DRP::Game
