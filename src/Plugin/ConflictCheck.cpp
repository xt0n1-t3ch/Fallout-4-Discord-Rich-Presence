#include "Plugin/ConflictCheck.h"

#include <string>

#include <Windows.h>

#include "Constants.h"

namespace F4DRP::Plugin {
bool isModuleLoaded(std::wstring_view dllName)
{
    std::wstring local{dllName};
    return ::GetModuleHandleW(local.c_str()) != nullptr;
}

bool conflictingPluginLoaded()
{
    std::wstring widened;
    widened.reserve(Constants::kConflictPluginDll.size());
    for (auto c : Constants::kConflictPluginDll) {
        widened.push_back(static_cast<wchar_t>(c));
    }
    return isModuleLoaded(widened);
}
} // namespace F4DRP::Plugin
