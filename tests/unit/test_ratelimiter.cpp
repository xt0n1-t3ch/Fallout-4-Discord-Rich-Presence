#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Discord/RateLimiter.h"

using namespace std::chrono_literals;

TEST_CASE("RateLimiter suppresses repeat hashes inside interval", "[ratelimit][time]")
{
    F4DRP::Discord::RateLimiter rl;
    const auto t0 = F4DRP::Discord::RateLimiter::clock::now();
    auto d1 = rl.admit(0xABCD, t0, 5);
    REQUIRE(d1.send);
    auto d2 = rl.admit(0xABCD, t0 + 1s, 5);
    REQUIRE_FALSE(d2.send);
}

TEST_CASE("RateLimiter admits state change even before interval", "[ratelimit][time]")
{
    F4DRP::Discord::RateLimiter rl;
    const auto t0 = F4DRP::Discord::RateLimiter::clock::now();
    REQUIRE(rl.admit(0xAAAA, t0, 5).send);
    auto d = rl.admit(0xBBBB, t0 + 1s, 5);
    REQUIRE(d.send);
}

TEST_CASE("RateLimiter enforces 5-in-20s ceiling", "[ratelimit][boundary]")
{
    F4DRP::Discord::RateLimiter rl;
    const auto t0 = F4DRP::Discord::RateLimiter::clock::now();
    REQUIRE(rl.admit(0x01, t0, 5).send);
    REQUIRE(rl.admit(0x02, t0 + 6s, 5).send);
    REQUIRE(rl.admit(0x03, t0 + 12s, 5).send);
    REQUIRE(rl.admit(0x04, t0 + 18s, 5).send);
    REQUIRE_FALSE(rl.admit(0x05, t0 + 18500ms, 5).send);
}

TEST_CASE("RateLimiter floors updateInterval to kUpdateIntervalFloorSec", "[ratelimit][boundary]")
{
    F4DRP::Discord::RateLimiter rl;
    const auto t0 = F4DRP::Discord::RateLimiter::clock::now();
    REQUIRE(rl.admit(0x01, t0, 1).send);
    auto d = rl.admit(0x01, t0 + 2s, 1);
    REQUIRE_FALSE(d.send);
    REQUIRE(rl.admit(0x02, t0 + std::chrono::seconds{F4DRP::Constants::kUpdateIntervalFloorSec + 1}, 1).send);
}
