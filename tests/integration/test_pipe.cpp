#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Discord/Client.h"
#include "Discord/Pipe.h"
#include "Discord/Protocol.h"

using namespace std::chrono_literals;

TEST_CASE("Discord pipe opens against running desktop client", "[integration][live]")
{
    F4DRP::Discord::Pipe pipe;
    const bool opened = pipe.open();
    REQUIRE(opened);
    REQUIRE(pipe.isOpen());
}

TEST_CASE("Discord pipe round-trip: HANDSHAKE -> any reply within 5s", "[integration][live]")
{
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
    F4DRP::Discord::Client client;
    client.start("12345678901234567");
    std::this_thread::sleep_for(2s);
    client.stop();
    SUCCEED("client lifecycle survives without crash even with invalid AppID");
}
