#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace F4DRP::Presence {
namespace Token {
    inline constexpr std::string_view kName = "name";
    inline constexpr std::string_view kLevel = "level";
    inline constexpr std::string_view kHp = "hp";
    inline constexpr std::string_view kCaps = "caps";
    inline constexpr std::string_view kLocation = "location";
    inline constexpr std::string_view kEnemy = "enemy";
    inline constexpr std::string_view kMenu = "menu";
    inline constexpr std::string_view kEvent = "event";
    inline constexpr std::string_view kPlayTimeHours = "playH";
    inline constexpr std::string_view kPlayTimeMinutes = "playM";
} // namespace Token

class FieldMap
{
public:
    void set(std::string_view key, std::string value);
    [[nodiscard]] std::string_view get(std::string_view key) const noexcept;
    [[nodiscard]] bool has(std::string_view key) const noexcept;

private:
    std::vector<std::pair<std::string, std::string>> m_items;
};

[[nodiscard]] std::string render(std::string_view tmpl, const FieldMap& fields);

[[nodiscard]] std::string joinNonEmpty(const std::vector<std::string>& parts, std::string_view sep);
} // namespace F4DRP::Presence
