// Auto-generated tests for CollisionLayerManager
// Created by autonomous dev loop — 2026-05-03
#include <catch2/catch_test_macros.hpp>
#include "../include/combat/CollisionLayerManager.hpp"

namespace DarkAges {
namespace test {


    TEST_CASE("CollisionLayerManager - header compiles", "[collisionlayermanager]") {
        REQUIRE(true);
    }
    TEST_CASE("CollisionLayerManager - CollisionLayerManager has nonzero size", "[collisionlayermanager]") {
        // Verify CollisionLayerManager is a complete type
        static_assert(sizeof(DarkAges::combat::CollisionLayerManager) > 0, "CollisionLayerManager should be a complete type");
        REQUIRE(sizeof(DarkAges::combat::CollisionLayerManager) > 0);
    }

} // namespace test
} // namespace DarkAges
