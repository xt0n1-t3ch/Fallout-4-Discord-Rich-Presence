#include "Game/Location.h"

#include <RE/Fallout.h>

#include "Game/PlayerAccess.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
LocationSnapshot captureLocation()
{
    LocationSnapshot r;
    F4DRP_LOG_DBG("LOC l1: enter captureLocation");
    auto* player = getPlayerSafe();
    F4DRP_LOG_DBG("LOC l2: player={}", static_cast<const void*>(player));
    if (player == nullptr) {
        return r;
    }
    auto* parentCell = player->parentCell;
    F4DRP_LOG_DBG("LOC l3: parentCell={}", static_cast<const void*>(parentCell));
    if (parentCell == nullptr) {
        return r;
    }
    r.isExterior = !parentCell->IsInterior();
    F4DRP_LOG_DBG("LOC l4: IsInterior OK, isExterior={}", r.isExterior);

    if (auto* loc = player->currentLocation) {
        F4DRP_LOG_DBG("LOC l5: currentLocation={}", static_cast<const void*>(loc));
        if (const char* lname = loc->GetFullName(); lname != nullptr && lname[0] != '\0') {
            r.name = lname;
            r.valid = true;
            F4DRP_LOG_DBG("LOC l6: returned currentLocation.name='{}'", r.name);
            return r;
        }
    }
    if (const char* cname = parentCell->GetFullName(); cname != nullptr && cname[0] != '\0') {
        r.name = cname;
        r.valid = true;
        F4DRP_LOG_DBG("LOC l7: returned parentCell.name='{}'", r.name);
        return r;
    }
    if (auto* ws = parentCell->worldSpace) {
        if (const char* wname = ws->GetFullName(); wname != nullptr && wname[0] != '\0') {
            r.name = wname;
            r.valid = true;
            F4DRP_LOG_DBG("LOC l8: returned worldSpace.name='{}'", r.name);
        }
    }
    return r;
}
} // namespace F4DRP::Game
