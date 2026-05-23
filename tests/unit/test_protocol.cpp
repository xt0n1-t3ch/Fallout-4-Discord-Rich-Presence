#include <cstring>

#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Discord/Protocol.h"

TEST_CASE("encodeFrame produces correct header + payload", "[protocol][format]")
{
    const std::string body = R"({"v":1,"client_id":"123"})";
    const auto bytes = F4DRP::Discord::encodeFrame(F4DRP::Discord::Opcode::Handshake, body);
    REQUIRE(bytes.size() == F4DRP::Constants::kPayloadHeaderBytes + body.size());

    std::uint32_t opcode = 0;
    std::uint32_t length = 0;
    std::memcpy(&opcode, bytes.data(), sizeof(opcode));
    std::memcpy(&length, bytes.data() + sizeof(opcode), sizeof(length));
    REQUIRE(opcode == static_cast<std::uint32_t>(F4DRP::Discord::Opcode::Handshake));
    REQUIRE(length == body.size());
    REQUIRE(std::string(reinterpret_cast<const char*>(bytes.data() + F4DRP::Constants::kPayloadHeaderBytes), length) ==
            body);
}

TEST_CASE("decodeFrame round-trips encodeFrame", "[protocol][format]")
{
    const std::string body = R"({"cmd":"SET_ACTIVITY"})";
    const auto bytes = F4DRP::Discord::encodeFrame(F4DRP::Discord::Opcode::Frame, body);
    const auto frame = F4DRP::Discord::decodeFrame(bytes);
    REQUIRE(frame.opcode == F4DRP::Discord::Opcode::Frame);
    REQUIRE(frame.payload == body);
}

TEST_CASE("clampToDiscordLimits truncates oversize fields", "[protocol][boundary]")
{
    F4DRP::Discord::PresenceState p;
    p.details = std::string(200, 'a');
    p.state = std::string(200, 'b');
    p.largeImageText = std::string(200, 'c');
    p.smallImageText = std::string(200, 'd');
    const auto out = F4DRP::Discord::clampToDiscordLimits(p);
    REQUIRE(out.details.size() == F4DRP::Constants::kDiscordDetailsMax);
    REQUIRE(out.state.size() == F4DRP::Constants::kDiscordStateMax);
    REQUIRE(out.largeImageText.size() == F4DRP::Constants::kDiscordLargeTextMax);
    REQUIRE(out.smallImageText.size() == F4DRP::Constants::kDiscordSmallTextMax);
    REQUIRE(out.hash != 0U);
}

TEST_CASE("hashPresence differs on field changes", "[protocol][format]")
{
    F4DRP::Discord::PresenceState a;
    a.details = "Pip-Boy";
    a.state = "Concord";
    F4DRP::Discord::PresenceState b = a;
    b.state = "Sanctuary";
    REQUIRE(F4DRP::Discord::hashPresence(a) != F4DRP::Discord::hashPresence(b));
}

TEST_CASE("buildHandshakePayload contains v=1 + client_id", "[protocol][format]")
{
    const auto j = F4DRP::Discord::buildHandshakePayload("12345678901234567");
    REQUIRE(j["v"].get<int>() == 1);
    REQUIRE(j["client_id"].get<std::string>() == "12345678901234567");
}

TEST_CASE("buildSetActivityPayload omits empty fields", "[protocol][null-empty]")
{
    F4DRP::Discord::PresenceState p;
    p.details = "Building";
    p.state = "Sanctuary Hills";
    p.largeImageKey = "fallout4";
    const auto j = F4DRP::Discord::buildSetActivityPayload(12345, "nonce-1", p);
    REQUIRE(j["cmd"].get<std::string>() == "SET_ACTIVITY");
    REQUIRE(j["nonce"].get<std::string>() == "nonce-1");
    REQUIRE(j["args"]["pid"].get<int>() == 12345);
    REQUIRE(j["args"]["activity"]["details"].get<std::string>() == "Building");
    REQUIRE(j["args"]["activity"]["state"].get<std::string>() == "Sanctuary Hills");
    REQUIRE(j["args"]["activity"].contains("timestamps") == false);
}

TEST_CASE("buildSetActivityPayload includes timestamps when start>0", "[protocol][boundary][time]")
{
    F4DRP::Discord::PresenceState p;
    p.details = "X";
    p.startTimestampUnix = 1716595200;
    const auto j = F4DRP::Discord::buildSetActivityPayload(1, "n", p);
    REQUIRE(j["args"]["activity"]["timestamps"]["start"].get<std::int64_t>() == 1716595200);
}
