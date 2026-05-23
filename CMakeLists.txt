cmake_minimum_required(VERSION 3.27)
project(f4discordrp_ng
    VERSION 0.1.0
    DESCRIPTION "Discord Rich Presence for Fallout 4 (cross-gen)"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "F4DiscordRP-NG only targets Windows x64.")
endif()

set(F4DRP_TARGET_RUNTIME "NG" CACHE STRING "Target runtime profile (NG or PRE_NG)")
set_property(CACHE F4DRP_TARGET_RUNTIME PROPERTY STRINGS "NG" "PRE_NG")

find_package(spdlog        CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt           CONFIG REQUIRED)

find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h" REQUIRED)

option(F4DRP_BUILD_PLUGIN "Build the F4SE plugin DLL (requires CommonLibF4 NG-fork + Boost + mmio + rapidcsv + VS 2022 toolchain). Default OFF; tests + IPC stack build standalone." OFF)

if(F4DRP_BUILD_PLUGIN)
    find_package(Boost  REQUIRED)
    find_package(mmio   CONFIG REQUIRED)

    include(FetchContent)
    FetchContent_Declare(
        CommonLibF4
        GIT_REPOSITORY https://github.com/alandtse/CommonLibF4.git
        GIT_TAG        master
        GIT_SHALLOW    ON
        SOURCE_SUBDIR  CommonLibF4
    )
    FetchContent_MakeAvailable(CommonLibF4)
endif()

if(NOT F4DRP_BUILD_PLUGIN)
    message(STATUS "F4DRP_BUILD_PLUGIN=OFF — skipping DLL target, tests-only build")
    enable_testing()
    add_subdirectory(tests)
    return()
endif()

add_library(${PROJECT_NAME} SHARED
    src/Plugin/Main.cpp
    src/Plugin/ConflictCheck.cpp
    src/Discord/Pipe.cpp
    src/Discord/Protocol.cpp
    src/Discord/RateLimiter.cpp
    src/Discord/Client.cpp
    src/Game/MenuTracker.cpp
    src/Game/PlayerSnapshot.cpp
    src/Game/Location.cpp
    src/Game/CombatTracker.cpp
    src/Game/EventTracker.cpp
    src/Config/Settings.cpp
    src/Config/Translation.cpp
    src/Mapper/Mapper.cpp
    src/Util/Logger.cpp
    src/Util/PlayTime.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME discord_rich_presence
    PREFIX ""
    SUFFIX ".dll"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    CommonLibF4::CommonLibF4
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    fmt::fmt
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE _UNICODE
    NOMINMAX WIN32_LEAN_AND_MEAN
    F4DRP_TARGET_RUNTIME_${F4DRP_TARGET_RUNTIME}=1
    _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS
    _SILENCE_ALL_CXX26_DEPRECATION_WARNINGS
    _CRT_SECURE_NO_WARNINGS
)

target_compile_options(${PROJECT_NAME} PRIVATE
    /W4 /WX
    /permissive-
    /Zc:__cplusplus
    /Zc:preprocessor
    /utf-8
    /MP
    /EHsc
    $<$<CONFIG:Release>:/O2>
    $<$<CONFIG:Release>:/Oi>
    $<$<CONFIG:Release>:/GL>
    /wd4267
    /wd4244
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:/LTCG>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
)

option(F4DRP_BUILD_TESTS "Build unit + contract tests" ON)
if(F4DRP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "Data/F4SE/Plugins")
install(FILES
    dist/discord_rich_presence.ini
    dist/discord_rich_presence_translation.ini
    DESTINATION "Data/F4SE/Plugins"
    OPTIONAL)
