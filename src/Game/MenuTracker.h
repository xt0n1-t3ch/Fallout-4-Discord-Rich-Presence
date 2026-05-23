#pragma once

#include <atomic>
#include <string>

#include "Game/GameState.h"

namespace F4DRP::Game {
class MenuTracker
{
public:
    static MenuTracker& instance();

    void install();
    void uninstall();

    MenuKind currentMenu() const noexcept { return m_current.load(std::memory_order_acquire); }
    bool newGameStarted() const noexcept { return m_newGameFlag.load(std::memory_order_acquire); }

    void onMenuOpenClose(std::string_view menuName, bool opening);
    void resetSession();

    static MenuKind classify(std::string_view menuName) noexcept;

private:
    MenuTracker() = default;

    std::atomic<MenuKind> m_current{MenuKind::MainMenu};
    std::atomic_bool m_newGameFlag{false};
    bool m_installed = false;
};
} // namespace F4DRP::Game
