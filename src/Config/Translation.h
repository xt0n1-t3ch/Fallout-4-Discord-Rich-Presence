#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace F4DRP::Config {
class Translation
{
public:
    static Translation loadOrEmpty(const std::filesystem::path& iniPath);
    static Translation fromString(const std::string& iniContents);
    static bool writeDefault(const std::filesystem::path& iniPath);

    std::string_view get(std::string_view key, std::string_view fallback) const;

private:
    std::unordered_map<std::string, std::string> m_entries;
};
} // namespace F4DRP::Config
