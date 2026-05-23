#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include <spdlog/spdlog.h>

namespace F4DRP::Util {
class Logger
{
public:
    static void install(const std::filesystem::path& logPath, bool debugMode);
    static void setDebugMode(bool enabled);
    static std::shared_ptr<spdlog::logger> get();

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};
} // namespace F4DRP::Util

#define F4DRP_LOG_INFO(...) ::F4DRP::Util::Logger::get()->info(__VA_ARGS__)
#define F4DRP_LOG_WARN(...) ::F4DRP::Util::Logger::get()->warn(__VA_ARGS__)
#define F4DRP_LOG_ERR(...) ::F4DRP::Util::Logger::get()->error(__VA_ARGS__)
#define F4DRP_LOG_DBG(...) ::F4DRP::Util::Logger::get()->debug(__VA_ARGS__)
