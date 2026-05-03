// BenchmarkTick.cpp - Performance benchmarks for core systems
// Phase 9: Performance Testing infrastructure
// Run with: ./darkages_tests "[.benchmark]"

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "combat/CombatSystem.hpp"
#include "physics/SpatialHash.hpp"
#include "physics/MovementSystem.hpp"
#include "zones/AreaOfInterest.hpp"
#include "combat/NPCAISystem.hpp"
#include "combat/ZoneEventSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <random>

using namespace DarkAges;
using namespace DarkAges::Constants;

static void populateRegistry(Registry& registry, int count) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-500.0f, 500.0f);
    std::uniform_real_distribution<float> velDist(-5.0f, 5.0f);

    for (int i = 0; i < count; i++) {
        EntityID entity = registry.create();
        registry.emplace<Position>(entity, Position::fromVec3(
            glm::vec3(posDist(rng), 0, posDist(rng)), 0));
        registry.emplace<Velocity>(entity, Velocity{Fixed(velDist(rng)), Fixed(0), Fixed(velDist(rng))});
        registry.emplace<CombatState>(entity);
        registry.emplace<Rotation>(entity);
    }
}

TEST_CASE("SpatialHash insert benchmark", "[.benchmark][spatial][performance]") {
    BENCHMARK("spatial_insert_1000") {
        SpatialHash hash(10.0f);
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-500.0f, 500.0f);
        for (int i = 0; i < 1000; i++) {
            Position pos = Position::fromVec3(glm::vec3(dist(rng), 0, dist(rng)), 0);
            hash.insert(static_cast<EntityID>(i), pos);
        }
    };
}

TEST_CASE("SpatialHash query benchmark", "[.benchmark][spatial][performance]") {
    SpatialHash hash(10.0f);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-500.0f, 500.0f);
    for (int i = 0; i < 1000; i++) {
        Position pos = Position::fromVec3(glm::vec3(dist(rng), 0, dist(rng)), 0);
        hash.insert(static_cast<EntityID>(i), pos);
    }

    BENCHMARK("spatial_query_1000_radius50") {
        Position center = Position::fromVec3(glm::vec3(0, 0, 0), 0);
        auto results = hash.query(center, 50.0f);
        (void)results;
    };
}

TEST_CASE("CombatSystem damage benchmark", "[.benchmark][combat][performance]") {
    Registry registry;
    CombatConfig config;
    config.baseMeleeDamage = 1000;
    config.criticalChance = 0;
    CombatSystem combat(config);

    SECTION("Damage 100 entities") {
        populateRegistry(registry, 100);
        auto view = registry.view<CombatState>();
        EntityID attacker = *(view.begin());

        BENCHMARK("combat_damage_100") {
            for (auto entity : view) {
                if (entity != attacker) {
                    bool isCritical = false;
                    auto damage = combat.calculateDamage(registry, attacker, entity, 1000, isCritical);
                    (void)damage;
                }
            }
        };
    }
}

TEST_CASE("MovementSystem benchmark", "[.benchmark][movement][performance]") {
    SECTION("Update 500 entities") {
        BENCHMARK("movement_500") {
            Registry reg;
            populateRegistry(reg, 500);
            MovementSystem movement;
            movement.update(reg, 16.67f);
        };
    }
}

TEST_CASE("AreaOfInterest benchmark", "[.benchmark][aoi][performance]") {
    Registry registry;
    populateRegistry(registry, 200);
    SpatialHash hash(10.0f);

    auto view = registry.view<Position>();
    for (auto entity : view) {
        hash.insert(entity, view.get<Position>(entity));
    }

    BENCHMARK("aoi_query_200_radius200") {
        Position center = Position::fromVec3(glm::vec3(0, 0, 0), 0);
        auto results = hash.query(center, 200.0f);
        (void)results;
    };
}

TEST_CASE("EnTT registry benchmark", "[.benchmark][ecs][performance]") {
    SECTION("Create 1000 entities") {
        BENCHMARK("entt_create_1000") {
            Registry reg;
            populateRegistry(reg, 1000);
        };
    }

    SECTION("Iterate 1000 entities") {
        Registry reg;
        populateRegistry(reg, 1000);

        BENCHMARK("entt_iterate_1000") {
            int count = 0;
            auto v = reg.view<Position, Velocity>();
            for (auto entity : v) {
                auto [pos, vel] = v.get<Position, Velocity>(entity);
                (void)pos;
                (void)vel;
                count++;
            }
            (void)count;
        };
    }

    SECTION("Destroy 1000 entities") {
        BENCHMARK("entt_destroy_1000") {
            Registry reg;
            populateRegistry(reg, 1000);
            reg.clear();
        };
    }
}

// ============================================================================
// NPC AI System benchmarks
// ============================================================================

static void populateWithNPCs(Registry& registry, int count, int players = 5) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-200.0f, 200.0f);
    
    // Create players (chase targets)
    for (int i = 0; i < players; i++) {
        EntityID entity = registry.create();
        registry.emplace<Position>(entity, Position::fromVec3(
            glm::vec3(posDist(rng), 0, posDist(rng)), 0));
        registry.emplace<CombatState>(entity);
        registry.emplace<PlayerComponent>(entity);
        registry.emplace<Rotation>(entity);
    }
    
    // Create NPCs with full AI state
    for (int i = 0; i < count; i++) {
        EntityID entity = registry.create();
        registry.emplace<Position>(entity, Position::fromVec3(
            glm::vec3(posDist(rng), 0, posDist(rng)), 0));
        registry.emplace<CombatState>(entity);
        registry.emplace<NPCTag>(entity);
        registry.emplace<Rotation>(entity);
        
        NPCAIState ai;
        ai.behavior = NPCBehavior::Idle;
        ai.aggroRange = 30.0f;
        ai.behaviorTimerMs = 0;
        ai.spawnPoint = Position::fromVec3(glm::vec3(posDist(rng), 0, posDist(rng)), 0);
        registry.emplace<NPCAIState>(entity, ai);
        
        NPCStats stats;
        stats.level = 5;
        stats.baseDamage = 10;
        stats.archetype = NPCArchetype::Melee;
        registry.emplace<NPCStats>(entity, stats);
        
        Velocity vel{Fixed(0), Fixed(0), Fixed(0)};
        registry.emplace<Velocity>(entity, vel);
    }
}

TEST_CASE("NPCAI System benchmark", "[.benchmark][npca][performance]") {
    SECTION("Update 50 NPCs with 5 players") {
        BENCHMARK_ADVANCED("npca_update_50")(Catch::Benchmark::Chronometer meter) {
            Registry registry;
            populateWithNPCs(registry, 50, 5);
            NPCAISystem npcAI;
            meter.measure([&] {
                npcAI.update(registry, 1000);
            });
        };
    }
    
    SECTION("Update 200 NPCs with 10 players") {
        BENCHMARK_ADVANCED("npca_update_200")(Catch::Benchmark::Chronometer meter) {
            Registry registry;
            populateWithNPCs(registry, 200, 10);
            NPCAISystem npcAI;
            meter.measure([&] {
                npcAI.update(registry, 1000);
            });
        };
    }
}

// ============================================================================
// Zone Event System benchmark
// ============================================================================

TEST_CASE("ZoneEventSystem benchmark", "[.benchmark][zonevent][performance]") {
    SECTION("Register 20 events") {
        BENCHMARK("zonevent_register_20") {
            ZoneEventSystem events;
            for (int i = 0; i < 20; i++) {
                ZoneEventDefinition def;
                def.eventId = i + 1;
                def.type = (i % 2 == 0) ? ZoneEventType::WorldBoss : ZoneEventType::WaveDefense;
                def.maxParticipants = 10;
                def.requiredPlayers = 1;
                def.spawnX = 0.0f;
                def.spawnZ = 0.0f;
                def.spawnRadius = 20.0f;
                events.registerEvent(def);
            }
        };
    }
}

// ============================================================================
// Netcode: entity state serialization benchmark
// ============================================================================

#include "netcode/NetworkManager.hpp"

TEST_CASE("Entity state serialization benchmark", "[.benchmark][netcode][performance]") {
    std::vector<Protocol::EntityStateData> entities;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-200.0f, 200.0f);
    
    for (int i = 0; i < 100; i++) {
        Protocol::EntityStateData state;
        state.entity = EntityID(i + 1);
        state.position.x = Fixed(dist(rng));
        state.position.z = Fixed(dist(rng));
        state.healthPercent = 100;
        state.animState = 0;
        state.entityType = 3; // NPC
        entities.push_back(state);
    }
    
    std::vector<EntityID> removed;
    
    // Construct a baseline (same entities, unchanged)
    std::vector<Protocol::EntityStateData> baseline = entities;
    
    BENCHMARK_ADVANCED("snapshot_serialize_100")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            auto data = Protocol::createFullSnapshot(100, 99, std::span<const Protocol::EntityStateData>(entities));
            return data.size();
        });
    };
    
    BENCHMARK_ADVANCED("delta_serialize_100_unchanged")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            auto data = Protocol::createDeltaSnapshot(101, 100,
                std::span<const Protocol::EntityStateData>(entities),
                std::span<const EntityID>(removed),
                std::span<const Protocol::EntityStateData>(baseline));
            return data.size();
        });
    };
    
    // Modify 50% of entities  
    for (size_t i = 0; i < entities.size(); i += 2) {
        entities[i].position.x += Fixed(10);
        entities[i].healthPercent -= 20;
    }
    
    BENCHMARK_ADVANCED("delta_serialize_100_half_changed")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            auto data = Protocol::createDeltaSnapshot(102, 100,
                std::span<const Protocol::EntityStateData>(entities),
                std::span<const EntityID>(removed),
                std::span<const Protocol::EntityStateData>(baseline));
            return data.size();
        });
    };
}

// ============================================================================
// Full tick simulation (combined systems)
// ============================================================================

TEST_CASE("Full tick benchmark — all systems", "[.benchmark][tick][performance]") {
    BENCHMARK_ADVANCED("full_tick_50_npcs_5_players")(Catch::Benchmark::Chronometer meter) {
        Registry registry;
        populateWithNPCs(registry, 50, 5);
        
        CombatConfig config;
        config.baseMeleeDamage = 100;
        CombatSystem combat(config);
        NPCAISystem npcAI;
        MovementSystem movement;
        uint32_t tick = 0;
        
        meter.measure([&] {
            tick++;
            movement.update(registry, 16.67f);
            npcAI.update(registry, tick * 17);
            // Simulate a few combat calculations
            auto view = registry.view<CombatState>();
            if (!view.empty()) {
                EntityID first = *(view.begin());
                for (auto e : view) {
                    if (e != first) {
                        bool crit = false;
                        combat.calculateDamage(registry, first, e, 100, crit);
                    }
                }
            }
        });
    };
}
