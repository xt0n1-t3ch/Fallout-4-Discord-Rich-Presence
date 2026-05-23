#pragma once

#include <string_view>

namespace F4DRP::Plugin {
bool isModuleLoaded(std::wstring_view dllName);
bool conflictingPluginLoaded();
} // namespace F4DRP::Plugin
