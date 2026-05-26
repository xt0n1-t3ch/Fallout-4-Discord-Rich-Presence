#pragma once

namespace F4DRP::Game {
using MainThreadTickFn = void (*)();

bool installMainUpdateHook(MainThreadTickFn a_onTick) noexcept;
void uninstallMainUpdateHook() noexcept;
} // namespace F4DRP::Game
