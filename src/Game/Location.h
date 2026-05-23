#pragma once

#include <string>

namespace F4DRP::Game {
struct LocationSnapshot
{
    std::string name;
    bool isExterior = true;
    bool valid = false;
};

LocationSnapshot captureLocation();
} // namespace F4DRP::Game
