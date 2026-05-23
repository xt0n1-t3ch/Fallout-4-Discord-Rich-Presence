#include <catch2/catch_test_macros.hpp>

#include "Util/Hash.h"

TEST_CASE("fnv1a64 empty string returns offset basis", "[hash][null-empty]")
{
    REQUIRE(F4DRP::Util::fnv1a64("") == 0xCBF29CE484222325ULL);
}

TEST_CASE("fnv1a64 differs across distinct inputs", "[hash][format]")
{
    const auto a = F4DRP::Util::fnv1a64("Pip-Boy");
    const auto b = F4DRP::Util::fnv1a64("Workshop");
    REQUIRE(a != b);
}

TEST_CASE("fnv1a64 known value 'foobar'", "[hash][format]")
{
    REQUIRE(F4DRP::Util::fnv1a64("foobar") == 0x85944171F73967E8ULL);
}
