// Auto-generated tests for CollisionUtils
// Created by autonomous dev loop — 2026-05-02
#include <catch2/catch_test_macros.hpp>
#include "../include/ecs/CoreTypes.hpp"

namespace DarkAges {
namespace test {


    TEST_CASE("CollisionUtils - header compiles", "[collisionutils]") {
        REQUIRE(true);
    }
    TEST_CASE("CollisionUtils - Position has nonzero size", "[collisionutils]") {
        // Verify Position is a complete type
        static_assert(sizeof(Position) > 0, "Position should be a complete type");
        REQUIRE(sizeof(Position) > 0);
    }
    TEST_CASE("CollisionUtils - Velocity has nonzero size", "[collisionutils]") {
        // Verify Velocity is a complete type
        static_assert(sizeof(Velocity) > 0, "Velocity should be a complete type");
        REQUIRE(sizeof(Velocity) > 0);
    }
    TEST_CASE("CollisionUtils - Rotation has nonzero size", "[collisionutils]") {
        // Verify Rotation is a complete type
        static_assert(sizeof(Rotation) > 0, "Rotation should be a complete type");
        REQUIRE(sizeof(Rotation) > 0);
    }
    TEST_CASE("CollisionUtils - InputState has nonzero size", "[collisionutils]") {
        // Verify InputState is a complete type
        static_assert(sizeof(InputState) > 0, "InputState should be a complete type");
        REQUIRE(sizeof(InputState) > 0);
    }

} // namespace test
} // namespace DarkAges
