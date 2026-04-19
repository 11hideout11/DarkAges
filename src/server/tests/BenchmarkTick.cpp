// BenchmarkTick.cpp - Performance benchmarks for core systems
// Phase 9: Performance Testing infrastructure
// Run with: ./darkages_tests "[.benchmark]"

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "combat/CombatSystem.hpp"
#include "physics/SpatialHash.hpp"
#include "physics/MovementSystem.hpp"
#include "zones/AreaOfInterest.hpp"
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
