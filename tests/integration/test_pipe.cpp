#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include <Windows.h>
#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Discord/Client.h"
#include "Discord/Pipe.h"
#include "Discord/Protocol.h"

using namespace std::chrono_literals;

namespace {
constexpr std::string_view kNetworkGateEnv = "F4DRP_LIVE_DISCORD_NETWORK";

bool networkGateEnabled()
{
    const char* v = std::getenv(std::string{kNetworkGateEnv}.c_str());
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}
} // namespace

#define REQUIRE_LIVE_NETWORK()                                                                                         \
    do {                                                                                                               \
        if (!networkGateEnabled()) {                                                                                   \
            SKIP("F4DRP_LIVE_DISCORD_NETWORK not set; skipping live Discord network round-trip.");                     \
        }                                                                                                              \
    } while (false)

TEST_CASE("Discord pipe opens against running desktop client", "[integration][live]")
{
    REQUIRE_LIVE_NETWORK();
    F4DRP::Discord::Pipe pipe;
    const bool opened = pipe.open();
    REQUIRE(opened);
    REQUIRE(pipe.isOpen());
}

TEST_CASE("Discord pipe round-trip: HANDSHAKE -> any reply within 5s", "[integration][live]")
{
    REQUIRE_LIVE_NETWORK();
    F4DRP::Discord::Pipe pipe;
    REQUIRE(pipe.open());

    const auto handshakeJson = F4DRP::Discord::buildHandshakePayload("12345678901234567").dump();
    const auto bytes = F4DRP::Discord::encodeFrame(F4DRP::Discord::Opcode::Handshake, handshakeJson);
    REQUIRE(pipe.write(bytes));

    const auto deadline = std::chrono::steady_clock::now() + 5s;
    std::optional<F4DRP::Discord::Frame> frame;
    while (std::chrono::steady_clock::now() < deadline) {
        frame = pipe.readFrame();
        if (frame) {
            break;
        }
        std::this_thread::sleep_for(50ms);
    }
    REQUIRE(frame.has_value());
    INFO("Discord replied opcode=" << static_cast<int>(frame->opcode) << " payload=" << frame->payload.substr(0, 200));
    REQUIRE((frame->opcode == F4DRP::Discord::Opcode::Frame || frame->opcode == F4DRP::Discord::Opcode::Close));
    REQUIRE_FALSE(frame->payload.empty());
}

TEST_CASE("Discord Client connects and runs without crash for 2s", "[integration][live]")
{
    REQUIRE_LIVE_NETWORK();
    F4DRP::Discord::Client client;
    client.start("12345678901234567");
    std::this_thread::sleep_for(2s);
    client.stop();
    SUCCEED("client lifecycle survives without crash even with invalid AppID");
}

TEST_CASE("Discord E2E: real AppID handshake + SetActivity is accepted", "[integration][live][e2e]")
{
    REQUIRE_LIVE_NETWORK();
    F4DRP::Discord::Pipe pipe;
    REQUIRE(pipe.open());

    const std::string appId = "1507704875939790889";
    const auto handshakeJson = F4DRP::Discord::buildHandshakePayload(appId).dump();
    const auto handshakeBytes = F4DRP::Discord::encodeFrame(F4DRP::Discord::Opcode::Handshake, handshakeJson);
    REQUIRE(pipe.write(handshakeBytes));

    auto readWithin = [&](std::chrono::milliseconds timeout) -> std::optional<F4DRP::Discord::Frame> {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (auto f = pipe.readFrame(); f) {
                return f;
            }
            std::this_thread::sleep_for(50ms);
        }
        return std::nullopt;
    };

    const auto ready = readWithin(5s);
    REQUIRE(ready.has_value());
    INFO("READY opcode=" << static_cast<int>(ready->opcode) << " payload=" << ready->payload.substr(0, 300));
    REQUIRE(ready->opcode == F4DRP::Discord::Opcode::Frame);
    REQUIRE(ready->payload.find("READY") != std::string::npos);

    F4DRP::Discord::PresenceState presence;
    presence.details = "E2E test run";
    presence.state = "validating cross-gen plugin";
    presence.largeImageKey = "fallout4";
    presence.largeImageText = "Fallout 4";
    presence.startTimestampUnix =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    const auto activityJson =
        F4DRP::Discord::buildSetActivityPayload(static_cast<int>(::GetCurrentProcessId()), "e2e-1", presence).dump();
    const auto activityBytes = F4DRP::Discord::encodeFrame(F4DRP::Discord::Opcode::Frame, activityJson);
    REQUIRE(pipe.write(activityBytes));

    const auto ack = readWithin(5s);
    REQUIRE(ack.has_value());
    INFO("SET_ACTIVITY ACK opcode=" << static_cast<int>(ack->opcode) << " payload=" << ack->payload.substr(0, 500));
    REQUIRE(ack->opcode == F4DRP::Discord::Opcode::Frame);
    REQUIRE(ack->payload.find("\"error\"") == std::string::npos);
    REQUIRE((ack->payload.find("\"evt\":null") != std::string::npos ||
             ack->payload.find("SET_ACTIVITY") != std::string::npos));

    std::this_thread::sleep_for(3s);
}
