// Auto-generated tests for CooldownState
// Created by autonomous dev loop — 2026-05-03
#include <catch2/catch_test_macros.hpp>
#include "../include/combat/detail/CooldownState.hpp"

namespace DarkAges {
namespace test {


    TEST_CASE("CooldownState - header compiles", "[cooldownstate]") {
        REQUIRE(true);
    }
    TEST_CASE("CooldownState - CooldownState has nonzero size", "[cooldownstate]") {
        // Verify CooldownState is a complete type
        static_assert(sizeof(CooldownState) > 0, "CooldownState should be a complete type");
        REQUIRE(sizeof(CooldownState) > 0);
    }

} // namespace test
} // namespace DarkAges
