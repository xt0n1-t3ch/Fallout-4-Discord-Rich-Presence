#pragma once

#include <cstdint>
#include <string_view>

namespace F4DRP::Util {
constexpr std::uint64_t fnv1a64(std::string_view bytes) noexcept
{
    constexpr std::uint64_t kOffset = 1469598103934665603ULL;
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    std::uint64_t h = kOffset;
    for (auto c : bytes) {
        h ^= static_cast<std::uint8_t>(c);
        h *= kPrime;
    }
    return h;
}
} // namespace F4DRP::Util
