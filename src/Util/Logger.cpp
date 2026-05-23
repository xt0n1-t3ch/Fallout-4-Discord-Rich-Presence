#include "Util/Logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include "Constants.h"

namespace F4DRP::Util {
std::shared_ptr<spdlog::logger> Logger::s_logger;

void Logger::install(const std::filesystem::path& logPath, bool debugMode)
{
    std::error_code ec;
    std::filesystem::create_directories(logPath.parent_path(), ec);

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
    auto dbgSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

    spdlog::sinks_init_list sinks{fileSink, dbgSink};
    s_logger = std::make_shared<spdlog::logger>(std::string{Constants::kPluginName}, sinks);
    s_logger->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
    s_logger->set_level(debugMode ? spdlog::level::debug : spdlog::level::info);
    s_logger->flush_on(spdlog::level::warn);
    spdlog::register_logger(s_logger);
}

void Logger::setDebugMode(bool enabled)
{
    if (s_logger) {
        s_logger->set_level(enabled ? spdlog::level::debug : spdlog::level::info);
    }
}

std::shared_ptr<spdlog::logger> Logger::get()
{
    return s_logger ? s_logger : spdlog::default_logger();
}
} // namespace F4DRP::Util
