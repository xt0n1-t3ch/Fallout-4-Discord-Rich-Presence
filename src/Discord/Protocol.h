#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace F4DRP::Discord {
enum class Opcode : std::uint32_t
{
    Handshake = 0,
    Frame = 1,
    Close = 2,
    Ping = 3,
    Pong = 4
};

struct PresenceState
{
    std::string details;
    std::string state;
    std::string largeImageKey;
    std::string largeImageText;
    std::string smallImageKey;
    std::string smallImageText;
    std::int64_t startTimestampUnix = 0;
    std::uint64_t hash = 0;

    bool operator==(const PresenceState&) const = default;
};

struct Frame
{
    Opcode opcode;
    std::string payload;
};

std::vector<std::uint8_t> encodeFrame(Opcode op, std::string_view jsonPayload);
Frame decodeFrame(std::span<const std::uint8_t> bytes);

nlohmann::json buildHandshakePayload(std::string_view appId);
nlohmann::json buildSetActivityPayload(int processId, std::string_view nonce, const PresenceState& presence);

std::uint64_t hashPresence(const PresenceState& p);
PresenceState clampToDiscordLimits(PresenceState p);
} // namespace F4DRP::Discord
