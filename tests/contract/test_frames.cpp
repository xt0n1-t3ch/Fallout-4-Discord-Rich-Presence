#include <filesystem>
#include <fstream>
#include <sstream>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "Discord/Protocol.h"

namespace {
nlohmann::json readJson(const std::filesystem::path& path)
{
    std::ifstream f{path};
    if (!f.is_open()) {
        throw std::runtime_error("missing golden frame: " + path.string());
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return nlohmann::json::parse(ss.str());
}

std::filesystem::path framesDir()
{
    return std::filesystem::path{__FILE__}.parent_path() / "frames";
}
} // namespace

TEST_CASE("handshake payload matches golden", "[contract][format]")
{
    const auto expected = readJson(framesDir() / "handshake.json");
    const auto actual = F4DRP::Discord::buildHandshakePayload("12345678901234567");
    REQUIRE(actual == expected);
}

TEST_CASE("set_activity minimal payload matches golden", "[contract][format]")
{
    F4DRP::Discord::PresenceState p;
    p.details = "In Pipboy Menu";
    p.state = "Sole Survivor | LVL 12 | HP 85% | 1234 caps";
    p.largeImageKey = "fallout4";
    p.largeImageText = "Fallout 4";
    p.startTimestampUnix = 1716595200;
    const auto expected = readJson(framesDir() / "set_activity_min.json");
    const auto actual = F4DRP::Discord::buildSetActivityPayload(4242, "nonce-0001", p);
    REQUIRE(actual == expected);
}

TEST_CASE("frame wire format round-trip on golden payloads", "[contract][format]")
{
    for (const auto& name : {"handshake.json", "set_activity_min.json"}) {
        const auto path = framesDir() / name;
        std::ifstream f{path};
        std::stringstream ss;
        ss << f.rdbuf();
        const auto raw = nlohmann::json::parse(ss.str()).dump();
        const auto bytes = F4DRP::Discord::encodeFrame(F4DRP::Discord::Opcode::Frame, raw);
        const auto decoded = F4DRP::Discord::decodeFrame(bytes);
        REQUIRE(decoded.payload == raw);
    }
}
