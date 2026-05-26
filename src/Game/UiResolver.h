#pragma once

namespace RE {
class UI;
}

namespace F4DRP::Game {
RE::UI* resolveUi() noexcept;
void resetUiResolver() noexcept;
} // namespace F4DRP::Game
