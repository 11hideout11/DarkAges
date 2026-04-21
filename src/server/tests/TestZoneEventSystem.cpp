// ZoneEventSystem unit tests
// Tests event lifecycle, participation tracking, objectives, rewards, edge cases

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/ZoneEventSystem.hpp"
#include "combat/ExperienceSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "combat/ChatSystem.hpp"
#include "ecs/Components.hpp"

using namespace DarkAges;
using Catch::Approx;

// Helper: create a test player
static EntityID createTestPlayer(Registry& registry, uint64_t playerId = 0,
                                  float gold = 100.0f) {
    auto entity = registry.create();
    registry.emplace<Position>(entity);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);

    PlayerInfo info{};
    info.playerId = playerId != 0 ? playerId : static_cast<uint64_t>(entity);
    info.connectionId = static_cast<uint32_t>(entity) + 1000;
    registry.emplace<PlayerInfo>(entity, info);

    PlayerProgression prog{};
    prog.level = 1;
    registry.emplace<PlayerProgression>(entity, prog);

    Inventory inv{};
    inv.gold = gold;
    registry.emplace<Inventory>(entity, inv);
    registry.emplace<ChatComponent>(entity);

    return entity;
}

// Helper: create a simple world boss event definition
static ZoneEventDefinition makeWorldBossEvent(uint32_t eventId = 1) {
    ZoneEventDefinition def{};
    def.eventId = eventId;
    std::strncpy(def.name, "Dragon Lord", sizeof(def.name) - 1);
    std::strncpy(def.description, "A mighty dragon terrorizes the land", sizeof(def.description) - 1);
    def.type = ZoneEventType::WorldBoss;
    def.cooldownMs = 60000; // 1 minute for testing
    def.spawnX = 100.0f;
    def.spawnZ = 200.0f;
    def.spawnRadius = 30.0f;
    def.requiredPlayers = 1;
    def.reward.xpReward = 1000;
    def.reward.goldReward = 500;
    def.reward.bonusItemId = 42;
    def.reward.bonusItemQuantity = 1;

    // Single phase: kill the boss
    def.phaseCount = 1;
    def.phases[0].phaseId = 0;
    std::strncpy(def.phases[0].name, "Slay the Dragon", sizeof(def.phases[0].name) - 1);
    def.phases[0].durationMs = 120000; // 2 minutes
    def.phases[0].bossNpcArchetypeId = 99;
    def.phases[0].bossLevel = 10;
    def.phases[0].objectiveCount = 1;
    def.phases[0].objectives[0].type = EventObjectiveType::KillBoss;
    def.phases[0].objectives[0].requiredCount = 1;
    std::strncpy(def.phases[0].objectives[0].description, "Slay the Dragon Lord",
                 sizeof(def.phases[0].objectives[0].description) - 1);

    return def;
}

// Helper: create a multi-phase wave defense event
static ZoneEventDefinition makeWaveDefenseEvent(uint32_t eventId = 2) {
    ZoneEventDefinition def{};
    def.eventId = eventId;
    std::strncpy(def.name, "Goblin Invasion", sizeof(def.name) - 1);
    std::strncpy(def.description, "Defend against goblin waves", sizeof(def.description) - 1);
    def.type = ZoneEventType::WaveDefense;
    def.cooldownMs = 30000;
    def.spawnX = 0.0f;
    def.spawnZ = 0.0f;
    def.spawnRadius = 50.0f;
    def.requiredPlayers = 1;
    def.reward.xpReward = 500;
    def.reward.goldReward = 200;

    // Phase 1: Kill 5 goblins
    def.phaseCount = 2;
    def.phases[0].phaseId = 0;
    std::strncpy(def.phases[0].name, "Wave 1", sizeof(def.phases[0].name) - 1);
    def.phases[0].durationMs = 60000;
    def.phases[0].npcArchetypeId = 1; // goblins
    def.phases[0].npcCount = 5;
    def.phases[0].npcLevel = 3;
    def.phases[0].objectiveCount = 1;
    def.phases[0].objectives[0].type = EventObjectiveType::KillNPC;
    def.phases[0].objectives[0].targetId = 1; // goblin archetype
    def.phases[0].objectives[0].requiredCount = 5;

    // Phase 2: Kill goblin chief
    def.phases[1].phaseId = 1;
    std::strncpy(def.phases[1].name, "Goblin Chief", sizeof(def.phases[1].name) - 1);
    def.phases[1].durationMs = 90000;
    def.phases[1].bossNpcArchetypeId = 2; // goblin chief
    def.phases[1].bossLevel = 5;
    def.phases[1].objectiveCount = 1;
    def.phases[1].objectives[0].type = EventObjectiveType::KillBoss;
    def.phases[1].objectives[0].requiredCount = 1;

    return def;
}

// ============================================================================
// TEST: Zone Event Types
// ============================================================================

TEST_CASE("ZoneEventDefinition default construction", "[zones][event]") {
    ZoneEventDefinition def;
    CHECK(def.eventId == 0);
    CHECK(def.type == ZoneEventType::WorldBoss);
    CHECK(def.phaseCount == 0);
    CHECK(def.cooldownMs == 300000);
    CHECK(def.requiredPlayers == 1);
    CHECK(def.maxParticipants == 0);
}

TEST_CASE("ZoneEventObjective default construction", "[zones][event]") {
    ZoneEventObjective obj;
    CHECK(obj.type == EventObjectiveType::KillNPC);
    CHECK(obj.targetId == 0);
    CHECK(obj.requiredCount == 1);
}

TEST_CASE("ZoneEventPhaseDefinition default construction", "[zones][event]") {
    ZoneEventPhaseDefinition phase;
    CHECK(phase.phaseId == 0);
    CHECK(phase.durationMs == 0);
    CHECK(phase.npcArchetypeId == 0);
    CHECK(phase.npcCount == 0);
    CHECK(phase.bossNpcArchetypeId == 0);
    CHECK(phase.objectiveCount == 0);
}

TEST_CASE("ActiveZoneEvent default construction", "[zones][event]") {
    ActiveZoneEvent event;
    CHECK(event.state == ZoneEventState::Inactive);
    CHECK(event.definition == nullptr);
    CHECK(event.currentPhase == 0);
    CHECK(event.participantCount == 0);
    CHECK_FALSE(event.isActive());
    CHECK_FALSE(event.isComplete());
}

TEST_CASE("ActiveZoneEvent reset", "[zones][event]") {
    ActiveZoneEvent event;
    event.state = ZoneEventState::Active;
    event.currentPhase = 3;
    event.participantCount = 5;
    event.bossEntity = static_cast<EntityID>(42);
    event.objectiveProgress[0] = 10;

    event.reset();

    CHECK(event.state == ZoneEventState::Inactive);
    CHECK(event.currentPhase == 0);
    CHECK(event.participantCount == 0);
    CHECK(static_cast<uint32_t>(event.bossEntity) == static_cast<uint32_t>(entt::null));
    CHECK(event.objectiveProgress[0] == 0);
}

TEST_CASE("EventParticipation default construction", "[zones][event]") {
    EventParticipation p;
    CHECK(p.playerId == 0);
    CHECK(p.totalDamage == 0);
    CHECK(p.kills == 0);
    CHECK(p.deaths == 0);
    CHECK_FALSE(p.contributed);
}

TEST_CASE("ZoneEventComponent default construction", "[zones][event]") {
    ZoneEventComponent zec;
    CHECK(zec.activeEventId == 0);
    CHECK_FALSE(zec.inEvent);
    CHECK(zec.joinTimeMs == 0);
}

TEST_CASE("ZoneEventConfig default construction", "[zones][event]") {
    ZoneEventConfig config;
    CHECK(config.announcementDurationMs == 15000);
    CHECK(config.participationRange == 100.0f);
    CHECK(config.minDamageForReward == 100);
    CHECK(config.topContributorBonusMultiplier == 2.0f);
}

// ============================================================================
// TEST: Participant Management
// ============================================================================

TEST_CASE("ActiveZoneEvent findParticipant", "[zones][event]") {
    ActiveZoneEvent event;
    event.addParticipant(100);
    event.addParticipant(200);

    auto* p = event.findParticipant(100);
    REQUIRE(p != nullptr);
    CHECK(p->playerId == 100);

    p = event.findParticipant(200);
    REQUIRE(p != nullptr);
    CHECK(p->playerId == 200);

    CHECK(event.findParticipant(999) == nullptr);
}

TEST_CASE("ActiveZoneEvent addParticipant duplicate", "[zones][event]") {
    ActiveZoneEvent event;
    auto* p1 = event.addParticipant(100);
    auto* p2 = event.addParticipant(100); // Same player

    CHECK(p1 == p2);
    CHECK(event.participantCount == 1);
}

TEST_CASE("ActiveZoneEvent addParticipant max limit", "[zones][event]") {
    ActiveZoneEvent event;
    for (uint32_t i = 0; i < MAX_EVENT_PARTICIPANTS; ++i) {
        REQUIRE(event.addParticipant(i + 1) != nullptr);
    }
    CHECK(event.participantCount == MAX_EVENT_PARTICIPANTS);

    // Adding one more should fail
    CHECK(event.addParticipant(9999) == nullptr);
}

// ============================================================================
// TEST: Event Registration
// ============================================================================

TEST_CASE("ZoneEventSystem register and get event", "[zones][event]") {
    ZoneEventSystem system;
    auto def = makeWorldBossEvent(1);

    system.registerEvent(def);

    const auto* found = system.getEvent(1);
    REQUIRE(found != nullptr);
    CHECK(found->eventId == 1);
    CHECK(std::string(found->name) == "Dragon Lord");
    CHECK(found->type == ZoneEventType::WorldBoss);
    CHECK(found->phaseCount == 1);

    CHECK(system.getEvent(999) == nullptr);
}

TEST_CASE("ZoneEventSystem register multiple events", "[zones][event]") {
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));
    system.registerEvent(makeWaveDefenseEvent(2));

    auto ids = system.getRegisteredEventIds();
    CHECK(ids.size() == 2);
    CHECK(system.getEvent(1) != nullptr);
    CHECK(system.getEvent(2) != nullptr);
}

TEST_CASE("ZoneEventSystem register update existing event", "[zones][event]") {
    ZoneEventSystem system;
    auto def = makeWorldBossEvent(1);
    system.registerEvent(def);

    // Update the event
    def.cooldownMs = 999999;
    system.registerEvent(def);

    const auto* found = system.getEvent(1);
    REQUIRE(found != nullptr);
    CHECK(found->cooldownMs == 999999);

    // Should still only have 1 event
    CHECK(system.getRegisteredEventIds().size() == 1);
}

// ============================================================================
// TEST: Event Lifecycle
// ============================================================================

TEST_CASE("ZoneEventSystem start event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));

    uint32_t time = 1000;
    CHECK(system.startEvent(registry, 1, time));
    CHECK(system.hasActiveEvent());

    const auto& active = system.getActiveEvent();
    CHECK(active.state == ZoneEventState::Announcing);
    CHECK(active.definition != nullptr);
    CHECK(active.definition->eventId == 1);
    CHECK(active.isActive());
}

TEST_CASE("ZoneEventSystem start event fails if already running", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));
    system.registerEvent(makeWaveDefenseEvent(2));

    CHECK(system.startEvent(registry, 1, 1000));
    CHECK_FALSE(system.startEvent(registry, 2, 1000)); // Can't start while another runs
}

TEST_CASE("ZoneEventSystem start nonexistent event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    CHECK_FALSE(system.startEvent(registry, 999, 1000));
    CHECK_FALSE(system.hasActiveEvent());
}

TEST_CASE("ZoneEventSystem event cooldown", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    auto def = makeWorldBossEvent(1);
    def.cooldownMs = 60000; // 1 minute
    system.registerEvent(def);

    // Start and immediately end
    CHECK(system.startEvent(registry, 1, 1000));
    system.endEvent(registry, 2000);

    // Try to start again immediately — should fail (cooldown)
    CHECK_FALSE(system.startEvent(registry, 1, 3000));

    // Try after cooldown — should succeed
    CHECK(system.startEvent(registry, 1, 70000));
}

TEST_CASE("ZoneEventSystem end event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));

    system.startEvent(registry, 1, 1000);
    CHECK(system.hasActiveEvent());

    CHECK(system.endEvent(registry, 2000));
    CHECK_FALSE(system.hasActiveEvent());
}

TEST_CASE("ZoneEventSystem end event when no active event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    CHECK_FALSE(system.endEvent(registry, 1000));
}

// ============================================================================
// TEST: Announcement Phase
// ============================================================================

TEST_CASE("ZoneEventSystem announcement to active transition", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 5000;
    system.setConfig(config);

    auto def = makeWorldBossEvent(1);
    def.cooldownMs = 0;
    system.registerEvent(def);

    uint32_t time = 1000;
    system.startEvent(registry, 1, time);
    CHECK(system.getActiveEvent().state == ZoneEventState::Announcing);

    // Still announcing before duration
    system.update(registry, time + 3000);
    CHECK(system.getActiveEvent().state == ZoneEventState::Announcing);

    // Should transition to active after announcement
    system.update(registry, time + 5000);
    CHECK(system.getActiveEvent().state == ZoneEventState::Active);
}

// ============================================================================
// TEST: Participation Tracking
// ============================================================================

TEST_CASE("ZoneEventSystem join event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);

    auto player = createTestPlayer(registry, 1);

    CHECK(system.joinEvent(registry, player, 2000));
    CHECK(system.isParticipating(1));

    // Check ZoneEventComponent
    REQUIRE(registry.all_of<ZoneEventComponent>(player));
    const auto& zec = registry.get<ZoneEventComponent>(player);
    CHECK(zec.inEvent);
    CHECK(zec.activeEventId == 1);
}

TEST_CASE("ZoneEventSystem join event when no event active", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    auto player = createTestPlayer(registry, 1);

    CHECK_FALSE(system.joinEvent(registry, player, 1000));
}

TEST_CASE("ZoneEventSystem join event max participants", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    auto def = makeWorldBossEvent(1);
    def.maxParticipants = 2;
    system.registerEvent(def);
    system.startEvent(registry, 1, 1000);

    auto p1 = createTestPlayer(registry, 1);
    auto p2 = createTestPlayer(registry, 2);
    auto p3 = createTestPlayer(registry, 3);

    CHECK(system.joinEvent(registry, p1, 2000));
    CHECK(system.joinEvent(registry, p2, 2000));
    CHECK_FALSE(system.joinEvent(registry, p3, 2000)); // Max reached
}

TEST_CASE("ZoneEventSystem leave event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);

    auto player = createTestPlayer(registry, 1);
    system.joinEvent(registry, player, 2000);
    CHECK(system.isParticipating(1));

    CHECK(system.leaveEvent(registry, player));

    // Player component should be cleared
    REQUIRE(registry.all_of<ZoneEventComponent>(player));
    CHECK_FALSE(registry.get<ZoneEventComponent>(player).inEvent);
}

TEST_CASE("ZoneEventSystem leave event when not in event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);

    auto player = createTestPlayer(registry, 1);
    CHECK_FALSE(system.leaveEvent(registry, player));
}

// ============================================================================
// TEST: Damage and Kill Tracking
// ============================================================================

TEST_CASE("ZoneEventSystem onDamageDealt", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000); // Moves to active

    auto player = createTestPlayer(registry, 1);
    system.joinEvent(registry, player, 2000);

    // Simulate damage
    auto dummy = registry.create();
    system.onDamageDealt(registry, player, dummy, 500, 3000);

    const auto* p = system.getParticipation(1);
    REQUIRE(p != nullptr);
    CHECK(p->totalDamage == 500);
    CHECK(p->contributed);
}

TEST_CASE("ZoneEventSystem onDamageDealt tracks multiple hits", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    auto player = createTestPlayer(registry, 1);
    system.joinEvent(registry, player, 2000);

    auto dummy = registry.create();
    system.onDamageDealt(registry, player, dummy, 100, 3000);
    system.onDamageDealt(registry, player, dummy, 200, 3100);
    system.onDamageDealt(registry, player, dummy, 300, 3200);

    const auto* p = system.getParticipation(1);
    REQUIRE(p != nullptr);
    CHECK(p->totalDamage == 600);
}

TEST_CASE("ZoneEventSystem onDamageDealt auto-adds participant", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    // Player didn't explicitly join, but deals damage
    auto player = createTestPlayer(registry, 1);
    auto dummy = registry.create();
    system.onDamageDealt(registry, player, dummy, 500, 2000);

    const auto* p = system.getParticipation(1);
    REQUIRE(p != nullptr);
    CHECK(p->totalDamage == 500);
    CHECK(p->contributed);
}

TEST_CASE("ZoneEventSystem onNPCKilled", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWaveDefenseEvent(2);
    system.registerEvent(def);
    system.startEvent(registry, 2, 1000);
    system.update(registry, 1000);

    auto player = createTestPlayer(registry, 1);
    system.joinEvent(registry, player, 2000);

    auto npc = registry.create();
    registry.emplace<PlayerInfo>(npc); // NPC with no PlayerInfo - doesn't matter, killer has it
    system.onNPCKilled(registry, player, npc, 1, 3000);

    const auto* p = system.getParticipation(1);
    REQUIRE(p != nullptr);
    CHECK(p->kills == 1);
    CHECK(p->contributed);
}

TEST_CASE("ZoneEventSystem onPlayerDied", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    auto player = createTestPlayer(registry, 1);
    system.joinEvent(registry, player, 2000);

    system.onPlayerDied(registry, player, 3000);

    const auto* p = system.getParticipation(1);
    REQUIRE(p != nullptr);
    CHECK(p->deaths == 1);
}

// ============================================================================
// TEST: Objective Progress
// ============================================================================

TEST_CASE("ZoneEventSystem kill objective progress", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWaveDefenseEvent(2);
    // Phase 1 requires killing 5 goblins (archetype 1)
    system.registerEvent(def);
    system.startEvent(registry, 2, 1000);
    system.update(registry, 1000);

    auto player = createTestPlayer(registry, 1);
    auto npc = registry.create();

    // Kill 4 goblins — phase should still be 0
    for (int i = 0; i < 4; ++i) {
        system.onNPCKilled(registry, player, npc, 1, 2000 + i * 100);
    }
    system.update(registry, 3000);
    CHECK(system.getActiveEvent().currentPhase == 0);

    // Kill 5th goblin — phase should advance
    system.onNPCKilled(registry, player, npc, 1, 3500);
    system.update(registry, 4000);
    CHECK(system.getActiveEvent().currentPhase == 1);
}

TEST_CASE("ZoneEventSystem damage objective progress", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWorldBossEvent(1);
    // Override to use damage objective
    def.phases[0].objectiveCount = 1;
    def.phases[0].objectives[0].type = EventObjectiveType::TotalDamage;
    def.phases[0].objectives[0].requiredCount = 1000;
    system.registerEvent(def);

    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    auto player = createTestPlayer(registry, 1);
    auto dummy = registry.create();

    system.onDamageDealt(registry, player, dummy, 500, 2000);
    system.update(registry, 2100);
    CHECK(system.getActiveEvent().currentPhase == 0); // Not enough yet

    system.onDamageDealt(registry, player, dummy, 500, 2200);
    system.update(registry, 2300);
    CHECK(system.getActiveEvent().state == ZoneEventState::Completing); // Done, only 1 phase
}

// ============================================================================
// TEST: Phase Advancement
// ============================================================================

TEST_CASE("ZoneEventSystem phase advancement", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWaveDefenseEvent(2);
    system.registerEvent(def);

    uint32_t time = 1000;
    system.startEvent(registry, 2, time);
    system.update(registry, time);

    // Should be in phase 0
    CHECK(system.getActiveEvent().currentPhase == 0);

    // Complete phase 0 objectives (kill 5 goblins)
    auto player = createTestPlayer(registry, 1);
    auto npc = registry.create();
    for (int i = 0; i < 5; ++i) {
        system.onNPCKilled(registry, player, npc, 1, time + 100 + i * 100);
    }

    // Update to trigger phase advancement
    system.update(registry, time + 1000);
    CHECK(system.getActiveEvent().currentPhase == 1);
}

TEST_CASE("ZoneEventSystem all phases complete ends event", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWorldBossEvent(1);
    system.registerEvent(def);

    uint32_t time = 1000;
    system.startEvent(registry, 1, time);
    system.update(registry, time);

    // Kill the boss
    auto player = createTestPlayer(registry, 1);
    auto boss = registry.create();
    system.joinEvent(registry, player, time);
    system.onNPCKilled(registry, player, boss, 99, time + 500);

    // Update to trigger completion
    system.update(registry, time + 1000);
    // Should be in Completing state (3s duration)
    CHECK(system.getActiveEvent().state == ZoneEventState::Completing);

    // Wait for completing phase to end
    system.update(registry, time + 5000);
    CHECK_FALSE(system.hasActiveEvent());
}

// ============================================================================
// TEST: Phase Timeout
// ============================================================================

TEST_CASE("ZoneEventSystem phase timeout advances phase", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWaveDefenseEvent(2);
    def.phases[0].durationMs = 5000; // 5 second timeout
    system.registerEvent(def);

    uint32_t time = 1000;
    system.startEvent(registry, 2, time);
    system.update(registry, time);

    CHECK(system.getActiveEvent().currentPhase == 0);

    // Phase 0 times out
    system.update(registry, time + 5000);
    CHECK(system.getActiveEvent().currentPhase == 1);
}

// ============================================================================
// TEST: NPC Spawn Callback
// ============================================================================

TEST_CASE("ZoneEventSystem spawn callback fires on phase start", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWaveDefenseEvent(2);
    system.registerEvent(def);

    bool spawnCalled = false;
    float spawnX = 0, spawnZ = 0;
    uint32_t spawnArchetype = 0, spawnCount = 0;
    uint8_t spawnLevel = 0;

    system.setSpawnCallback([&](float x, float z, uint32_t arch, uint8_t level, uint32_t count) {
        spawnCalled = true;
        spawnX = x;
        spawnZ = z;
        spawnArchetype = arch;
        spawnLevel = level;
        spawnCount = count;
    });

    system.startEvent(registry, 2, 1000);
    system.update(registry, 1000); // Moves to active

    CHECK(spawnCalled);
    CHECK(spawnX == 0.0f);
    CHECK(spawnZ == 0.0f);
    CHECK(spawnArchetype == 1);
    CHECK(spawnLevel == 3);
    CHECK(spawnCount == 5);
}

TEST_CASE("ZoneEventSystem boss spawn callback fires on phase start", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWorldBossEvent(1);
    system.registerEvent(def);

    bool bossSpawnCalled = false;
    float bossX = 0, bossZ = 0;
    uint8_t bossLevel = 0;

    system.setBossSpawnCallback([&](float x, float z, uint8_t level) -> EntityID {
        bossSpawnCalled = true;
        bossX = x;
        bossZ = z;
        bossLevel = level;
        return static_cast<EntityID>(42);
    });

    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    CHECK(bossSpawnCalled);
    CHECK(bossX == 100.0f);
    CHECK(bossZ == 200.0f);
    CHECK(bossLevel == 10);
}

// ============================================================================
// TEST: Rewards
// ============================================================================

TEST_CASE("ZoneEventSystem rewards distributed on completion", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    config.topContributorBonusMultiplier = 1.0f; // No bonus for this test
    system.setConfig(config);

    auto def = makeWorldBossEvent(1);
    def.reward.xpReward = 500;
    def.reward.goldReward = 250;
    system.registerEvent(def);

    ExperienceSystem xpSystem;
    ItemSystem itemSystem;
    system.setExperienceSystem(&xpSystem);

    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    auto player = createTestPlayer(registry, 1);
    system.joinEvent(registry, player, 2000);

    // Deal enough damage to contribute
    auto dummy = registry.create();
    system.onDamageDealt(registry, player, dummy, 500, 2000);

    // Complete the event
    auto boss = registry.create();
    system.onNPCKilled(registry, player, boss, 99, 3000);
    system.update(registry, 3000);
    system.update(registry, 6000); // Past completion phase

    // Player should have received gold (100 base + 250 reward)
    const Inventory& inv = registry.get<Inventory>(player);
    CHECK(inv.gold == Approx(100.0f + 250.0f));
}

// ============================================================================
// TEST: State Callbacks
// ============================================================================

TEST_CASE("ZoneEventSystem state change callback", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);
    system.registerEvent(makeWorldBossEvent(1));

    std::vector<ZoneEventState> stateChanges;
    system.setEventStateCallback([&](uint32_t eventId, ZoneEventState state) {
        stateChanges.push_back(state);
    });

    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000); // Announcing -> Active

    CHECK(stateChanges.size() >= 2);
    CHECK(stateChanges[0] == ZoneEventState::Announcing);
    CHECK(stateChanges[1] == ZoneEventState::Active);
}

TEST_CASE("ZoneEventSystem phase change callback", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWaveDefenseEvent(2);
    system.registerEvent(def);

    std::vector<uint32_t> phaseChanges;
    system.setPhaseChangeCallback([&](uint32_t eventId, uint32_t phaseIdx) {
        phaseChanges.push_back(phaseIdx);
    });

    system.startEvent(registry, 2, 1000);
    system.update(registry, 1000);

    // Complete phase 0
    auto player = createTestPlayer(registry, 1);
    auto npc = registry.create();
    for (int i = 0; i < 5; ++i) {
        system.onNPCKilled(registry, player, npc, 1, 2000 + i * 100);
    }
    system.update(registry, 3000);

    CHECK(phaseChanges.size() == 1);
    CHECK(phaseChanges[0] == 1);
}

// ============================================================================
// TEST: Queries
// ============================================================================

TEST_CASE("ZoneEventSystem getCurrentPhaseDef", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);

    auto def = makeWaveDefenseEvent(2);
    system.registerEvent(def);

    CHECK(system.getCurrentPhaseDef() == nullptr);

    system.startEvent(registry, 2, 1000);
    system.update(registry, 1000);

    const auto* phase = system.getCurrentPhaseDef();
    REQUIRE(phase != nullptr);
    CHECK(std::string(phase->name) == "Wave 1");
}

TEST_CASE("ZoneEventSystem hasActiveEvent", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    system.registerEvent(makeWorldBossEvent(1));

    CHECK_FALSE(system.hasActiveEvent());

    system.startEvent(registry, 1, 1000);
    CHECK(system.hasActiveEvent());

    system.endEvent(registry, 2000);
    CHECK_FALSE(system.hasActiveEvent());
}

// ============================================================================
// TEST: Edge Cases
// ============================================================================

TEST_CASE("ZoneEventSystem update when inactive is no-op", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    // Should not crash
    system.update(registry, 1000);
    CHECK_FALSE(system.hasActiveEvent());
}

TEST_CASE("ZoneEventSystem damage when inactive is no-op", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    auto player = createTestPlayer(registry, 1);
    auto dummy = registry.create();

    system.onDamageDealt(registry, player, dummy, 500, 1000);
    CHECK(system.getParticipation(1) == nullptr);
}

TEST_CASE("ZoneEventSystem kill when inactive is no-op", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    auto player = createTestPlayer(registry, 1);
    auto npc = registry.create();

    system.onNPCKilled(registry, player, npc, 1, 1000);
    CHECK(system.getParticipation(1) == nullptr);
}

TEST_CASE("ZoneEventSystem event types enum values", "[zones][event]") {
    CHECK(static_cast<uint8_t>(ZoneEventType::WorldBoss) == 0);
    CHECK(static_cast<uint8_t>(ZoneEventType::WaveDefense) == 1);
    CHECK(static_cast<uint8_t>(ZoneEventType::Territory) == 2);
    CHECK(static_cast<uint8_t>(ZoneEventType::Invasion) == 3);
    CHECK(static_cast<uint8_t>(ZoneEventType::TimedKill) == 4);
}

TEST_CASE("ZoneEventSystem event state enum values", "[zones][event]") {
    CHECK(static_cast<uint8_t>(ZoneEventState::Inactive) == 0);
    CHECK(static_cast<uint8_t>(ZoneEventState::Announcing) == 1);
    CHECK(static_cast<uint8_t>(ZoneEventState::Active) == 2);
    CHECK(static_cast<uint8_t>(ZoneEventState::Completing) == 3);
    CHECK(static_cast<uint8_t>(ZoneEventState::Cooldown) == 4);
}

TEST_CASE("ZoneEventSystem objective type enum values", "[zones][event]") {
    CHECK(static_cast<uint8_t>(EventObjectiveType::KillNPC) == 0);
    CHECK(static_cast<uint8_t>(EventObjectiveType::KillBoss) == 1);
    CHECK(static_cast<uint8_t>(EventObjectiveType::SurviveTime) == 2);
    CHECK(static_cast<uint8_t>(EventObjectiveType::TotalDamage) == 3);
    CHECK(static_cast<uint8_t>(EventObjectiveType::PlayerDeaths) == 4);
    CHECK(static_cast<uint8_t>(EventObjectiveType::ProtectEntity) == 5);
}

TEST_CASE("ZoneEventSystem multi-participant tracking", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    system.setConfig(config);
    system.registerEvent(makeWorldBossEvent(1));
    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    auto p1 = createTestPlayer(registry, 1);
    auto p2 = createTestPlayer(registry, 2);
    auto p3 = createTestPlayer(registry, 3);

    system.joinEvent(registry, p1, 2000);
    system.joinEvent(registry, p2, 2000);
    system.joinEvent(registry, p3, 2000);

    auto dummy = registry.create();
    system.onDamageDealt(registry, p1, dummy, 1000, 3000);
    system.onDamageDealt(registry, p2, dummy, 2000, 3000);
    system.onDamageDealt(registry, p3, dummy, 500, 3000);

    CHECK(system.getActiveEvent().participantCount == 3);
    CHECK(system.getParticipation(1)->totalDamage == 1000);
    CHECK(system.getParticipation(2)->totalDamage == 2000);
    CHECK(system.getParticipation(3)->totalDamage == 500);
}

TEST_CASE("ZoneEventSystem top contributor gets bonus", "[zones][event]") {
    Registry registry;
    ZoneEventSystem system;
    ZoneEventConfig config;
    config.announcementDurationMs = 0;
    config.topContributorBonusMultiplier = 2.0f;
    system.setConfig(config);

    auto def = makeWorldBossEvent(1);
    def.reward.goldReward = 100;
    system.registerEvent(def);

    ItemSystem itemSystem;
    system.setItemSystem(&itemSystem);

    system.startEvent(registry, 1, 1000);
    system.update(registry, 1000);

    auto p1 = createTestPlayer(registry, 1, 0.0f);
    auto p2 = createTestPlayer(registry, 2, 0.0f);

    system.joinEvent(registry, p1, 2000);
    system.joinEvent(registry, p2, 2000);

    auto dummy = registry.create();
    system.onDamageDealt(registry, p1, dummy, 500, 3000);  // Less damage
    system.onDamageDealt(registry, p2, dummy, 1500, 3000); // More damage (top)

    // Complete the event
    auto boss = registry.create();
    system.onNPCKilled(registry, p2, boss, 99, 4000);
    system.update(registry, 4000);
    system.update(registry, 7000);

    // p2 should get double gold (top contributor)
    CHECK(registry.get<Inventory>(p2).gold == Approx(200.0f));
    // p1 should get normal gold
    CHECK(registry.get<Inventory>(p1).gold == Approx(100.0f));
}
