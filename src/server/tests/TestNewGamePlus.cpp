#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/NewGamePlusSystem.hpp"
#include "ecs/CoreTypes.hpp"

using namespace DarkAges;
using Catch::Approx;

// ============================================================
// NewGamePlusSystem Tests — WP-3.3
// ============================================================

TEST_CASE("NewGamePlusSystem: eligibility — no zones completed", "[ngplus]") {
    PlayerProgression prog;
    // Default: no zones completed
    REQUIRE_FALSE(prog.tutorialComplete);
    REQUIRE_FALSE(prog.arenaComplete);
    REQUIRE_FALSE(prog.bossComplete);

    CHECK_FALSE(NewGamePlusSystem::isEligibleForNGPlus(prog));
}

TEST_CASE("NewGamePlusSystem: eligibility — partial completion", "[ngplus]") {
    PlayerProgression prog;

    SECTION("tutorial only") {
        prog.tutorialComplete = true;
        CHECK_FALSE(NewGamePlusSystem::isEligibleForNGPlus(prog));
    }

    SECTION("tutorial + arena") {
        prog.tutorialComplete = true;
        prog.arenaComplete = true;
        CHECK_FALSE(NewGamePlusSystem::isEligibleForNGPlus(prog));
    }

    SECTION("arena + boss (no tutorial)") {
        prog.arenaComplete = true;
        prog.bossComplete = true;
        CHECK_FALSE(NewGamePlusSystem::isEligibleForNGPlus(prog));
    }
}

TEST_CASE("NewGamePlusSystem: eligibility — all zones completed", "[ngplus]") {
    PlayerProgression prog;
    prog.tutorialComplete = true;
    prog.arenaComplete = true;
    prog.bossComplete = true;

    CHECK(NewGamePlusSystem::isEligibleForNGPlus(prog));
}

TEST_CASE("NewGamePlusSystem: activation — resets progression and increments count", "[ngplus]") {
    PlayerProgression prog;
    prog.tutorialComplete = true;
    prog.arenaComplete = true;
    prog.bossComplete = true;
    prog.highestZoneUnlocked = 100;  // Boss zone

    NewGamePlusSystem::activateNGPlus(prog);

    // NG+ count incremented
    CHECK(prog.ngPlusCount == 1);

    // Zone progression reset
    CHECK_FALSE(prog.tutorialComplete);
    CHECK_FALSE(prog.arenaComplete);
    CHECK_FALSE(prog.bossComplete);
    CHECK(prog.highestZoneUnlocked == 98);  // Back to tutorial
}

TEST_CASE("NewGamePlusSystem: activation — multiple cycles accumulate", "[ngplus]") {
    PlayerProgression prog;
    prog.tutorialComplete = true;
    prog.arenaComplete = true;
    prog.bossComplete = true;

    // First NG+
    NewGamePlusSystem::activateNGPlus(prog);
    CHECK(prog.ngPlusCount == 1);

    // Complete zones again
    prog.tutorialComplete = true;
    prog.arenaComplete = true;
    prog.bossComplete = true;

    // Second NG+
    NewGamePlusSystem::activateNGPlus(prog);
    CHECK(prog.ngPlusCount == 2);
    CHECK_FALSE(prog.tutorialComplete);
    CHECK_FALSE(prog.arenaComplete);
    CHECK_FALSE(prog.bossComplete);
}

TEST_CASE("NewGamePlusSystem: activation — not eligible is no-op", "[ngplus]") {
    PlayerProgression prog;
    prog.tutorialComplete = true;
    // Missing arena and boss

    NewGamePlusSystem::activateNGPlus(prog);

    // No change
    CHECK(prog.ngPlusCount == 0);
    CHECK(prog.tutorialComplete);
    CHECK_FALSE(prog.arenaComplete);
    CHECK_FALSE(prog.bossComplete);
}

TEST_CASE("NewGamePlusSystem: effective difficulty — no NG+ cycles", "[ngplus]") {
    // NG+0 with various base difficulties
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(1.0f, 0) == Approx(1.0f));
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(1.5f, 0) == Approx(1.5f));
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(2.0f, 0) == Approx(2.0f));
}

TEST_CASE("NewGamePlusSystem: effective difficulty — one NG+ cycle", "[ngplus]") {
    // NG+1: 1.0 * (1.0 + 0.1) = 1.1
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(1.0f, 1) == Approx(1.1f));

    // NG+1 on hard mode: 1.5 * 1.1 = 1.65
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(1.5f, 1) == Approx(1.65f));
}

TEST_CASE("NewGamePlusSystem: effective difficulty — multiple NG+ cycles", "[ngplus]") {
    // NG+2: 1.0 * (1.0 + 0.2) = 1.2
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(1.0f, 2) == Approx(1.2f));

    // NG+3: 1.0 * (1.0 + 0.3) = 1.3
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(1.0f, 3) == Approx(1.3f));

    // NG+10: 1.0 * (1.0 + 1.0) = 2.0 (double difficulty)
    CHECK(NewGamePlusSystem::getEffectiveDifficulty(1.0f, 10) == Approx(2.0f));
}

TEST_CASE("NewGamePlusSystem: level offset", "[ngplus]") {
    CHECK(NewGamePlusSystem::getLevelOffset(0) == 0);
    CHECK(NewGamePlusSystem::getLevelOffset(1) == 1);
    CHECK(NewGamePlusSystem::getLevelOffset(5) == 5);
    CHECK(NewGamePlusSystem::getLevelOffset(100) == 100);
}

TEST_CASE("NewGamePlusSystem: XP multiplier", "[ngplus]") {
    // NG+0 → 1.0x XP
    CHECK(NewGamePlusSystem::getXpMultiplier(0) == Approx(1.0f));

    // NG+1 → 1.15x XP
    CHECK(NewGamePlusSystem::getXpMultiplier(1) == Approx(1.15f));

    // NG+2 → 1.30x XP
    CHECK(NewGamePlusSystem::getXpMultiplier(2) == Approx(1.30f));
}

TEST_CASE("NewGamePlusSystem: loot multiplier", "[ngplus]") {
    CHECK(NewGamePlusSystem::getLootMultiplier(0) == Approx(1.0f));
    CHECK(NewGamePlusSystem::getLootMultiplier(1) == Approx(1.05f));
    CHECK(NewGamePlusSystem::getLootMultiplier(10) == Approx(1.50f));
}
