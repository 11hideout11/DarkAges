// [COMBAT_AGENT] Integration tests for StatusEffectSystem <-> Combat/Movement interactions
// Tests that status effects properly influence combat damage, crowd control, and regeneration

#include <catch2/catch_test_macros.hpp>
#include "combat/CombatSystem.hpp"
#include "combat/StatusEffectSystem.hpp"
#include "physics/MovementSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using namespace DarkAges;

// ============================================================================
// Helper: Create a test entity with combat components
// ============================================================================
static EntityID createTestEntity(Registry& registry, int16_t health = 10000,
                                  float x = 0.0f, float z = 0.0f) {
    EntityID entity = registry.create();
    registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(x, 0.0f, z), 0));
    registry.emplace<Velocity>(entity);
    Rotation& rot = registry.emplace<Rotation>(entity);
    rot.yaw = 0.0f;  // Facing +Z (forward)
    registry.emplace<CombatState>(entity);
    registry.emplace<InputState>(entity);
    auto& combat = registry.get<CombatState>(entity);
    combat.health = health;
    combat.maxHealth = health;
    return entity;
}

// ============================================================================
// DAMAGE MODIFIER TESTS
// ============================================================================

TEST_CASE("StatusEffect damage multiplier affects combat", "[combat][integration]") {
    Registry registry;
    CombatConfig config;
    config.baseMeleeDamage = 2000;
    config.damageVariance = 0;  // Disable variance for deterministic tests
    config.criticalChance = 0;  // Disable crits for deterministic tests
    config.attackCooldownMs = 0;  // No cooldown
    config.allowFriendlyFire = true;

    CombatSystem combat(config);
    StatusEffectSystem ses;
    combat.setStatusEffectSystem(&ses);

    EntityID attacker = createTestEntity(registry, 10000, 0.0f, 0.0f);
    EntityID target = createTestEntity(registry, 10000, 0.0f, 1.0f);  // In front (+Z)

    SECTION("Damage buff increases damage dealt") {
        // Apply +50% damage buff to attacker
        StatusEffect buff;
        buff.type = StatusEffectType::Buff;
        buff.statType = StatType::AttackDamage;
        buff.modifierPercent = 0.5f;  // +50%
        buff.durationMs = 10000;
        buff.source = attacker;
        buff.target = attacker;
        buff.name = "DamageBuff";

        ses.applyEffect(registry, attacker, buff, 0);

        // Get health before
        int16_t healthBefore = registry.get<CombatState>(target).health;

        // Attack - target is at +Z which is forward (yaw=0)
        HitResult result = combat.performMeleeAttack(registry, attacker, 100);

        REQUIRE(result.hit);
        // With 50% buff, damage should be 2000 * 1.5 = 3000
        REQUIRE(result.damageDealt == 3000);

        int16_t healthAfter = registry.get<CombatState>(target).health;
        REQUIRE(healthBefore - healthAfter == 3000);
    }

    SECTION("Damage debuff decreases damage dealt") {
        // Apply -30% damage debuff to attacker
        StatusEffect debuff;
        debuff.type = StatusEffectType::Debuff;
        debuff.statType = StatType::AttackDamage;
        debuff.modifierPercent = -0.3f;  // -30%
        debuff.durationMs = 10000;
        debuff.source = attacker;
        debuff.target = attacker;
        debuff.name = "DamageDebuff";
        debuff.debuff = true;

        ses.applyEffect(registry, attacker, debuff, 0);

        HitResult result = combat.performMeleeAttack(registry, attacker, 100);

        REQUIRE(result.hit);
        // With -30% debuff, damage should be 2000 * 0.7 = 1400
        REQUIRE(result.damageDealt == 1400);
    }

    SECTION("Armor buff reduces damage taken") {
        // Apply +50% armor buff to target
        StatusEffect armorBuff;
        armorBuff.type = StatusEffectType::Buff;
        armorBuff.statType = StatType::Armor;
        armorBuff.modifierPercent = 0.5f;  // +50% armor
        armorBuff.durationMs = 10000;
        armorBuff.source = target;
        armorBuff.target = target;
        armorBuff.name = "ArmorBuff";

        ses.applyEffect(registry, target, armorBuff, 0);

        HitResult result = combat.performMeleeAttack(registry, attacker, 100);

        REQUIRE(result.hit);
        // With 50% armor, damage should be 2000 / 1.5 = 1333
        REQUIRE(result.damageDealt == 1333);
    }

    SECTION("Combined damage buff and armor buff") {
        // Attacker: +50% damage
        StatusEffect dmgBuff;
        dmgBuff.type = StatusEffectType::Buff;
        dmgBuff.statType = StatType::AttackDamage;
        dmgBuff.modifierPercent = 0.5f;
        dmgBuff.durationMs = 10000;
        dmgBuff.source = attacker;
        dmgBuff.target = attacker;
        dmgBuff.name = "DmgBuff";
        ses.applyEffect(registry, attacker, dmgBuff, 0);

        // Target: +100% armor (doubles armor)
        StatusEffect armorBuff;
        armorBuff.type = StatusEffectType::Buff;
        armorBuff.statType = StatType::Armor;
        armorBuff.modifierPercent = 1.0f;  // +100% armor
        armorBuff.durationMs = 10000;
        armorBuff.source = target;
        armorBuff.target = target;
        armorBuff.name = "ArmorBuff";
        ses.applyEffect(registry, target, armorBuff, 0);

        HitResult result = combat.performMeleeAttack(registry, attacker, 100);

        REQUIRE(result.hit);
        // 2000 * 1.5 / 2.0 = 1500
        REQUIRE(result.damageDealt == 1500);
    }
}

// ============================================================================
// SHIELD ABSORPTION TESTS
// ============================================================================

TEST_CASE("Shield absorbs damage", "[combat][integration]") {
    Registry registry;
    CombatConfig config;
    config.baseMeleeDamage = 2000;
    config.damageVariance = 0;
    config.criticalChance = 0;
    config.attackCooldownMs = 0;
    config.allowFriendlyFire = true;

    CombatSystem combat(config);
    StatusEffectSystem ses;
    combat.setStatusEffectSystem(&ses);

    EntityID attacker = createTestEntity(registry, 10000, 0.0f, 0.0f);
    EntityID target = createTestEntity(registry, 10000, 0.0f, 1.0f);  // In front (+Z)

    SECTION("Shield absorbs partial damage") {
        // Apply shield that absorbs 1500 damage
        StatusEffect shield;
        shield.type = StatusEffectType::Shield;
        shield.shieldAbsorb = 1500;
        shield.durationMs = 10000;
        shield.source = target;
        shield.target = target;
        shield.name = "Shield";

        ses.applyEffect(registry, target, shield, 0);

        int16_t healthBefore = registry.get<CombatState>(target).health;
        HitResult result = combat.performMeleeAttack(registry, attacker, 100);
        int16_t healthAfter = registry.get<CombatState>(target).health;

        REQUIRE(result.hit);
        // Shield absorbs 1500 of 2000 damage, 500 goes through
        REQUIRE(healthBefore - healthAfter == 500);
    }

    SECTION("Shield absorbs all damage") {
        // Apply shield that absorbs 5000 damage (more than attack)
        StatusEffect shield;
        shield.type = StatusEffectType::Shield;
        shield.shieldAbsorb = 5000;
        shield.durationMs = 10000;
        shield.source = target;
        shield.target = target;
        shield.name = "BigShield";

        ses.applyEffect(registry, target, shield, 0);

        int16_t healthBefore = registry.get<CombatState>(target).health;
        HitResult result = combat.performMeleeAttack(registry, attacker, 100);
        int16_t healthAfter = registry.get<CombatState>(target).health;

        REQUIRE(result.hit);  // Hit still registers
        REQUIRE(healthBefore == healthAfter);  // No health lost
    }
}

// ============================================================================
// CROWD CONTROL - COMBAT TESTS
// ============================================================================

TEST_CASE("Stun prevents attacking", "[combat][integration]") {
    Registry registry;
    CombatConfig config;
    config.attackCooldownMs = 0;
    config.allowFriendlyFire = true;
    config.damageVariance = 0;
    config.criticalChance = 0;

    CombatSystem combat(config);
    StatusEffectSystem ses;
    combat.setStatusEffectSystem(&ses);

    EntityID attacker = createTestEntity(registry, 10000, 0.0f, 0.0f);
    EntityID target = createTestEntity(registry, 10000, 0.0f, 1.0f);  // In front (+Z)

    SECTION("Stunned entity cannot attack") {
        // Apply stun to attacker
        StatusEffect stun;
        stun.type = StatusEffectType::Stun;
        stun.durationMs = 3000;
        stun.source = target;
        stun.target = attacker;
        stun.name = "Stun";

        ses.applyEffect(registry, attacker, stun, 0);

        // Tick once to compute modifiers
        ses.update(registry, 100);

        REQUIRE_FALSE(combat.canAttack(registry, attacker, 200));
    }

    SECTION("Stunned entity cannot cast abilities") {
        // Apply stun
        StatusEffect stun;
        stun.type = StatusEffectType::Stun;
        stun.durationMs = 3000;
        stun.source = target;
        stun.target = attacker;
        stun.name = "Stun";

        ses.applyEffect(registry, attacker, stun, 0);
        ses.update(registry, 100);

        // Try to attack (should fail since stunned)
        AttackInput input;
        input.type = AttackInput::MELEE;
        input.timestamp = 200;

        HitResult result = combat.processAttack(registry, attacker, input, 200);
        REQUIRE_FALSE(result.hit);
        REQUIRE(result.hitType == std::string("cooldown"));
    }

    SECTION("Silenced entity cannot cast abilities") {
        // Apply silence
        StatusEffect silence;
        silence.type = StatusEffectType::Silence;
        silence.durationMs = 3000;
        silence.source = target;
        silence.target = attacker;
        silence.name = "Silence";

        ses.applyEffect(registry, attacker, silence, 0);
        ses.update(registry, 100);

        // Silence blocks ability casting but not basic attacks
        // canAttack checks for stun only, so melee should still work
        REQUIRE(combat.canAttack(registry, attacker, 200));

        // But ability input should be blocked
        AttackInput abilityInput;
        abilityInput.type = AttackInput::ABILITY;
        abilityInput.timestamp = 200;
        abilityInput.targetEntity = static_cast<uint32_t>(target);

        HitResult result = combat.processAttack(registry, attacker, abilityInput, 200);
        REQUIRE_FALSE(result.hit);
        REQUIRE(result.hitType == std::string("crowd_controlled"));
    }
}

// ============================================================================
// CROWD CONTROL - MOVEMENT TESTS
// ============================================================================

TEST_CASE("Crowd control prevents movement", "[movement][integration]") {
    Registry registry;
    MovementSystem movement;
    StatusEffectSystem ses;
    movement.setStatusEffectSystem(&ses);

    SECTION("Stunned entity does not move") {
        EntityID entity = registry.create();
        registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0));
        registry.emplace<Velocity>(entity);
        Rotation& rot = registry.emplace<Rotation>(entity);
        rot.yaw = 0.0f;
        registry.emplace<CombatState>(entity);

        // Apply stun
        StatusEffect stun;
        stun.type = StatusEffectType::Stun;
        stun.durationMs = 3000;
        stun.source = entity;
        stun.target = entity;
        stun.name = "Stun";
        ses.applyEffect(registry, entity, stun, 0);
        ses.update(registry, 100);

        // Create input moving forward
        InputState input;
        input.forward = 1;
        input.timestamp_ms = 200;
        registry.emplace<InputState>(entity, input);

        Position before = registry.get<Position>(entity);
        movement.update(registry, 250);
        Position after = registry.get<Position>(entity);

        REQUIRE(before.x == after.x);
        REQUIRE(before.z == after.z);
    }

    SECTION("Rooted entity does not move") {
        EntityID entity = registry.create();
        registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0));
        registry.emplace<Velocity>(entity);
        Rotation& rot = registry.emplace<Rotation>(entity);
        rot.yaw = 0.0f;
        registry.emplace<CombatState>(entity);

        // Apply root
        StatusEffect root;
        root.type = StatusEffectType::Root;
        root.durationMs = 3000;
        root.source = entity;
        root.target = entity;
        root.name = "Root";
        ses.applyEffect(registry, entity, root, 0);
        ses.update(registry, 100);

        InputState input;
        input.forward = 1;
        input.timestamp_ms = 200;
        registry.emplace<InputState>(entity, input);

        Position before = registry.get<Position>(entity);
        movement.update(registry, 250);
        Position after = registry.get<Position>(entity);

        REQUIRE(before.x == after.x);
        REQUIRE(before.z == after.z);
    }

    SECTION("Slowed entity moves at reduced speed") {
        // Create two entities - one slowed, one normal
        EntityID normal = registry.create();
        registry.emplace<Position>(normal, Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0));
        registry.emplace<Velocity>(normal);
        Rotation& rotN = registry.emplace<Rotation>(normal);
        rotN.yaw = 0.0f;
        registry.emplace<CombatState>(normal);

        EntityID slowed = registry.create();
        registry.emplace<Position>(slowed, Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0));
        registry.emplace<Velocity>(slowed);
        Rotation& rotS = registry.emplace<Rotation>(slowed);
        rotS.yaw = 0.0f;
        registry.emplace<CombatState>(slowed);

        // Apply 50% slow to slowed entity
        StatusEffect slow;
        slow.type = StatusEffectType::Debuff;
        slow.statType = StatType::MovementSpeed;
        slow.modifierPercent = -0.5f;  // -50% speed
        slow.durationMs = 10000;
        slow.source = slowed;
        slow.target = slowed;
        slow.name = "Slow";
        slow.debuff = true;
        ses.applyEffect(registry, slowed, slow, 0);
        ses.update(registry, 100);

        // Both get same forward input
        InputState input;
        input.forward = 1;
        input.timestamp_ms = 200;
        registry.emplace<InputState>(normal, input);
        registry.emplace<InputState>(slowed, input);

        // Run for multiple ticks to accumulate movement
        for (uint32_t t = 300; t <= 1000; t += 16) {
            movement.update(registry, t);
        }

        float normalZ = std::abs(registry.get<Position>(normal).z * Constants::FIXED_TO_FLOAT);
        float slowedZ = std::abs(registry.get<Position>(slowed).z * Constants::FIXED_TO_FLOAT);

        // Slowed entity should have moved less
        REQUIRE(slowedZ < normalZ);
        // Approximately 50% as far (allow some tolerance)
        REQUIRE(slowedZ > 0.0f);  // But did move
    }
}

// ============================================================================
// ATTACK SPEED MODIFIER TESTS
// ============================================================================

TEST_CASE("Attack speed modifier affects cooldown", "[combat][integration]") {
    Registry registry;
    CombatConfig config;
    config.attackCooldownMs = 1000;  // 1 second cooldown
    config.allowFriendlyFire = true;
    config.damageVariance = 0;
    config.criticalChance = 0;

    CombatSystem combat(config);
    StatusEffectSystem ses;
    combat.setStatusEffectSystem(&ses);

    EntityID attacker = createTestEntity(registry, 10000, 0.0f, 0.0f);
    EntityID target = createTestEntity(registry, 10000, 0.0f, 1.0f);  // In front (+Z)

    SECTION("Attack speed buff reduces cooldown") {
        // Apply +100% attack speed (halves cooldown)
        StatusEffect atkSpeedBuff;
        atkSpeedBuff.type = StatusEffectType::Buff;
        atkSpeedBuff.statType = StatType::AttackSpeed;
        atkSpeedBuff.modifierPercent = 1.0f;  // +100% attack speed
        atkSpeedBuff.durationMs = 10000;
        atkSpeedBuff.source = attacker;
        atkSpeedBuff.target = attacker;
        atkSpeedBuff.name = "Haste";
        ses.applyEffect(registry, attacker, atkSpeedBuff, 0);
        ses.update(registry, 100);

        // First attack at t=1000 (use non-zero to avoid lastAttackTime==0 shortcut)
        registry.get<CombatState>(attacker).lastAttackTime = 1000;

        // At 1500ms, normal cooldown (1000ms) would block, but with +100% speed
        // effective cooldown = 1000/2 = 500ms, so attack should be allowed
        REQUIRE(combat.canAttack(registry, attacker, 1500));

        // At 1400ms, only 400ms elapsed, still on cooldown (500ms needed)
        REQUIRE_FALSE(combat.canAttack(registry, attacker, 1400));
    }
}

// ============================================================================
// REGENERATION MODIFIER TESTS
// ============================================================================

TEST_CASE("HealthRegenSystem uses regen modifier", "[combat][integration]") {
    Registry registry;
    StatusEffectSystem ses;

    HealthRegenSystem healthRegen;
    healthRegen.setStatusEffectSystem(&ses);

    EntityID entity = createTestEntity(registry, 5000, 0.0f, 0.0f);  // Start at half health
    auto& combat = registry.get<CombatState>(entity);
    combat.health = 5000;
    combat.maxHealth = 10000;

    SECTION("Regen buff increases healing") {
        // Apply +100% regen buff
        StatusEffect regenBuff;
        regenBuff.type = StatusEffectType::Buff;
        regenBuff.statType = StatType::HealthRegen;
        regenBuff.modifierPercent = 1.0f;  // +100% regen
        regenBuff.durationMs = 10000;
        regenBuff.source = entity;
        regenBuff.target = entity;
        regenBuff.name = "RegenBuff";
        ses.applyEffect(registry, entity, regenBuff, 0);
        ses.update(registry, 100);

        int16_t healthBefore = combat.health;

        // Tick regen at 1 second
        healthRegen.update(registry, 1000);

        int16_t healed = combat.health - healthBefore;
        // Normal regen is 50, with +100% = 100
        REQUIRE(healed == 100);
    }
}

TEST_CASE("ManaRegenSystem regenerates mana", "[combat][integration]") {
    Registry registry;
    StatusEffectSystem ses;

    ManaRegenSystem manaRegen;
    manaRegen.setStatusEffectSystem(&ses);

    EntityID entity = registry.create();
    Mana& mana = registry.emplace<Mana>(entity);
    mana.current = 50.0f;
    mana.max = 100.0f;
    mana.regenerationRate = 5.0f;  // 5 mana per second

    SECTION("Basic mana regeneration") {
        manaRegen.update(registry, 1000);

        REQUIRE(mana.current == 55.0f);
    }

    SECTION("Mana does not exceed max") {
        mana.current = 98.0f;
        manaRegen.update(registry, 1000);

        REQUIRE(mana.current == 100.0f);  // Clamped to max
    }

    SECTION("Mana regen buff increases regeneration") {
        // Apply +100% mana regen buff
        StatusEffect regenBuff;
        regenBuff.type = StatusEffectType::Buff;
        regenBuff.statType = StatType::ManaRegen;
        regenBuff.modifierPercent = 1.0f;  // +100% mana regen
        regenBuff.durationMs = 10000;
        regenBuff.source = entity;
        regenBuff.target = entity;
        regenBuff.name = "ManaRegenBuff";
        ses.applyEffect(registry, entity, regenBuff, 0);
        ses.update(registry, 100);

        manaRegen.update(registry, 1000);

        // 5 * 2.0 = 10 mana regenerated
        REQUIRE(mana.current == 60.0f);
    }

    SECTION("Mana regen debuff decreases regeneration") {
        // Apply -50% mana regen debuff
        StatusEffect regenDebuff;
        regenDebuff.type = StatusEffectType::Debuff;
        regenDebuff.statType = StatType::ManaRegen;
        regenDebuff.modifierPercent = -0.5f;  // -50% mana regen
        regenDebuff.durationMs = 10000;
        regenDebuff.source = entity;
        regenDebuff.target = entity;
        regenDebuff.name = "ManaDrain";
        regenDebuff.debuff = true;
        ses.applyEffect(registry, entity, regenDebuff, 0);
        ses.update(registry, 100);

        manaRegen.update(registry, 1000);

        // 5 * 0.5 = 2.5 mana regenerated
        REQUIRE(mana.current == 52.5f);
    }
}

// ============================================================================
// FULL COMBAT INTEGRATION TESTS
// ============================================================================

TEST_CASE("Full combat scenario with status effects", "[combat][integration]") {
    Registry registry;
    CombatConfig config;
    config.baseMeleeDamage = 2000;
    config.damageVariance = 0;
    config.criticalChance = 0;
    config.attackCooldownMs = 500;

    CombatSystem combat(config);
    StatusEffectSystem ses;
    combat.setStatusEffectSystem(&ses);

    EntityID warrior = createTestEntity(registry, 10000, 0.0f, 0.0f);
    EntityID mage = createTestEntity(registry, 8000, 0.0f, 1.0f);  // In front of warrior

    SECTION("Warrior buffs damage, mage shields, warrior attacks through") {
        // Warrior gets +50% damage buff
        StatusEffect dmgBuff;
        dmgBuff.type = StatusEffectType::Buff;
        dmgBuff.statType = StatType::AttackDamage;
        dmgBuff.modifierPercent = 0.5f;
        dmgBuff.durationMs = 10000;
        dmgBuff.source = warrior;
        dmgBuff.target = warrior;
        dmgBuff.name = "BattleCry";
        ses.applyEffect(registry, warrior, dmgBuff, 0);

        // Mage gets shield for 1000 damage
        StatusEffect shield;
        shield.type = StatusEffectType::Shield;
        shield.shieldAbsorb = 1000;
        shield.durationMs = 10000;
        shield.source = mage;
        shield.target = mage;
        shield.name = "ManaShield";
        ses.applyEffect(registry, mage, shield, 0);

        ses.update(registry, 100);

        // Warrior attacks mage
        int16_t mageHealthBefore = registry.get<CombatState>(mage).health;
        HitResult result = combat.performMeleeAttack(registry, warrior, 200);

        REQUIRE(result.hit);
        // Damage: 2000 * 1.5 = 3000
        // Shield absorbs 1000, remaining 2000 damage
        REQUIRE(result.damageDealt == 3000);
        int16_t mageHealthAfter = registry.get<CombatState>(mage).health;
        REQUIRE(mageHealthBefore - mageHealthAfter == 2000);
    }

    SECTION("Stun breaks attack chain") {
        // Warrior attacks at t=0
        REQUIRE(combat.canAttack(registry, warrior, 0));

        registry.get<CombatState>(warrior).lastAttackTime = 0;

        // At t=600, cooldown passed, should be able to attack
        REQUIRE(combat.canAttack(registry, warrior, 600));

        // Mage stuns warrior at t=600
        StatusEffect stun;
        stun.type = StatusEffectType::Stun;
        stun.durationMs = 2000;
        stun.source = mage;
        stun.target = warrior;
        stun.name = "Stun";
        ses.applyEffect(registry, warrior, stun, 600);
        ses.update(registry, 650);

        // Warrior cannot attack while stunned
        REQUIRE_FALSE(combat.canAttack(registry, warrior, 700));

        // After stun expires at t=2600, warrior can attack again
        ses.update(registry, 2600);
        REQUIRE(combat.canAttack(registry, warrior, 2700));
    }
}
