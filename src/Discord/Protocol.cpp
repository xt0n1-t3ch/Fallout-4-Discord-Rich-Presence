#include "Discord/Protocol.h"

#include <bit>
#include <cstring>
#include <stdexcept>

#include "Constants.h"
#include "Util/Hash.h"

namespace F4DRP::Discord {
namespace {
    std::string truncate(std::string s, std::size_t limit)
    {
        if (s.size() > limit) {
            s.resize(limit);
        }
        return s;
    }
} // namespace

std::vector<std::uint8_t> encodeFrame(Opcode op, std::string_view jsonPayload)
{
    const auto length = static_cast<std::uint32_t>(jsonPayload.size());
    if (jsonPayload.size() > Constants::kPayloadMaxBytes) {
        throw std::length_error("discord payload exceeds limit");
    }
    std::vector<std::uint8_t> out;
    out.resize(Constants::kPayloadHeaderBytes + jsonPayload.size());

    const auto opcode = static_cast<std::uint32_t>(op);
    std::memcpy(out.data(), &opcode, sizeof(opcode));
    std::memcpy(out.data() + sizeof(opcode), &length, sizeof(length));
    std::memcpy(out.data() + Constants::kPayloadHeaderBytes, jsonPayload.data(), jsonPayload.size());
    return out;
}

Frame decodeFrame(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() < Constants::kPayloadHeaderBytes) {
        throw std::length_error("discord frame too short");
    }
    std::uint32_t opcode = 0;
    std::uint32_t length = 0;
    std::memcpy(&opcode, bytes.data(), sizeof(opcode));
    std::memcpy(&length, bytes.data() + sizeof(opcode), sizeof(length));

    if (bytes.size() < Constants::kPayloadHeaderBytes + length) {
        throw std::length_error("discord frame truncated");
    }
    const auto* payloadPtr = reinterpret_cast<const char*>(bytes.data() + Constants::kPayloadHeaderBytes);
    return Frame{static_cast<Opcode>(opcode), std::string{payloadPtr, length}};
}

nlohmann::json buildHandshakePayload(std::string_view appId)
{
    return nlohmann::json{{"v", 1}, {"client_id", std::string{appId}}};
}

nlohmann::json buildSetActivityPayload(int processId, std::string_view nonce, const PresenceState& presence)
{
    nlohmann::json assets = nlohmann::json::object();
    if (!presence.largeImageKey.empty())
        assets["large_image"] = presence.largeImageKey;
    if (!presence.largeImageText.empty())
        assets["large_text"] = presence.largeImageText;
    if (!presence.smallImageKey.empty())
        assets["small_image"] = presence.smallImageKey;
    if (!presence.smallImageText.empty())
        assets["small_text"] = presence.smallImageText;

    nlohmann::json activity = nlohmann::json::object();
    if (!presence.details.empty())
        activity["details"] = presence.details;
    if (!presence.state.empty())
        activity["state"] = presence.state;
    if (!assets.empty())
        activity["assets"] = std::move(assets);
    if (presence.startTimestampUnix > 0) {
        activity["timestamps"] = {{"start", presence.startTimestampUnix}};
    }

    return nlohmann::json{{"cmd", "SET_ACTIVITY"},
                          {"nonce", std::string{nonce}},
                          {"args", {{"pid", processId}, {"activity", std::move(activity)}}}};
}

std::uint64_t hashPresence(const PresenceState& p)
{
    std::string composite;
    composite.reserve(p.details.size() + p.state.size() + p.largeImageKey.size() + p.largeImageText.size() +
                      p.smallImageKey.size() + p.smallImageText.size() + 6);
    composite.append(p.details)
        .append("\x1F")
        .append(p.state)
        .append("\x1F")
        .append(p.largeImageKey)
        .append("\x1F")
        .append(p.largeImageText)
        .append("\x1F")
        .append(p.smallImageKey)
        .append("\x1F")
        .append(p.smallImageText);
    return Util::fnv1a64(composite);
}

PresenceState clampToDiscordLimits(PresenceState p)
{
    p.details = truncate(std::move(p.details), Constants::kDiscordDetailsMax);
    p.state = truncate(std::move(p.state), Constants::kDiscordStateMax);
    p.largeImageText = truncate(std::move(p.largeImageText), Constants::kDiscordLargeTextMax);
    p.smallImageText = truncate(std::move(p.smallImageText), Constants::kDiscordSmallTextMax);
    p.hash = hashPresence(p);
    return p;
}
} // namespace F4DRP::Discord
