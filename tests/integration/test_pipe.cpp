#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "Discord/Client.h"
#include "Discord/Pipe.h"

using namespace std::chrono_literals;

TEST_CASE("Discord pipe opens against running desktop client", "[integration][live]")
{
    F4DRP::Discord::Pipe pipe;
    const bool opened = pipe.open();
    REQUIRE(opened);
    REQUIRE(pipe.isOpen());
}

TEST_CASE("Discord Client handshakes and reaches Ready", "[integration][live]")
{
    F4DRP::Discord::Client client;
    client.start("12345678901234567");
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!client.isReady() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }
    REQUIRE(client.isReady());
    client.stop();
}

TEST_CASE("Discord Client accepts SET_ACTIVITY update", "[integration][live]")
{
    F4DRP::Discord::Client client;
    client.start("12345678901234567");
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!client.isReady() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }
    REQUIRE(client.isReady());

    F4DRP::Discord::PresenceState p;
    p.details = "F4DRP integration test";
    p.state = "Hello from CI";
    p.largeImageKey = "fallout4";
    p.largeImageText = "Fallout 4";
    p.startTimestampUnix =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    p = F4DRP::Discord::clampToDiscordLimits(p);

    client.update(p, 5);
    std::this_thread::sleep_for(2s);
    REQUIRE(client.isReady());
    client.stop();
}
