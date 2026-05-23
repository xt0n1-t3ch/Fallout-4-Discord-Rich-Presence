#include "Game/Location.h"

#include <RE/Fallout.h>

namespace F4DRP::Game {
LocationSnapshot captureLocation()
{
    LocationSnapshot r;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (player == nullptr) {
        return r;
    }
    auto* parentCell = player->parentCell;
    if (parentCell == nullptr) {
        return r;
    }
    r.isExterior = !parentCell->IsInterior();

    if (auto* loc = player->currentLocation) {
        if (const char* lname = loc->GetFullName(); lname != nullptr && lname[0] != '\0') {
            r.name = lname;
            r.valid = true;
            return r;
        }
    }
    if (const char* cname = parentCell->GetFullName(); cname != nullptr && cname[0] != '\0') {
        r.name = cname;
        r.valid = true;
        return r;
    }
    if (auto* ws = parentCell->worldSpace) {
        if (const char* wname = ws->GetFullName(); wname != nullptr && wname[0] != '\0') {
            r.name = wname;
            r.valid = true;
        }
    }
    return r;
}
} // namespace F4DRP::Game
