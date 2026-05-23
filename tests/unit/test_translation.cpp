#include <catch2/catch_test_macros.hpp>

#include "Config/Translation.h"
#include "Constants.h"

TEST_CASE("Translation override replaces default", "[translation][format]")
{
    const std::string ini = "[Strings]\ns_T_PipboyMenu=En la Pip-Boy\n";
    auto t = F4DRP::Config::Translation::fromString(ini);
    REQUIRE(t.get(F4DRP::Constants::StringKey::kPipboyMenu, "FALLBACK") == "En la Pip-Boy");
}

TEST_CASE("Translation empty value falls back", "[translation][null-empty]")
{
    const std::string ini = "[Strings]\ns_T_PipboyMenu=\n";
    auto t = F4DRP::Config::Translation::fromString(ini);
    REQUIRE(t.get(F4DRP::Constants::StringKey::kPipboyMenu, "FALLBACK") == "FALLBACK");
}

TEST_CASE("Translation missing key falls back", "[translation][null-empty]")
{
    auto t = F4DRP::Config::Translation::fromString("");
    REQUIRE(t.get(F4DRP::Constants::StringKey::kMainMenu, "DEFAULT") == "DEFAULT");
}

TEST_CASE("Translation accepts Unicode UTF-8 value", "[translation][format]")
{
    const std::string ini = "[Strings]\ns_T_MainMenu=\xe4\xb8\xbb\xe9\x80\x89\xe9\xa1\xb9\n";
    auto t = F4DRP::Config::Translation::fromString(ini);
    REQUIRE(std::string{t.get(F4DRP::Constants::StringKey::kMainMenu, "x")} == "\xe4\xb8\xbb\xe9\x80\x89\xe9\xa1\xb9");
}
