#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/ZoneDifficultySystem.hpp"

using namespace DarkAges;
using Catch::Approx;

// ============================================================
// ZoneDifficultySystem Tests
// ============================================================

TEST_CASE("ZoneDifficultySystem: default construction", "[difficulty]") {
    ZoneDifficultySystem system;

    REQUIRE(system.getDifficultyMultiplier() == Approx(1.0f));
    REQUIRE_FALSE(system.isHardMode());
}

TEST_CASE("ZoneDifficultySystem: normal mode (1.0x) passthrough", "[difficulty]") {
    ZoneDifficultySystem system(1.0f);

    REQUIRE_FALSE(system.isHardMode());
    CHECK(system.scaleHealth(1000) == 1000);
    CHECK(system.scaleDamage(50) == 50);
    CHECK(system.scaleXpReward(200) == 200);
    CHECK(system.scaleLevel(5) == 5);
}

TEST_CASE("ZoneDifficultySystem: hard mode (1.5x) scaling", "[difficulty]") {
    ZoneDifficultySystem system(1.5f);

    REQUIRE(system.isHardMode());
    CHECK(system.getDifficultyMultiplier() == Approx(1.5f));

    // Health: 1000 * 1.5 = 1500
    CHECK(system.scaleHealth(1000) == 1500);

    // Damage: 30 * 1.5 = 45
    CHECK(system.scaleDamage(30) == 45);

    // XP: 100 * 1.5 = 150
    CHECK(system.scaleXpReward(100) == 150);

    // Level: 5 + (1.5 - 1.0) * 10 = 5 + 5 = 10
    CHECK(system.scaleLevel(5) == 10);
}

TEST_CASE("ZoneDifficultySystem: hard mode (1.5x) rounding", "[difficulty]") {
    ZoneDifficultySystem system(1.5f);

    // Odd values - rounding to nearest
    CHECK(system.scaleHealth(1001) == 1502);  // 1501.5 -> 1502
    CHECK(system.scaleDamage(31) == 47);      // 46.5 -> 47
    CHECK(system.scaleXpReward(101) == 152);  // 151.5 -> 152
}

TEST_CASE("ZoneDifficultySystem: nightmare mode (2.0x) scaling", "[difficulty]") {
    ZoneDifficultySystem system(2.0f);

    REQUIRE(system.isHardMode());

    // Health: 1000 * 2.0 = 2000
    CHECK(system.scaleHealth(1000) == 2000);

    // Damage: 30 * 2.0 = 60
    CHECK(system.scaleDamage(30) == 60);

    // XP: 100 * 2.0 = 200
    CHECK(system.scaleXpReward(100) == 200);

    // Level: 5 + (2.0 - 1.0) * 10 = 5 + 10 = 15
    CHECK(system.scaleLevel(5) == 15);
}

TEST_CASE("ZoneDifficultySystem: extreme mode (3.0x) scaling", "[difficulty]") {
    ZoneDifficultySystem system(3.0f);

    CHECK(system.scaleHealth(500) == 1500);
    CHECK(system.scaleDamage(20) == 60);
    CHECK(system.scaleXpReward(50) == 150);
    CHECK(system.scaleLevel(1) == 21);  // 1 + (3-1)*10 = 21
}

TEST_CASE("ZoneDifficultySystem: level scaling clamps to 255", "[difficulty]") {
    ZoneDifficultySystem system(2.5f);

    // 250 + (2.5 - 1.0) * 10 = 250 + 15 = 265 -> clamp to 255
    CHECK(system.scaleLevel(250) == 255);
}

TEST_CASE("ZoneDifficultySystem: setDifficultyMultiplier at runtime", "[difficulty]") {
    ZoneDifficultySystem system;

    system.setDifficultyMultiplier(1.5f);
    REQUIRE(system.isHardMode());
    CHECK(system.scaleHealth(1000) == 1500);

    system.setDifficultyMultiplier(1.0f);
    REQUIRE_FALSE(system.isHardMode());
    CHECK(system.scaleHealth(1000) == 1000);
}

TEST_CASE("ZoneDifficultySystem: very low base values", "[difficulty]") {
    ZoneDifficultySystem system(1.5f);

    // Even with 1 HP, scaling applies
    CHECK(system.scaleHealth(1) == 2);    // 1 * 1.5 = 1.5 -> 2
    CHECK(system.scaleDamage(1) == 2);    // 1 * 1.5 = 1.5 -> 2
    CHECK(system.scaleXpReward(1) == 2);  // 1 * 1.5 = 1.5 -> 2

    // Level 1 -> 1 + 5 = 6
    CHECK(system.scaleLevel(1) == 6);
}

TEST_CASE("ZoneDifficultySystem: zero base values", "[difficulty]") {
    ZoneDifficultySystem system(1.5f);

    CHECK(system.scaleHealth(0) == 0);
    CHECK(system.scaleDamage(0) == 0);
    CHECK(system.scaleXpReward(0) == 0);
    CHECK(system.scaleLevel(0) == 5);  // 0 + (1.5-1)*10 = 5
}
