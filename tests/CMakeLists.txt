find_package(Catch2 CONFIG REQUIRED)

add_executable(f4drp_tests
    unit/test_hash.cpp
    unit/test_playtime.cpp
    unit/test_protocol.cpp
    unit/test_ratelimiter.cpp
    unit/test_settings.cpp
    unit/test_translation.cpp
    unit/test_menucatalog.cpp
    unit/test_dispatch.cpp
    unit/test_template.cpp
    unit/test_composer.cpp
    unit/test_presenceconfig.cpp
    contract/test_frames.cpp
)

target_include_directories(f4drp_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${SIMPLEINI_INCLUDE_DIRS}
)

target_sources(f4drp_tests PRIVATE
    ../src/Discord/Protocol.cpp
    ../src/Discord/RateLimiter.cpp
    ../src/Config/Settings.cpp
    ../src/Config/Translation.cpp
    ../src/Util/PlayTime.cpp
    ../src/Util/Logger.cpp
    ../src/Presence/Template.cpp
    ../src/Presence/Composer.cpp
    ../src/Presence/PresenceConfig.cpp
)

target_compile_definitions(f4drp_tests PRIVATE
    F4DRP_TESTS=1
    UNICODE _UNICODE
    NOMINMAX WIN32_LEAN_AND_MEAN
)

target_link_libraries(f4drp_tests PRIVATE
    Catch2::Catch2WithMain
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    fmt::fmt
)

target_compile_options(f4drp_tests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc)

include(Catch)
catch_discover_tests(f4drp_tests)

option(F4DRP_LIVE_DISCORD "Build live Discord integration target (network gated at runtime by F4DRP_LIVE_DISCORD_NETWORK env var)" ON)
if(F4DRP_LIVE_DISCORD)
    add_executable(f4drp_integration
        integration/test_pipe.cpp
        ../src/Discord/Pipe.cpp
        ../src/Discord/Protocol.cpp
        ../src/Discord/RateLimiter.cpp
        ../src/Discord/Client.cpp
        ../src/Util/Logger.cpp
    )
    target_include_directories(f4drp_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_compile_definitions(f4drp_integration PRIVATE
        F4DRP_TESTS=1 F4DRP_LIVE=1
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
    )
    target_link_libraries(f4drp_integration PRIVATE
        Catch2::Catch2WithMain spdlog::spdlog
        nlohmann_json::nlohmann_json fmt::fmt
    )
    target_compile_options(f4drp_integration PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc)
    catch_discover_tests(f4drp_integration)
endif()
