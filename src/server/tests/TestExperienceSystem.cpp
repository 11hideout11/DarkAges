#include <catch2/catch_test_macros.hpp>
#include "combat/ExperienceSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

using namespace DarkAges;

// ============================================================================
// Experience System Tests
// ============================================================================

TEST_CASE("ExperienceSystem xpForLevel formula", "[experience][xp]") {
    // Level 1 -> 100 XP
    CHECK(ExperienceSystem::xpForLevel(0) == 100); // Edge: level 0 treated as 1
    CHECK(ExperienceSystem::xpForLevel(1) == 100);

    // Level 2 -> 100 * 2^1.5 = 282.8... -> 282 (truncated)
    CHECK(ExperienceSystem::xpForLevel(2) == 282);

    // Level 5 -> 100 * 5^1.5 = 1118.0...
    CHECK(ExperienceSystem::xpForLevel(5) == 1118);

    // Level 10 -> 100 * 10^1.5 = 3162.2... -> 3162
    CHECK(ExperienceSystem::xpForLevel(10) == 3162);

    // Verify monotonic increase
    for (uint32_t i = 2; i <= 20; ++i) {
        CHECK(ExperienceSystem::xpForLevel(i) > ExperienceSystem::xpForLevel(i - 1));
    }
}

TEST_CASE("ExperienceSystem awardXP basic", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    // Award 50 XP
    bool leveledUp = xpSys.awardXP(registry, player, 50);
    CHECK_FALSE(leveledUp);

    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.level == 1);
    CHECK(prog.currentXP == 50);
    CHECK(prog.statPoints == 0);
}

TEST_CASE("ExperienceSystem awardXP triggers level up", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    // Award exactly 100 XP (level 1 threshold)
    bool leveledUp = xpSys.awardXP(registry, player, 100);
    CHECK(leveledUp);

    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.level == 2);
    CHECK(prog.currentXP == 0); // XP reset after level up
    CHECK(prog.statPoints == 3); // 3 stat points per level
}

TEST_CASE("ExperienceSystem awardXP multi-level", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    // Award enough XP for multiple levels (level 1: 100, level 2: 282)
    // Total for level 3: 100 + 282 = 382
    bool leveledUp = xpSys.awardXP(registry, player, 400);
    CHECK(leveledUp);

    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.level == 3);
    CHECK(prog.currentXP == 400 - 100 - 282); // 18 XP remaining
    CHECK(prog.statPoints == 6); // 2 levels * 3 points
}

TEST_CASE("ExperienceSystem awardKillXP from NPC", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    auto npc = registry.create();
    NPCStats stats;
    stats.xpReward = 150;
    registry.emplace<NPCStats>(npc, stats);

    bool leveledUp = xpSys.awardKillXP(registry, player, npc);
    CHECK(leveledUp); // 150 > 100, should level up

    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.level == 2);
    CHECK(prog.currentXP == 50); // 150 - 100
}

TEST_CASE("ExperienceSystem awardKillXP ignores non-player killer", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto npc1 = registry.create();
    registry.emplace<NPCTag>(npc1);
    registry.emplace<PlayerProgression>(npc1); // Has progression but not a player

    auto npc2 = registry.create();
    NPCStats stats;
    stats.xpReward = 100;
    registry.emplace<NPCStats>(npc2, stats);

    bool leveledUp = xpSys.awardKillXP(registry, npc1, npc2);
    CHECK_FALSE(leveledUp);
}

TEST_CASE("ExperienceSystem awardKillXP with no NPCStats", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    auto entity = registry.create();
    // No NPCStats component

    bool leveledUp = xpSys.awardKillXP(registry, player, entity);
    CHECK_FALSE(leveledUp);
}

TEST_CASE("ExperienceSystem awardXP with no PlayerProgression", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    // No PlayerProgression

    bool leveledUp = xpSys.awardXP(registry, player, 100);
    CHECK_FALSE(leveledUp);
}

TEST_CASE("ExperienceSystem level up callback fires", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    uint32_t callbackLevel = 0;
    EntityID callbackPlayer = entt::null;
    xpSys.setLevelUpCallback([&](EntityID player, uint32_t level) {
        callbackPlayer = player;
        callbackLevel = level;
    });

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    xpSys.awardXP(registry, player, 100);

    CHECK(callbackLevel == 2);
    CHECK(callbackPlayer == player);
}

TEST_CASE("ExperienceSystem level up callback fires multiple times", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    uint32_t callbackCount = 0;
    xpSys.setLevelUpCallback([&](EntityID, uint32_t) {
        callbackCount++;
    });

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    // Award 400 XP (should level up twice: 1->2->3)
    xpSys.awardXP(registry, player, 400);

    CHECK(callbackCount == 2);
}

TEST_CASE("ExperienceSystem XP gain callback fires", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    uint64_t callbackXP = 0;
    EntityID callbackPlayer = entt::null;
    xpSys.setXPGainCallback([&](EntityID player, uint64_t xp) {
        callbackPlayer = player;
        callbackXP = xp;
    });

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    xpSys.awardXP(registry, player, 75);

    CHECK(callbackXP == 75);
    CHECK(callbackPlayer == player);
}

TEST_CASE("ExperienceSystem XP gain callback fires even without level up", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    uint64_t totalXP = 0;
    xpSys.setXPGainCallback([&](EntityID, uint64_t xp) {
        totalXP += xp;
    });

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    xpSys.awardXP(registry, player, 50);
    xpSys.awardXP(registry, player, 30);

    CHECK(totalXP == 80);
}

TEST_CASE("ExperienceSystem checkAndApplyLevelUp", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    auto& prog = registry.emplace<PlayerProgression>(player);
    prog.currentXP = 99; // Not enough yet

    CHECK_FALSE(xpSys.checkAndApplyLevelUp(registry, player));
    CHECK(prog.level == 1);

    prog.currentXP = 100; // Exactly enough
    CHECK(xpSys.checkAndApplyLevelUp(registry, player));
    CHECK(prog.level == 2);
}

TEST_CASE("ExperienceSystem checkAndApplyLevelUp with no progression", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    // No PlayerProgression

    CHECK_FALSE(xpSys.checkAndApplyLevelUp(registry, player));
}

TEST_CASE("ExperienceSystem stat points accumulate", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    // Level 1 -> 2 (3 points)
    xpSys.awardXP(registry, player, 100);
    // Level 2 -> 3 (3 more points, total 6)
    xpSys.awardXP(registry, player, 282);

    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.level == 3);
    CHECK(prog.statPoints == 6);
}

TEST_CASE("ExperienceSystem progressive level thresholds", "[experience][xp]") {
    entt::registry registry;
    ExperienceSystem xpSys;

    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    auto& prog = registry.emplace<PlayerProgression>(player);

    // Verify that xpToNextLevel updates correctly after each level up
    CHECK(prog.xpToNextLevel == 100);

    xpSys.awardXP(registry, player, 100);
    CHECK(prog.level == 2);
    CHECK(prog.xpToNextLevel == ExperienceSystem::xpForLevel(2));

    xpSys.awardXP(registry, player, 282);
    CHECK(prog.level == 3);
    CHECK(prog.xpToNextLevel == ExperienceSystem::xpForLevel(3));
}

// ============================================================================
// Loot System Tests (bonus - same file)
// ============================================================================

TEST_CASE("LootSystem generateLoot with no LootTable", "[loot][experience]") {
    entt::registry registry;
    LootSystem lootSys;

    auto npc = registry.create();
    registry.emplace<Position>(npc, Position::fromVec3(glm::vec3(0, 0, 0)));

    auto player = registry.create();

    uint32_t dropped = lootSys.generateLoot(registry, npc, player);
    CHECK(dropped == 0);
}

TEST_CASE("LootSystem generateLoot with no Position", "[loot][experience]") {
    entt::registry registry;
    LootSystem lootSys;

    auto npc = registry.create();
    registry.emplace<LootTable>(npc);

    auto player = registry.create();

    uint32_t dropped = lootSys.generateLoot(registry, npc, player);
    CHECK(dropped == 0);
}

TEST_CASE("LootSystem pickupLoot with no loot entity", "[loot][experience]") {
    entt::registry registry;
    LootSystem lootSys;

    auto player = registry.create();
    auto lootEntity = registry.create();
    // No LootDropData

    bool picked = lootSys.pickupLoot(registry, player, lootEntity);
    CHECK_FALSE(picked);
}

TEST_CASE("LootSystem update cleans expired loot", "[loot][experience]") {
    entt::registry registry;
    LootSystem lootSys;

    // Create expired loot
    auto loot = registry.create();
    registry.emplace<Position>(loot, Position::fromVec3(glm::vec3(0, 0, 0)));
    LootDropData data;
    data.itemId = 1;
    data.quantity = 1;
    data.despawnTimeMs = 1000; // Already expired at time 2000
    registry.emplace<LootDropData>(loot, data);
    registry.emplace<LootDropTag>(loot);

    // Update at time 2000 — loot should be despawned
    lootSys.update(registry, 2000);
    // Entity should be destroyed (or marked for destruction)
    CHECK_FALSE(registry.valid(loot));
}

TEST_CASE("LootSystem update keeps unexpired loot", "[loot][experience]") {
    entt::registry registry;
    LootSystem lootSys;

    // Create loot that hasn't expired
    auto loot = registry.create();
    registry.emplace<Position>(loot, Position::fromVec3(glm::vec3(0, 0, 0)));
    LootDropData data;
    data.itemId = 1;
    data.quantity = 1;
    data.despawnTimeMs = 5000; // Expires at time 5000
    registry.emplace<LootDropData>(loot, data);
    registry.emplace<LootDropTag>(loot);

    // Update at time 2000 — loot should remain
    lootSys.update(registry, 2000);
    CHECK(registry.valid(loot));
}
