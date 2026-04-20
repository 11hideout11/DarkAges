// Tests for StatusEffectSystem — buffs, debuffs, DoTs, HoTs, crowd control, shields

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/StatusEffectSystem.hpp"
#include "ecs/CoreTypes.hpp"

using namespace DarkAges;

// ============================================================================
// Test Helpers
// ============================================================================

namespace {

Registry createTestRegistry() {
    return Registry{};
}

EntityID createTestEntity(Registry& registry, int16_t health = 10000) {
    auto entity = registry.create();
    registry.emplace<Position>(entity);
    registry.emplace<CombatState>(entity);
    auto* combat = registry.try_get<CombatState>(entity);
    combat->health = health;
    combat->maxHealth = health;
    return entity;
}

StatusEffect makeBuff(uint32_t templateId = 1, uint32_t durationMs = 10000,
                      float modifierPercent = 0.2f, StatType stat = StatType::AttackDamage) {
    StatusEffect e;
    e.templateId = templateId;
    e.type = StatusEffectType::Buff;
    e.statType = stat;
    e.durationMs = durationMs;
    e.modifierPercent = modifierPercent;
    e.maxStacks = 3;
    e.name = "TestBuff";
    e.debuff = false;
    return e;
}

StatusEffect makeDebuff(uint32_t templateId = 10, uint32_t durationMs = 5000,
                        float modifierPercent = -0.3f, StatType stat = StatType::MovementSpeed) {
    StatusEffect e;
    e.templateId = templateId;
    e.type = StatusEffectType::Debuff;
    e.statType = stat;
    e.durationMs = durationMs;
    e.modifierPercent = modifierPercent;
    e.maxStacks = 1;
    e.name = "TestDebuff";
    e.debuff = true;
    return e;
}

StatusEffect makeDoT(uint32_t templateId = 20, uint32_t durationMs = 10000,
                     uint32_t tickIntervalMs = 2000, int16_t tickAmount = -100) {
    StatusEffect e;
    e.templateId = templateId;
    e.type = StatusEffectType::DoT;
    e.durationMs = durationMs;
    e.tickIntervalMs = tickIntervalMs;
    e.tickAmount = tickAmount;
    e.maxStacks = 1;
    e.name = "TestDoT";
    e.debuff = true;
    return e;
}

StatusEffect makeHoT(uint32_t templateId = 30, uint32_t durationMs = 10000,
                     uint32_t tickIntervalMs = 3000, int16_t tickAmount = 50) {
    StatusEffect e;
    e.templateId = templateId;
    e.type = StatusEffectType::HoT;
    e.durationMs = durationMs;
    e.tickIntervalMs = tickIntervalMs;
    e.tickAmount = tickAmount;
    e.maxStacks = 1;
    e.name = "TestHoT";
    e.debuff = false;
    return e;
}

StatusEffect makeStun(uint32_t templateId = 40, uint32_t durationMs = 3000) {
    StatusEffect e;
    e.templateId = templateId;
    e.type = StatusEffectType::Stun;
    e.durationMs = durationMs;
    e.maxStacks = 1;
    e.name = "TestStun";
    e.debuff = true;
    return e;
}

StatusEffect makeShield(int16_t absorb = 500, uint32_t templateId = 50, uint32_t durationMs = 15000) {
    StatusEffect e;
    e.templateId = templateId;
    e.type = StatusEffectType::Shield;
    e.durationMs = durationMs;
    e.shieldAbsorb = absorb;
    e.maxStacks = 1;
    e.name = "TestShield";
    e.debuff = false;
    return e;
}

} // namespace

// ============================================================================
// Basic Application Tests
// ============================================================================

TEST_CASE("StatusEffectSystem apply and query", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("apply effect returns valid ID") {
        uint32_t id = system.applyEffect(registry, entity, makeBuff(), 0);
        REQUIRE(id > 0);
    }

    SECTION("entity has effect after apply") {
        system.applyEffect(registry, entity, makeBuff(), 0);
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));
        REQUIRE_FALSE(system.hasEffect(registry, entity, StatusEffectType::Debuff));
    }

    SECTION("entity has ActiveStatusEffects component after apply") {
        system.applyEffect(registry, entity, makeBuff(), 0);
        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active != nullptr);
        REQUIRE(active->count == 1);
    }

    SECTION("multiple different effects") {
        system.applyEffect(registry, entity, makeBuff(), 0);
        system.applyEffect(registry, entity, makeDebuff(), 0);
        system.applyEffect(registry, entity, makeDoT(), 0);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->count == 3);
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Debuff));
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::DoT));
    }
}

// ============================================================================
// Removal Tests
// ============================================================================

TEST_CASE("StatusEffectSystem removal", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("remove by ID") {
        uint32_t id = system.applyEffect(registry, entity, makeBuff(), 0);
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));

        bool removed = system.removeEffect(registry, entity, id);
        REQUIRE(removed);
        REQUIRE_FALSE(system.hasEffect(registry, entity, StatusEffectType::Buff));
    }

    SECTION("remove by type removes all of that type") {
        system.applyEffect(registry, entity, makeBuff(1), 0);
        system.applyEffect(registry, entity, makeBuff(2), 0);
        system.applyEffect(registry, entity, makeDebuff(), 0);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->count == 3);

        uint8_t removed = system.removeEffectsByType(registry, entity, StatusEffectType::Buff);
        REQUIRE(removed == 2);
        REQUIRE(active->count == 1);
        REQUIRE_FALSE(system.hasEffect(registry, entity, StatusEffectType::Buff));
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Debuff));
    }

    SECTION("remove nonexistent ID returns false") {
        bool removed = system.removeEffect(registry, entity, 9999);
        REQUIRE_FALSE(removed);
    }

    SECTION("remove from entity with no effects returns false") {
        bool removed = system.removeEffect(registry, entity, 1);
        REQUIRE_FALSE(removed);
    }

    SECTION("clear all effects") {
        system.applyEffect(registry, entity, makeBuff(), 0);
        system.applyEffect(registry, entity, makeDebuff(), 0);
        system.applyEffect(registry, entity, makeDoT(), 0);

        system.clearAll(registry, entity);
        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->count == 0);
    }
}

// ============================================================================
// Cleanse Tests
// ============================================================================

TEST_CASE("StatusEffectSystem cleanse", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("cleanse removes only debuffs") {
        system.applyEffect(registry, entity, makeBuff(), 0);
        system.applyEffect(registry, entity, makeDebuff(), 0);
        system.applyEffect(registry, entity, makeDoT(), 0);

        uint8_t cleansed = system.cleanse(registry, entity);
        REQUIRE(cleansed == 2);  // Debuff and DoT

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->count == 1);
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));
        REQUIRE_FALSE(system.hasEffect(registry, entity, StatusEffectType::Debuff));
    }

    SECTION("cleanse on entity with no debuffs removes nothing") {
        system.applyEffect(registry, entity, makeBuff(), 0);

        uint8_t cleansed = system.cleanse(registry, entity);
        REQUIRE(cleansed == 0);
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));
    }
}

// ============================================================================
// Expiration Tests
// ============================================================================

TEST_CASE("StatusEffectSystem expiration", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("effect expires after duration") {
        auto buff = makeBuff(1, 5000);  // 5 second duration
        system.applyEffect(registry, entity, buff, 1000);

        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));

        // At time 5999, not yet expired
        system.update(registry, 5999);
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));

        // At time 6000 (1000 + 5000), expired
        system.update(registry, 6000);
        REQUIRE_FALSE(system.hasEffect(registry, entity, StatusEffectType::Buff));
    }

    SECTION("permanent effect (duration=0) never expires") {
        auto buff = makeBuff(1, 0);  // 0 = permanent
        system.applyEffect(registry, entity, buff, 0);

        system.update(registry, 999999);
        REQUIRE(system.hasEffect(registry, entity, StatusEffectType::Buff));
    }
}

// ============================================================================
// DoT Tests
// ============================================================================

TEST_CASE("StatusEffectSystem DoT", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry, 5000);

    SECTION("DoT deals damage on tick") {
        auto dot = makeDoT(1, 10000, 2000, -100);  // 100 damage every 2s
        system.applyEffect(registry, entity, dot, 0);

        auto* combat = registry.try_get<CombatState>(entity);
        REQUIRE(combat->health == 5000);

        // Tick at 2000ms
        system.update(registry, 2000);
        REQUIRE(combat->health == 4900);

        // Tick at 4000ms
        system.update(registry, 4000);
        REQUIRE(combat->health == 4800);
    }

    SECTION("DoT can kill entity") {
        auto dot = makeDoT(1, 10000, 1000, -2000);  // 2000 damage every 1s
        system.applyEffect(registry, entity, dot, 0);

        auto* combat = registry.try_get<CombatState>(entity);
        system.update(registry, 1000);
        system.update(registry, 2000);
        system.update(registry, 3000);

        REQUIRE(combat->health == 0);
        REQUIRE(combat->isDead);
    }

    SECTION("DoT does not tick before interval") {
        auto dot = makeDoT(1, 10000, 2000, -100);
        system.applyEffect(registry, entity, dot, 0);

        auto* combat = registry.try_get<CombatState>(entity);
        system.update(registry, 1999);  // Not yet 2000ms
        REQUIRE(combat->health == 5000);

        system.update(registry, 2000);  // Now it ticks
        REQUIRE(combat->health == 4900);
    }
}

// ============================================================================
// HoT Tests
// ============================================================================

TEST_CASE("StatusEffectSystem HoT", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry, 5000);

    SECTION("HoT heals on tick") {
        auto* combat = registry.try_get<CombatState>(entity);
        combat->health = 4000;  // Damaged

        auto hot = makeHoT(1, 10000, 3000, 100);  // 100 heal every 3s
        system.applyEffect(registry, entity, hot, 0);

        system.update(registry, 3000);
        REQUIRE(combat->health == 4100);

        system.update(registry, 6000);
        REQUIRE(combat->health == 4200);
    }

    SECTION("HoT does not overheal") {
        auto* combat = registry.try_get<CombatState>(entity);
        combat->health = 4950;

        auto hot = makeHoT(1, 10000, 1000, 200);  // 200 heal every 1s
        system.applyEffect(registry, entity, hot, 0);

        system.update(registry, 1000);
        REQUIRE(combat->health == 5000);  // Capped at max
    }

    SECTION("HoT does not heal dead entities") {
        auto* combat = registry.try_get<CombatState>(entity);
        combat->isDead = true;
        combat->health = 0;

        auto hot = makeHoT(1, 10000, 1000, 100);
        system.applyEffect(registry, entity, hot, 0);

        system.update(registry, 1000);
        REQUIRE(combat->health == 0);
    }
}

// ============================================================================
// Crowd Control Tests
// ============================================================================

TEST_CASE("StatusEffectSystem crowd control", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("stun prevents movement and casting") {
        system.applyEffect(registry, entity, makeStun(), 0);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE_FALSE(active->canMove());
        REQUIRE_FALSE(active->canCastAbilities());
        REQUIRE(active->isCrowdControlled());
    }

    SECTION("root prevents movement but allows casting") {
        StatusEffect root;
        root.type = StatusEffectType::Root;
        root.durationMs = 5000;
        root.name = "Root";
        root.debuff = true;
        system.applyEffect(registry, entity, root, 0);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE_FALSE(active->canMove());
        REQUIRE(active->canCastAbilities());
        REQUIRE(active->isCrowdControlled());
    }

    SECTION("silence allows movement but prevents casting") {
        StatusEffect silence;
        silence.type = StatusEffectType::Silence;
        silence.durationMs = 5000;
        silence.name = "Silence";
        silence.debuff = true;
        system.applyEffect(registry, entity, silence, 0);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->canMove());
        REQUIRE_FALSE(active->canCastAbilities());
        REQUIRE_FALSE(active->isCrowdControlled());
    }

    SECTION("CC expires correctly") {
        system.applyEffect(registry, entity, makeStun(1, 3000), 1000);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->computedMods.stunned);

        system.update(registry, 4000);  // 1000 + 3000 = expired
        REQUIRE_FALSE(active->computedMods.stunned);
        REQUIRE(active->canMove());
    }
}

// ============================================================================
// Shield Tests
// ============================================================================

TEST_CASE("StatusEffectSystem shields", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("shield absorbs damage") {
        system.applyEffect(registry, entity, makeShield(500), 0);

        int16_t remaining = system.absorbDamage(registry, entity, 300);
        REQUIRE(remaining == 0);  // All absorbed

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->computedMods.shieldRemaining == 200);
    }

    SECTION("shield is depleted and removed") {
        system.applyEffect(registry, entity, makeShield(500), 0);

        int16_t remaining = system.absorbDamage(registry, entity, 600);
        REQUIRE(remaining == 100);  // 500 absorbed, 100 remains

        REQUIRE_FALSE(system.hasEffect(registry, entity, StatusEffectType::Shield));
    }

    SECTION("no shield returns full damage") {
        int16_t remaining = system.absorbDamage(registry, entity, 300);
        REQUIRE(remaining == 300);
    }

    SECTION("zero damage is not absorbed") {
        system.applyEffect(registry, entity, makeShield(500), 0);
        int16_t remaining = system.absorbDamage(registry, entity, 0);
        REQUIRE(remaining == 0);
    }
}

// ============================================================================
// Stacking Tests
// ============================================================================

TEST_CASE("StatusEffectSystem stacking", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("same template stacks up to max") {
        auto buff = makeBuff(1, 10000, 0.2f);  // maxStacks = 3
        buff.source = static_cast<EntityID>(1);

        system.applyEffect(registry, entity, buff, 0);
        system.applyEffect(registry, entity, buff, 0);
        system.applyEffect(registry, entity, buff, 0);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->count == 1);  // Still 1 entry
        REQUIRE(active->effects[0].stacks == 3);
    }

    SECTION("stacking refreshes duration") {
        auto buff = makeBuff(1, 5000, 0.2f);
        buff.source = static_cast<EntityID>(1);

        system.applyEffect(registry, entity, buff, 1000);
        system.applyEffect(registry, entity, buff, 3000);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->effects[0].expiresAtMs == 8000);  // 3000 + 5000
    }

    SECTION("beyond max stacks only refreshes duration") {
        auto buff = makeBuff(1, 5000, 0.2f);
        buff.source = static_cast<EntityID>(1);

        system.applyEffect(registry, entity, buff, 0);
        system.applyEffect(registry, entity, buff, 0);
        system.applyEffect(registry, entity, buff, 0);
        system.applyEffect(registry, entity, buff, 0);  // 4th application

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->count == 1);
        REQUIRE(active->effects[0].stacks == 3);  // Still 3 (max)
    }
}

// ============================================================================
// Stat Modifier Tests
// ============================================================================

TEST_CASE("StatusEffectSystem stat modifiers", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("buff increases stat") {
        system.applyEffect(registry, entity, makeBuff(1, 10000, 0.2f, StatType::AttackDamage), 0);
        system.update(registry, 0);

        auto& mods = system.getModifiers(registry, entity);
        REQUIRE(mods.damageMultiplier == Catch::Approx(1.2f));
    }

    SECTION("debuff decreases stat") {
        system.applyEffect(registry, entity, makeDebuff(2, 10000, -0.3f, StatType::MovementSpeed), 0);
        system.update(registry, 0);

        auto& mods = system.getModifiers(registry, entity);
        REQUIRE(mods.speedMultiplier == Catch::Approx(0.7f));
    }

    SECTION("multiple modifiers multiply") {
        system.applyEffect(registry, entity, makeBuff(1, 10000, 0.2f, StatType::AttackDamage), 0);
        system.applyEffect(registry, entity, makeBuff(2, 10000, 0.1f, StatType::AttackDamage), 0);
        system.update(registry, 0);

        auto& mods = system.getModifiers(registry, entity);
        REQUIRE(mods.damageMultiplier == Catch::Approx(1.2f * 1.1f));
    }

    SECTION("stacks affect modifier") {
        auto buff = makeBuff(1, 10000, 0.1f, StatType::AttackDamage);
        buff.source = static_cast<EntityID>(1);

        system.applyEffect(registry, entity, buff, 0);
        system.applyEffect(registry, entity, buff, 0);
        system.applyEffect(registry, entity, buff, 0);  // 3 stacks
        system.update(registry, 0);

        auto& mods = system.getModifiers(registry, entity);
        // 1.0 + (0.1 * 3) = 1.3
        REQUIRE(mods.damageMultiplier == Catch::Approx(1.3f));
    }

    SECTION("default modifiers for entity with no effects") {
        auto& mods = system.getModifiers(registry, entity);
        REQUIRE(mods.speedMultiplier == 1.0f);
        REQUIRE(mods.damageMultiplier == 1.0f);
        REQUIRE_FALSE(mods.stunned);
    }
}

// ============================================================================
// Capacity Tests
// ============================================================================

TEST_CASE("StatusEffectSystem capacity limits", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry);

    SECTION("max effects enforced") {
        // Apply MAX_EFFECTS different effects (different templates)
        for (uint32_t i = 0; i < ActiveStatusEffects::MAX_EFFECTS; ++i) {
            auto buff = makeBuff(i + 1, 10000, 0.1f);
            buff.source = static_cast<EntityID>(i + 1);  // Different source to prevent stacking
            uint32_t id = system.applyEffect(registry, entity, buff, 0);
            REQUIRE(id > 0);
        }

        // Next one should fail
        auto extraBuff = makeBuff(99, 10000, 0.1f);
        extraBuff.source = static_cast<EntityID>(99);
        uint32_t id = system.applyEffect(registry, entity, extraBuff, 0);
        REQUIRE(id == 0);

        auto* active = registry.try_get<ActiveStatusEffects>(entity);
        REQUIRE(active->count == ActiveStatusEffects::MAX_EFFECTS);
    }
}

// ============================================================================
// Callback Tests
// ============================================================================

TEST_CASE("StatusEffectSystem callbacks", "[StatusEffectSystem]") {
    auto registry = createTestRegistry();
    StatusEffectSystem system;
    auto entity = createTestEntity(registry, 5000);

    SECTION("DoT triggers damage callback") {
        bool callbackCalled = false;
        system.setOnPeriodicDamage([&](EntityID target, EntityID source, int16_t damage) {
            callbackCalled = true;
            REQUIRE(target == entity);
            REQUIRE(damage == 100);
        });

        auto dot = makeDoT(1, 10000, 1000, -100);
        system.applyEffect(registry, entity, dot, 0);
        system.update(registry, 1000);

        REQUIRE(callbackCalled);
    }

    SECTION("HoT triggers heal callback") {
        bool callbackCalled = false;
        system.setOnPeriodicHeal([&](EntityID target, int16_t amount) {
            callbackCalled = true;
            REQUIRE(target == entity);
            REQUIRE(amount == 50);
        });

        auto* combat = registry.try_get<CombatState>(entity);
        combat->health = 4000;

        auto hot = makeHoT(1, 10000, 1000, 50);
        system.applyEffect(registry, entity, hot, 0);
        system.update(registry, 1000);

        REQUIRE(callbackCalled);
    }
}

// ============================================================================
// Effect Structure Tests
// ============================================================================

TEST_CASE("StatusEffect isExpired and shouldTick", "[StatusEffectSystem]") {
    SECTION("isExpired with duration") {
        auto effect = makeBuff();
        effect.durationMs = 5000;
        effect.expiresAtMs = 5000;

        REQUIRE_FALSE(effect.isExpired(4999));
        REQUIRE(effect.isExpired(5000));
        REQUIRE(effect.isExpired(6000));
    }

    SECTION("permanent effect never expires") {
        auto effect = makeBuff();
        effect.durationMs = 0;

        REQUIRE_FALSE(effect.isExpired(999999));
    }

    SECTION("shouldTick based on interval") {
        auto effect = makeDoT();
        effect.tickIntervalMs = 2000;
        effect.lastTickMs = 0;

        REQUIRE_FALSE(effect.shouldTick(1999));
        REQUIRE(effect.shouldTick(2000));
        REQUIRE(effect.shouldTick(3000));
    }

    SECTION("isCrowdControl") {
        REQUIRE(makeStun().isCrowdControl());

        StatusEffect root;
        root.type = StatusEffectType::Root;
        REQUIRE(root.isCrowdControl());

        StatusEffect silence;
        silence.type = StatusEffectType::Silence;
        REQUIRE(silence.isCrowdControl());

        REQUIRE_FALSE(makeBuff().isCrowdControl());
    }
}

// ============================================================================
// ActiveStatusEffects Component Tests
// ============================================================================

TEST_CASE("ActiveStatusEffects find and query", "[StatusEffectSystem]") {
    ActiveStatusEffects active;

    SECTION("findIndex returns -1 for empty") {
        REQUIRE(active.findIndex(1) == -1);
    }

    SECTION("hasEffect returns false for empty") {
        REQUIRE_FALSE(active.hasEffect(StatusEffectType::Buff));
    }

    SECTION("findIndex finds by ID") {
        active.effects[0].effectId = 42;
        active.effects[0].type = StatusEffectType::Buff;
        active.count = 1;

        REQUIRE(active.findIndex(42) == 0);
        REQUIRE(active.findIndex(99) == -1);
    }

    SECTION("findTypeIndex finds first of type") {
        active.effects[0].effectId = 1;
        active.effects[0].type = StatusEffectType::Buff;
        active.effects[1].effectId = 2;
        active.effects[1].type = StatusEffectType::Debuff;
        active.effects[2].effectId = 3;
        active.effects[2].type = StatusEffectType::Buff;
        active.count = 3;

        REQUIRE(active.findTypeIndex(StatusEffectType::Buff) == 0);
        REQUIRE(active.findTypeIndex(StatusEffectType::Debuff) == 1);
        REQUIRE(active.findTypeIndex(StatusEffectType::DoT) == -1);
    }
}
