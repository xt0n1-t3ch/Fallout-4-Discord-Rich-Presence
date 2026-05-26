#include "Presence/Template.h"

namespace F4DRP::Presence {
namespace {
    std::string_view trim(std::string_view s) noexcept
    {
        const auto first = s.find_first_not_of(" \t");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = s.find_last_not_of(" \t");
        return s.substr(first, last - first + 1);
    }
} // namespace

void FieldMap::set(std::string_view key, std::string value)
{
    for (auto& item : m_items) {
        if (item.first == key) {
            item.second = std::move(value);
            return;
        }
    }
    m_items.emplace_back(std::string{key}, std::move(value));
}

std::string_view FieldMap::get(std::string_view key) const noexcept
{
    for (const auto& item : m_items) {
        if (item.first == key) {
            return item.second;
        }
    }
    return {};
}

bool FieldMap::has(std::string_view key) const noexcept
{
    for (const auto& item : m_items) {
        if (item.first == key) {
            return true;
        }
    }
    return false;
}

std::string render(std::string_view tmpl, const FieldMap& fields)
{
    std::string out;
    out.reserve(tmpl.size() + 16);

    std::size_t i = 0;
    while (i < tmpl.size()) {
        const char c = tmpl[i];
        if (c == '{') {
            const auto close = tmpl.find('}', i + 1);
            if (close == std::string_view::npos) {
                out.append(tmpl.substr(i));
                break;
            }
            const auto key = tmpl.substr(i + 1, close - i - 1);
            out.append(fields.get(key));
            i = close + 1;
        }
        else {
            out.push_back(c);
            ++i;
        }
    }
    return out;
}

std::string joinNonEmpty(const std::vector<std::string>& parts, std::string_view sep)
{
    std::string out;
    bool first = true;
    for (const auto& part : parts) {
        const auto trimmed = trim(part);
        if (trimmed.empty()) {
            continue;
        }
        if (!first) {
            out.append(sep);
        }
        out.append(trimmed);
        first = false;
    }
    return out;
}
} // namespace F4DRP::Presence
