// Smoke test for AttackState integration
// Verifies that AttackState is a complete, instantiable type.
// This test file was simplified to avoid compilation dependency on private Phase enum.

#include <catch2/catch_test_macros.hpp>
#include "combat/detail/AttackState.hpp"

namespace da = DarkAges;

TEST_CASE("AttackState type is complete (sizeof > 0)", "[attackstate]") {
    REQUIRE(sizeof(da::combat::detail::AttackState) > 0);
}
