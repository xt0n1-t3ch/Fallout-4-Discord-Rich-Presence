#include <catch2/catch_test_macros.hpp>

#include "Presence/Template.h"

using F4DRP::Presence::FieldMap;
using F4DRP::Presence::joinNonEmpty;
using F4DRP::Presence::render;

namespace {
FieldMap sample()
{
    FieldMap f;
    f.set("name", "Lexie");
    f.set("level", "14");
    f.set("hp", "100");
    f.set("location", "Vault 114");
    return f;
}
} // namespace

TEST_CASE("render substitutes known tokens", "[template]")
{
    const auto f = sample();
    REQUIRE(render("{name}", f) == "Lexie");
    REQUIRE(render("LVL:{level}", f) == "LVL:14");
    REQUIRE(render("{name} - LVL {level} - {hp}%", f) == "Lexie - LVL 14 - 100%");
}

TEST_CASE("render drops unknown or empty tokens to nothing", "[template]")
{
    FieldMap f;
    f.set("name", "");
    REQUIRE(render("{name}", f) == "");
    REQUIRE(render("{missing}", f) == "");
    REQUIRE(render("a{missing}b", f) == "ab");
    REQUIRE(render("HP:{hp}%", f) == "HP:%");
}

TEST_CASE("render preserves literal text and unclosed braces", "[template]")
{
    const auto f = sample();
    REQUIRE(render("just text", f) == "just text");
    REQUIRE(render("open {name", f) == "open {name");
    REQUIRE(render("", f) == "");
}

TEST_CASE("render handles adjacent and repeated tokens", "[template]")
{
    const auto f = sample();
    REQUIRE(render("{name}{level}", f) == "Lexie14");
    REQUIRE(render("{name}-{name}", f) == "Lexie-Lexie");
}

TEST_CASE("joinNonEmpty trims, drops empties, joins with separator", "[template]")
{
    REQUIRE(joinNonEmpty({"a", "b", "c"}, " | ") == "a | b | c");
    REQUIRE(joinNonEmpty({"a", "", "c"}, " | ") == "a | c");
    REQUIRE(joinNonEmpty({"", "", ""}, " | ") == "");
    REQUIRE(joinNonEmpty({"only"}, " | ") == "only");
    REQUIRE(joinNonEmpty({}, " | ") == "");
    REQUIRE(joinNonEmpty({"  spaced  ", " x "}, " - ") == "spaced - x");
    REQUIRE(joinNonEmpty({"a", "   ", "b"}, " . ") == "a . b");
}

TEST_CASE("FieldMap set overwrites and get returns empty for missing", "[template]")
{
    FieldMap f;
    f.set("k", "v1");
    f.set("k", "v2");
    REQUIRE(f.get("k") == "v2");
    REQUIRE(f.has("k"));
    REQUIRE_FALSE(f.has("nope"));
    REQUIRE(f.get("nope") == "");
}
