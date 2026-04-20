#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ecs/CoreTypes.hpp"
#include "combat/NPCAISystem.hpp"
#include "combat/ExperienceSystem.hpp"
#include "combat/CombatSystem.hpp"
#include "Constants.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using namespace DarkAges;

// ============================================================================
// Helper functions
// ============================================================================

static EntityID createPlayer(Registry& registry, float x, float z) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(x, 0, z)));
    registry.emplace<Velocity>(entity);
    registry.emplace<CombatState>(entity);
    registry.emplace<Mana>(entity);
    registry.emplace<PlayerTag>(entity);
    registry.emplace<PlayerProgression>(entity);
    return entity;
}

static EntityID createNPC(Registry& registry, float x, float z,
                           float aggroRange = 15.0f, float leashRange = 30.0f,
                           float attackRange = 2.0f, uint16_t baseDamage = 10,
                           uint32_t xpReward = 50) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(x, 0, z)));
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<BoundingVolume>(entity);

    CombatState combat;
    combat.health = 1000;
    combat.maxHealth = 1000;
    registry.emplace<CombatState>(entity, combat);

    Mana mana;
    registry.emplace<Mana>(entity, mana);

    registry.emplace<NPCTag>(entity);
    registry.emplace<CollisionLayer>(entity, CollisionLayer::makeNPC());

    NPCAIState ai;
    ai.aggroRange = aggroRange;
    ai.leashRange = leashRange;
    ai.attackRange = attackRange;
    ai.spawnPoint = Position::fromVec3(glm::vec3(x, 0, z));
    registry.emplace<NPCAIState>(entity, ai);

    NPCStats stats;
    stats.baseDamage = baseDamage;
    stats.xpReward = xpReward;
    stats.respawnTimeMs = 10000;
    registry.emplace<NPCStats>(entity, stats);

    return entity;
}

// ============================================================================
// NPC AI System Tests
// ============================================================================

TEST_CASE("NPCAI starts in Idle behavior", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Idle);
    REQUIRE(ai.target == static_cast<EntityID>(entt::null));
}

TEST_CASE("NPCAI detects nearby player and chases", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f, 15.0f);
    auto player = createPlayer(registry, 5.0f, 5.0f);  // ~7m away

    // Tick once — NPC should detect player and switch to Chase
    aiSystem.update(registry, 0);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Chase);
    REQUIRE(ai.target == player);
}

TEST_CASE("NPCAI does not detect player outside aggro range", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f, 10.0f);  // 10m aggro
    auto player = createPlayer(registry, 50.0f, 50.0f);   // ~70m away

    aiSystem.update(registry, 0);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Idle);
    REQUIRE(ai.target == static_cast<EntityID>(entt::null));
}

TEST_CASE("NPCAI chases then attacks when in range", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f, 15.0f, 30.0f, 2.0f);
    auto player = createPlayer(registry, 1.0f, 0.0f);  // 1m away — in attack range

    aiSystem.update(registry, 0);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Attack);
    REQUIRE(ai.target == player);
}

TEST_CASE("NPCAI attacks deal damage to player", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f, 15.0f, 30.0f, 2.0f, 25);
    auto player = createPlayer(registry, 1.0f, 0.0f);

    int16_t initialHealth = registry.get<CombatState>(player).health;

    // Track damage via callback
    bool damageDealt = false;
    aiSystem.setDamageCallback([&](EntityID n, EntityID t, int16_t dmg) {
        damageDealt = true;
        REQUIRE(n == npc);
        REQUIRE(t == player);
        REQUIRE(dmg == 25);
    });

    // Tick to enter attack state
    aiSystem.update(registry, 0);
    REQUIRE(registry.get<NPCAIState>(npc).behavior == NPCBehavior::Attack);

    // Advance time past attack cooldown (1500ms)
    aiSystem.update(registry, 1500);

    REQUIRE(damageDealt);

    // Player health should be reduced
    int16_t newHealth = registry.get<CombatState>(player).health;
    REQUIRE(newHealth < initialHealth);
    REQUIRE(newHealth == initialHealth - 25);
}

TEST_CASE("NPCAI reverts to idle when target dies", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f, 15.0f, 30.0f, 2.0f);
    auto player = createPlayer(registry, 1.0f, 0.0f);

    // Enter chase
    aiSystem.update(registry, 0);
    REQUIRE(registry.get<NPCAIState>(npc).behavior == NPCBehavior::Attack);

    // Kill the player
    registry.get<CombatState>(player).isDead = true;

    // NPC should go back to idle
    aiSystem.update(registry, 0);
    REQUIRE(registry.get<NPCAIState>(npc).behavior == NPCBehavior::Idle);
    REQUIRE(registry.get<NPCAIState>(npc).target == static_cast<EntityID>(entt::null));
}

TEST_CASE("NPCAI chases player who moves away", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f, 15.0f, 30.0f, 2.0f);
    auto player = createPlayer(registry, 1.0f, 0.0f);

    // Enter attack
    aiSystem.update(registry, 0);
    REQUIRE(registry.get<NPCAIState>(npc).behavior == NPCBehavior::Attack);

    // Move player out of attack range
    registry.get<Position>(player) = Position::fromVec3(glm::vec3(10.0f, 0, 0));

    // NPC should switch back to chase
    aiSystem.update(registry, 0);
    REQUIRE(registry.get<NPCAIState>(npc).behavior == NPCBehavior::Chase);
}

TEST_CASE("NPCAI flees at low health", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f, 15.0f, 30.0f, 2.0f);
    auto player = createPlayer(registry, 1.0f, 0.0f);

    // Set NPC health to 15% (below 20% flee threshold)
    auto& combat = registry.get<CombatState>(npc);
    combat.health = 150;  // 150/1000 = 15%
    combat.maxHealth = 1000;

    // NPC should flee instead of attacking
    aiSystem.update(registry, 0);
    REQUIRE(registry.get<NPCAIState>(npc).behavior == NPCBehavior::Flee);
}

TEST_CASE("NPCAI stops at zero velocity in Idle", "[combat][npc_ai]") {
    Registry registry;
    NPCAISystem aiSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f);

    // Give NPC some velocity
    registry.get<Velocity>(npc).dx = 100;
    registry.get<Velocity>(npc).dz = 100;

    aiSystem.update(registry, 0);

    // Idle should zero velocity
    const auto& vel = registry.get<Velocity>(npc);
    REQUIRE(vel.dx == 0);
    REQUIRE(vel.dz == 0);
}

// ============================================================================
// Experience System Tests
// ============================================================================

TEST_CASE("XP formula scales correctly", "[combat][xp]") {
    REQUIRE(ExperienceSystem::xpForLevel(1) == 100);
    REQUIRE(ExperienceSystem::xpForLevel(2) > 100);
    REQUIRE(ExperienceSystem::xpForLevel(5) > ExperienceSystem::xpForLevel(2));
    REQUIRE(ExperienceSystem::xpForLevel(10) > ExperienceSystem::xpForLevel(5));
    // Level 10 should be about 3162
    REQUIRE(ExperienceSystem::xpForLevel(10) == Catch::Approx(3162).margin(5));
}

TEST_CASE("Awarding XP updates player progression", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);
    auto& prog = registry.get<PlayerProgression>(player);
    REQUIRE(prog.currentXP == 0);

    xpSystem.awardXP(registry, player, 50);
    REQUIRE(prog.currentXP == 50);
    REQUIRE(prog.level == 1);
}

TEST_CASE("Awarding enough XP triggers level up", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);
    auto& prog = registry.get<PlayerProgression>(player);

    bool leveledUp = xpSystem.awardXP(registry, player, 100);  // Exactly level 1 requirement
    REQUIRE(leveledUp);
    REQUIRE(prog.level == 2);
    REQUIRE(prog.currentXP == 0);  // Remaining XP after level-up
    REQUIRE(prog.statPoints == 3);  // 3 points per level
}

TEST_CASE("Multiple level-ups from large XP gain", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);

    // Award enough XP for multiple levels
    // Level 1: 100, Level 2: ~283, Level 3: ~520 = ~903 total
    bool leveledUp = xpSystem.awardXP(registry, player, 1000);
    REQUIRE(leveledUp);

    const auto& prog = registry.get<PlayerProgression>(player);
    REQUIRE(prog.level >= 2);  // Should have gained at least 1 level
}

TEST_CASE("XP callback fires on XP gain", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    uint64_t xpGained = 0;
    xpSystem.setXPGainCallback([&](EntityID, uint64_t xp) {
        xpGained = xp;
    });

    auto player = createPlayer(registry, 0.0f, 0.0f);
    xpSystem.awardXP(registry, player, 75);
    REQUIRE(xpGained == 75);
}

TEST_CASE("Level-up callback fires on level up", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    uint32_t newLevel = 0;
    xpSystem.setLevelUpCallback([&](EntityID, uint32_t level) {
        newLevel = level;
    });

    auto player = createPlayer(registry, 0.0f, 0.0f);
    xpSystem.awardXP(registry, player, 100);
    REQUIRE(newLevel == 2);
}

TEST_CASE("Kill XP awards NPC's xpReward to killer", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);
    auto npc = createNPC(registry, 5.0f, 0.0f, 15.0f, 30.0f, 2.0f, 10, 75);

    xpSystem.awardKillXP(registry, player, npc);

    const auto& prog = registry.get<PlayerProgression>(player);
    REQUIRE(prog.currentXP == 75);
}

TEST_CASE("Non-player cannot gain XP", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    auto npc1 = createNPC(registry, 0.0f, 0.0f);
    auto npc2 = createNPC(registry, 5.0f, 0.0f);

    bool result = xpSystem.awardKillXP(registry, npc1, npc2);
    REQUIRE_FALSE(result);
}

// ============================================================================
// Loot System Tests
// ============================================================================

TEST_CASE("Loot generation creates drop entities", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    auto npc = createNPC(registry, 5.0f, 5.0f);
    auto player = createPlayer(registry, 0.0f, 0.0f);

    // Add loot table to NPC
    LootTable table;
    table.entries[0] = {1001, 1, 1, 1.0f};  // 100% drop
    table.entries[1] = {1002, 1, 3, 1.0f};  // 100% drop, 1-3 quantity
    table.count = 2;
    table.goldDropMin = 10.0f;
    table.goldDropMax = 50.0f;
    registry.emplace<LootTable>(npc, table);

    uint32_t dropped = lootSystem.generateLoot(registry, npc, player);
    REQUIRE(dropped >= 2);  // At least the 2 guaranteed items (gold is separate)

    // Count loot entities
    int lootCount = 0;
    registry.view<LootDropData>().each([&](auto, auto&) { ++lootCount; });
    REQUIRE(lootCount >= 2);  // Items + gold
}

TEST_CASE("Loot drops at NPC position", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    auto npc = createNPC(registry, 10.0f, 20.0f);
    auto player = createPlayer(registry, 0.0f, 0.0f);

    LootTable table;
    table.entries[0] = {1001, 1, 1, 1.0f};
    table.count = 1;
    registry.emplace<LootTable>(npc, table);

    lootSystem.generateLoot(registry, npc, player);

    // Find the loot entity
    bool foundLoot = false;
    registry.view<LootDropData, Position>().each([&](auto, auto& data, auto& pos) {
        // Loot should be near (10, 20) with small random offset
        float x = static_cast<float>(pos.x) * Constants::FIXED_TO_FLOAT;
        float z = static_cast<float>(pos.z) * Constants::FIXED_TO_FLOAT;
        if (std::abs(x - 10.0f) < 3.0f && std::abs(z - 20.0f) < 3.0f) {
            foundLoot = true;
        }
    });
    REQUIRE(foundLoot);
}

TEST_CASE("Loot pickup by owner succeeds", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);

    // Create loot entity owned by the player
    auto lootEntity = registry.create();
    registry.emplace<Position>(lootEntity, Position::fromVec3(glm::vec3(1.0f, 0, 0)));
    registry.emplace<LootDropTag>(lootEntity);
    LootDropData data;
    data.itemId = 1001;
    data.quantity = 1;
    data.ownerPlayer = player;
    registry.emplace<LootDropData>(lootEntity, data);

    bool pickedUp = lootSystem.pickupLoot(registry, player, lootEntity);
    REQUIRE(pickedUp);

    // Loot entity should be destroyed
    REQUIRE_FALSE(registry.valid(lootEntity));
}

TEST_CASE("Loot pickup by non-owner fails", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    auto player1 = createPlayer(registry, 0.0f, 0.0f);
    auto player2 = createPlayer(registry, 5.0f, 0.0f);

    auto lootEntity = registry.create();
    registry.emplace<Position>(lootEntity, Position{0, 0, 0, 0});
    registry.emplace<LootDropTag>(lootEntity);
    LootDropData data;
    data.itemId = 1001;
    data.quantity = 1;
    data.ownerPlayer = player1;  // Owned by player 1
    registry.emplace<LootDropData>(lootEntity, data);

    bool pickedUp = lootSystem.pickupLoot(registry, player2, lootEntity);
    REQUIRE_FALSE(pickedUp);
    REQUIRE(registry.valid(lootEntity));  // Still exists
}

TEST_CASE("Loot pickup by anyone when owner is null", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);

    auto lootEntity = registry.create();
    registry.emplace<Position>(lootEntity, Position{0, 0, 0, 0});
    registry.emplace<LootDropTag>(lootEntity);
    LootDropData data;
    data.itemId = 1001;
    data.quantity = 1;
    data.ownerPlayer = entt::null;  // No owner — anyone can pick up
    registry.emplace<LootDropData>(lootEntity, data);

    bool pickedUp = lootSystem.pickupLoot(registry, player, lootEntity);
    REQUIRE(pickedUp);
}

TEST_CASE("Loot callback fires on drop", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    uint32_t callbackItemId = 0;
    uint32_t callbackQuantity = 0;
    lootSystem.setLootDropCallback([&](EntityID, uint32_t itemId, uint32_t quantity, float) {
        callbackItemId = itemId;
        callbackQuantity = quantity;
    });

    auto npc = createNPC(registry, 0.0f, 0.0f);
    auto player = createPlayer(registry, 0.0f, 0.0f);

    LootTable table;
    table.entries[0] = {42, 3, 3, 1.0f};  // Item 42, exactly 3, 100% chance
    table.count = 1;
    registry.emplace<LootTable>(npc, table);

    lootSystem.generateLoot(registry, npc, player);

    REQUIRE(callbackItemId == 42);
    REQUIRE(callbackQuantity == 3);
}

TEST_CASE("Loot callback fires on pickup", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    uint32_t pickedItemId = 0;
    lootSystem.setLootPickupCallback([&](EntityID, uint32_t itemId, uint32_t, float) {
        pickedItemId = itemId;
    });

    auto player = createPlayer(registry, 0.0f, 0.0f);

    auto lootEntity = registry.create();
    registry.emplace<Position>(lootEntity, Position{0, 0, 0, 0});
    registry.emplace<LootDropTag>(lootEntity);
    LootDropData data;
    data.itemId = 99;
    data.quantity = 1;
    data.ownerPlayer = entt::null;
    registry.emplace<LootDropData>(lootEntity, data);

    lootSystem.pickupLoot(registry, player, lootEntity);
    REQUIRE(pickedItemId == 99);
}

TEST_CASE("Loot with 0% drop chance never drops", "[combat][loot]") {
    Registry registry;
    LootSystem lootSystem;

    auto npc = createNPC(registry, 0.0f, 0.0f);
    auto player = createPlayer(registry, 0.0f, 0.0f);

    LootTable table;
    table.entries[0] = {1001, 1, 1, 0.0f};  // 0% drop chance
    table.count = 1;
    registry.emplace<LootTable>(npc, table);

    uint32_t dropped = lootSystem.generateLoot(registry, npc, player);
    REQUIRE(dropped == 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("Full PvE scenario: NPC chases, attacks, player kills, gets XP and loot", "[combat][integration]") {
    Registry registry;
    NPCAISystem aiSystem;
    ExperienceSystem xpSystem;
    LootSystem lootSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);
    auto npc = createNPC(registry, 5.0f, 0.0f, 15.0f, 30.0f, 2.0f, 20, 100);

    // Add loot table
    LootTable table;
    table.entries[0] = {5001, 1, 2, 1.0f};
    table.count = 1;
    registry.emplace<LootTable>(npc, table);

    // 1. NPC detects player and chases
    aiSystem.update(registry, 0);
    REQUIRE(registry.get<NPCAIState>(npc).behavior == NPCBehavior::Chase);

    // 2. Player moves to NPC and kills it
    registry.get<CombatState>(npc).health = 0;
    registry.get<CombatState>(npc).isDead = true;

    // 3. Award XP
    bool leveledUp = xpSystem.awardKillXP(registry, player, npc);
    REQUIRE(leveledUp);  // 100 XP >= 100 threshold → level up
    const auto& prog = registry.get<PlayerProgression>(player);
    REQUIRE(prog.currentXP == 0);   // 100 - 100 = 0 after level-up
    REQUIRE(prog.level == 2);

    // 4. Generate loot
    uint32_t dropped = lootSystem.generateLoot(registry, npc, player);
    REQUIRE(dropped >= 1);

    // 5. Find and pick up loot
    registry.view<LootDropData>().each([&](EntityID lootEnt, auto&) {
        bool picked = lootSystem.pickupLoot(registry, player, lootEnt);
        // Should succeed since we're the killer
    });
}

TEST_CASE("NPC spawns with correct health scaling", "[combat][npc_ai]") {
    Registry registry;

    auto npc1 = createNPC(registry, 0.0f, 0.0f);
    REQUIRE(registry.get<CombatState>(npc1).maxHealth == 1000);

    auto npc5 = createNPC(registry, 10.0f, 0.0f);
    REQUIRE(registry.get<CombatState>(npc5).maxHealth == 1000);
}

TEST_CASE("XP system: player progression persists", "[combat][xp]") {
    Registry registry;
    ExperienceSystem xpSystem;

    auto player = createPlayer(registry, 0.0f, 0.0f);

    // Kill several NPCs
    for (int i = 0; i < 3; ++i) {
        auto npc = createNPC(registry, static_cast<float>(i * 10), 0.0f,
                             15.0f, 30.0f, 2.0f, 10, 40);
        xpSystem.awardKillXP(registry, player, npc);
    }

    const auto& prog = registry.get<PlayerProgression>(player);
    REQUIRE(prog.currentXP == 20);    // 120 - 100 after level-up
    REQUIRE(prog.level == 2);
    REQUIRE(prog.statPoints == 3);
}

TEST_CASE("PlayerProgression component added to spawned players", "[combat][xp]") {
    Registry registry;

    auto player = registry.create();
    registry.emplace<Position>(player, Position{0, 0, 0, 0});
    registry.emplace<Velocity>(player);
    registry.emplace<CombatState>(player);
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);

    const auto& prog = registry.get<PlayerProgression>(player);
    REQUIRE(prog.level == 1);
    REQUIRE(prog.currentXP == 0);
    REQUIRE(prog.statPoints == 0);
}