// [INSTRUMENTATION] Compile-time sanity checks for combat state types
// Validates that RecoveryState is a complete, non-zero sized type.
// This is an auto-generated smoke test to catch header/namespace regressions.

#include <catch2/catch_test_macros.hpp>

#include "combat/detail/RecoveryState.hpp"

using namespace DarkAges::combat::detail;

TEST_CASE("RecoveryState header compiles", "[combat][smoke]") {
    // Presence of the type is sufficient — no instantiation needed
    REQUIRE(true);
}

TEST_CASE("RecoveryState has non-zero size", "[combat][smoke]") {
    REQUIRE(sizeof(RecoveryState) > 0);
}

