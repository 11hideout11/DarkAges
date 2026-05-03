// [COMBAT_AGENT] Integration tests for server-side Combat FSM state transitions
// Tests verify that the CombatSystem's polymorphic state machine transitions correctly
// across network-driven tick boundaries.

#include <catch2/catch_test_macros.hpp>
#include "combat/CombatSystem.hpp"
#include "combat/detail/State.hpp"
#include "combat/detail/IdleState.hpp"
#include "combat/detail/AttackState.hpp"
#include "combat/detail/CooldownState.hpp"
#include "combat/detail/RecoveryState.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <string>
#include <iostream>

namespace da = DarkAges;

// Helper: create entity with required components for combat FSM
static da::EntityID create_combat_entity(da::Registry& registry, int16_t health = 10000) {
    da::EntityID e = registry.create();
    da::CombatState& cs = registry.emplace<da::CombatState>(e);
    cs.health = health;
    cs.maxHealth = 10000;
    cs.teamId = 0;
    cs.classType = 0;
    cs.isDead = false;
    cs.lastAttackTime = 0;
    cs.lastGlobalCooldownTime = 0;
    registry.emplace<da::Position>(e, da::Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<da::Rotation>(e);
    return e;
}

TEST_CASE("Combat FSM: Idle -> Attack -> Cooldown -> Recovery -> Idle cycle", "[combat][fsm][integration]") {
    da::Registry registry;
    da::CombatSystem combat;  // default config: attackCooldownMs = 500ms (current)
    constexpr float tickSec = 0.016f;  // 60Hz
    uint32_t currentTimeMs = 1000;

    da::EntityID entity = create_combat_entity(registry);
    da::EntityID target = create_combat_entity(registry);  // dummy target for melee
    // Position target within melee range (1.5m) of attacker
    {
        auto& pos = registry.get<da::Position>(target);
        pos = da::Position::fromVec3(glm::vec3(0, 0, 1), 0);
    }
    da::CombatState& cs = registry.get<da::CombatState>(entity);

    // 1. First update establishes initial state (CombatState starts with null currentState;
    //    CombatSystem::updateFSM lazily constructs IdleState on first tick)
    combat.updateFSM(registry, tickSec, currentTimeMs);
    currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
    REQUIRE(cs.currentState != nullptr);
    REQUIRE(std::string(cs.currentState->Name()) == "Idle");

    // 2. Trigger an attack via processAttack (sets lastAttackTime and lastGlobalCooldownTime)
    da::AttackInput input;
    input.type = da::AttackInput::MELEE;
    da::HitResult result = combat.processAttack(registry, entity, input, currentTimeMs);
    REQUIRE(result.hit == true);
    REQUIRE(cs.lastAttackTime == currentTimeMs);
    REQUIRE(cs.lastGlobalCooldownTime == currentTimeMs);

    // Next tick: CombatSystem::updateFSM calls GetNextState(Idle) -> AttackState
    combat.updateFSM(registry, tickSec, currentTimeMs);
    currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
    REQUIRE(std::string(cs.currentState->Name()) == "Attack");

    // 3. Advance until AttackState finishes -> CooldownState
    //    AttackState: windup 100ms + active 33ms ≈ 133ms total → ~8 ticks at 60Hz
    bool saw_cooldown = false;
    for (int i = 0; i < 30; ++i) {
        combat.updateFSM(registry, tickSec, currentTimeMs);
        currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
        if (std::string(cs.currentState->Name()) == "Cooldown") {
            saw_cooldown = true;
            break;
        }
    }
    REQUIRE(saw_cooldown);
    REQUIRE(std::string(cs.currentState->Name()) == "Cooldown");

    // 4. CooldownState -> RecoveryState after attackCooldownMs (default 500ms) elapses from lastAttackTime
    //    500ms / 16ms ≈ 31 ticks; pad to 50
    saw_cooldown = false;
    for (int i = 0; i < 50; ++i) {
        combat.updateFSM(registry, tickSec, currentTimeMs);
        currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
        if (std::string(cs.currentState->Name()) == "Recovery") {
            saw_cooldown = true;
            break;
        }
    }
    REQUIRE(saw_cooldown);
    REQUIRE(std::string(cs.currentState->Name()) == "Recovery");

    // 5. RecoveryState -> IdleState after 0.5s recovery duration
    //    500ms / 16ms ≈ 31 ticks
    bool saw_idle = false;
    for (int i = 0; i < 50; ++i) {
        combat.updateFSM(registry, tickSec, currentTimeMs);
        currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
        if (std::string(cs.currentState->Name()) == "Idle") {
            saw_idle = true;
            break;
        }
    }
    REQUIRE(saw_idle);
    REQUIRE(std::string(cs.currentState->Name()) == "Idle");
}

TEST_CASE("Combat FSM: processAttack blocks during cooldown and respects GCD", "[combat][fsm][network]") {
    da::Registry registry;
    da::CombatSystem combat;
    constexpr float tickSec = 0.016f;
    uint32_t currentTimeMs = 1000;

    da::EntityID entity = create_combat_entity(registry);
    da::EntityID target = create_combat_entity(registry);
    {
        auto& pos = registry.get<da::Position>(target);
        pos = da::Position::fromVec3(glm::vec3(0, 0, 1), 0);
    }
    da::CombatState& cs = registry.get<da::CombatState>(entity);

    da::AttackInput input;
    input.type = da::AttackInput::MELEE;

    // First attack succeeds
    da::HitResult result = combat.processAttack(registry, entity, input, currentTimeMs);
    REQUIRE(result.hit == true);
    REQUIRE(cs.lastAttackTime == currentTimeMs);
    REQUIRE(cs.lastGlobalCooldownTime == currentTimeMs);
    REQUIRE(std::string(cs.currentState->Name()) == "Attack");

    // Advance to Cooldown state (Attack completes)
    for (int i = 0; i < 10; ++i) {
        combat.updateFSM(registry, tickSec, currentTimeMs);
        currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
        if (std::string(cs.currentState->Name()) == "Cooldown") break;
    }
    REQUIRE(std::string(cs.currentState->Name()) == "Cooldown");

    // Second attack attempt during cooldown fails
    currentTimeMs += 100;  // only 100ms since last attack
    result = combat.processAttack(registry, entity, input, currentTimeMs);
    REQUIRE(result.hit == false);
    REQUIRE(std::string(result.hitType) == "cooldown");
    REQUIRE(std::string(cs.currentState->Name()) == "Cooldown");

    // Advance ticks until cooldown completes (~500ms total from first attack)
    // We are at ~1000 + ~160 + 100 = ~1260ms; first attack at 1000 → cooldown done at 1500ms
    // Need ~240ms → 15 ticks
    for (int i = 0; i < 30; ++i) {
        combat.updateFSM(registry, tickSec, currentTimeMs);
        currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
        if (std::string(cs.currentState->Name()) == "Recovery") break;
    }
    REQUIRE(std::string(cs.currentState->Name()) == "Recovery");

    // After Recovery → Idle, advance time to clear GCD before attacking
    for (int i = 0; i < 60; ++i) {
        combat.updateFSM(registry, tickSec, currentTimeMs);
        currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
        if (std::string(cs.currentState->Name()) == "Idle") break;
    }
    // After reaching Idle, keep advancing ticks to clear global cooldown
    for (int i = 0; i < 25; ++i) {
        combat.updateFSM(registry, tickSec, currentTimeMs);
        currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
    }
    // Debug: print timestamps before attack
    std::cout << "DEBUG: currentTime=" << currentTimeMs
              << " lastAttackTime=" << cs.lastAttackTime
              << " lastGCD=" << cs.lastGlobalCooldownTime << std::endl;
    currentTimeMs += 50;
    result = combat.processAttack(registry, entity, input, currentTimeMs);
    std::cout << "DEBUG: result.hit=" << result.hit
              << " hitType=" << (result.hitType ? result.hitType : "null") << std::endl;
    REQUIRE(result.hit == true);
    REQUIRE(std::string(cs.currentState->Name()) == "Attack");
}

TEST_CASE("Combat FSM: Dead entities cannot attack; death freezes state", "[combat][fsm]") {
    da::Registry registry;
    da::CombatSystem combat;
    constexpr float tickSec = 0.016f;
    uint32_t currentTimeMs = 1000;

    da::EntityID entity = create_combat_entity(registry);
    da::CombatState& cs = registry.get<da::CombatState>(entity);

    // Kill the entity (sets isDead=true, health=0, calls death callback)
    combat.killEntity(registry, entity, da::EntityID{});
    REQUIRE(cs.isDead);
    REQUIRE(cs.health == 0);

    // Attack on dead entity fails
    da::AttackInput input;
    input.type = da::AttackInput::MELEE;
    da::HitResult result = combat.processAttack(registry, entity, input, currentTimeMs);
    REQUIRE(result.hit == false);
    REQUIRE(std::string(result.hitType) == "dead");

    // FSM tick does not revive; currentState remains non-null (state before death)
    combat.updateFSM(registry, tickSec, currentTimeMs);
    currentTimeMs += static_cast<uint32_t>(tickSec * 1000);
    REQUIRE(cs.currentState != nullptr);
}

TEST_CASE("Combat FSM: State type names and construction are valid", "[combat][fsm]") {
    using Idle = da::combat::detail::IdleState;
    using Attack = da::combat::detail::AttackState;
    using Cooldown = da::combat::detail::CooldownState;
    using Recovery = da::combat::detail::RecoveryState;

    Idle idle; Attack atk; Cooldown cd; Recovery rec;
    REQUIRE(std::string(idle.Name()) == "Idle");
    REQUIRE(std::string(atk.Name()) == "Attack");
    REQUIRE(std::string(cd.Name()) == "Cooldown");
    REQUIRE(std::string(rec.Name()) == "Recovery");
}

TEST_CASE("Combat FSM: AttackState update progression yields Finish and transitions to Cooldown", "[combat][fsm]") {
    da::Registry registry;
    // Use explicit timings to bound test runtime
    da::CombatConfig config;
    config.attackWindupMs = 100;
    // Active duration is hardcoded in AttackState (33ms); no config field to override
    da::CombatSystem combat(config);
    da::EntityID entity = create_combat_entity(registry);
    da::CombatState& cs = registry.get<da::CombatState>(entity);

    using Attack = da::combat::detail::AttackState;
    using Cooldown = da::combat::detail::CooldownState;

    // Manually seed AttackState to isolate it from processAttack
    cs.currentState.reset(new Attack());
    cs.currentState->Enter(registry, entity, combat.getConfig());

    uint32_t timeMs = 0;
    da::StateStatus status = da::StateStatus::Continue;
    int iterations = 0;
    while (iterations < 200) {
        status = cs.currentState->Update(registry, entity, 0.016f, timeMs);
        timeMs += 16;
        ++iterations;
        if (status == da::StateStatus::Finish) break;
    }
    REQUIRE(status == da::StateStatus::Finish);

    // GetNextState should return CooldownState (not a base State pointer to same)
    da::State* next = cs.currentState->GetNextState(&cs);
    REQUIRE(next != nullptr);
    // next is a raw pointer owned by the state machine; we just check type
    auto* cooldown = dynamic_cast<Cooldown*>(next);
    REQUIRE(cooldown != nullptr);
}

TEST_CASE("Combat FSM: Integration round-trip — full attack chain with healing", "[combat][fsm][integration]") {
    // End-to-end: attacker attacks target, target dies and respawns, then heals
    da::Registry registry;
    da::CombatSystem combat;
    constexpr float tickSec = 0.016f;
    uint32_t t = 2000;

    // Attacker and target
    da::EntityID attacker = create_combat_entity(registry, 10000);
    da::EntityID target = create_combat_entity(registry, 5000);  // lower HP

    da::CombatState& attackerCS = registry.get<da::CombatState>(attacker);
    da::CombatState& targetCS = registry.get<da::CombatState>(target);

    // Initial state: both Idle
    combat.updateFSM(registry, tickSec, t);
    t += 16;
    REQUIRE(std::string(attackerCS.currentState->Name()) == "Idle");
    REQUIRE(std::string(targetCS.currentState->Name()) == "Idle");

    // Attacker performs melee attack
    da::AttackInput input;
    input.type = da::AttackInput::MELEE;
    da::HitResult hr = combat.performMeleeAttack(registry, attacker, t);
    REQUIRE(hr.hit == true);
    REQUIRE(hr.target == target);
    REQUIRE(hr.damageDealt > 0);
    REQUIRE(targetCS.health < 5000);

    // Run FSM for both entities: attacker goes through Attack->Cooldown->Recovery->Idle
    // Target takes damage but stays in Idle (no attack)
    int ticks = 0;
    while (std::string(attackerCS.currentState->Name()) != "Idle" && ticks < 200) {
        combat.updateFSM(registry, tickSec, t);
        t += 16;
        ++ticks;
    }
    REQUIRE(std::string(attackerCS.currentState->Name()) == "Idle");
    REQUIRE(targetCS.health > 0);  // target still alive after one hit

    // Kill target with two quick attacks
    for (int i = 0; i < 2; ++i) {
        hr = combat.performMeleeAttack(registry, attacker, t);
        t += 16;
    }
    REQUIRE(targetCS.isDead);
    REQUIRE(targetCS.health == 0);

    // Respawn target
    da::Position spawn = da::Position::fromVec3(glm::vec3(10, 5, 10), 0);
    combat.respawnEntity(registry, target, spawn);
    REQUIRE_FALSE(targetCS.isDead);
    REQUIRE(targetCS.health == targetCS.maxHealth);
    auto& pos = registry.get<da::Position>(target);
    REQUIRE(pos.x == spawn.x);
    REQUIRE(pos.z == spawn.z);

    // Heal attacker (full health already, should clamp)
    combat.applyHeal(registry, attacker, 1000);
    REQUIRE(attackerCS.health == attackerCS.maxHealth);
}
