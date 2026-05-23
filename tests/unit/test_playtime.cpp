#include <catch2/catch_test_macros.hpp>

#include "Util/PlayTime.h"

using namespace std::chrono_literals;

TEST_CASE("PlayTime accumulates while running", "[playtime][time]")
{
    F4DRP::Util::PlayTime pt;
    const auto t0 = F4DRP::Util::PlayTime::clock::now();
    pt.start(t0);
    REQUIRE(pt.accumulatedSeconds(t0 + 7s) == 7U);
}

TEST_CASE("PlayTime pauses correctly", "[playtime][time]")
{
    F4DRP::Util::PlayTime pt;
    const auto t0 = F4DRP::Util::PlayTime::clock::now();
    pt.start(t0);
    pt.pause(t0 + 5s);
    REQUIRE(pt.accumulatedSeconds(t0 + 100s) == 5U);
    pt.resume(t0 + 100s);
    REQUIRE(pt.accumulatedSeconds(t0 + 110s) == 15U);
}

TEST_CASE("PlayTime reset clears state", "[playtime][null-empty]")
{
    F4DRP::Util::PlayTime pt;
    const auto t0 = F4DRP::Util::PlayTime::clock::now();
    pt.start(t0);
    pt.pause(t0 + 30s);
    pt.reset();
    REQUIRE(pt.accumulatedSeconds(t0 + 60s) == 0U);
    REQUIRE_FALSE(pt.isRunning());
}
