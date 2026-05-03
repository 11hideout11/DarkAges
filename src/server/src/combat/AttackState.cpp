#include "combat/detail/AttackState.hpp"
#include "combat/detail/CooldownState.hpp"
#include "combat/detail/Hitbox.hpp"
#include "combat/CombatSystem.hpp"
#include <chrono>
#include <cmath>

namespace DarkAges::combat::detail {

// ---------------------------------------------------------------------------
// Helper: apply damage to a target and flag death
// ---------------------------------------------------------------------------
static bool applyDamage(Registry& registry, EntityID target, EntityID attacker,
                       int16_t damage, uint32_t /*currentTimeMs*/) {
    if (CombatState* combat = registry.try_get<CombatState>(target)) {
        if (combat->isDead) return false;

        combat->health -= damage;
        if (combat->health < 0) combat->health = 0;
        if (combat->health == 0) {
            combat->isDead = true;
        }
        // Record attacker for XP/loot/credit
        combat->lastAttacker = attacker;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// AttackState implementation
// Three-phase melee attack: Windup -> Active (single-tick collision) -> Cooldown
// ---------------------------------------------------------------------------

AttackState::AttackState()
    : timer_(0.0f), windupDuration_(0.1f), activeDuration_(0.033f),
      phase_(Phase::Windup), createdHitbox_(false), config_(nullptr) {}

void AttackState::Enter(Registry& registry, EntityID entity, const CombatConfig& config) {
    timer_ = 0.0f;
    phase_ = Phase::Windup;
    createdHitbox_ = false;
    config_ = &config;
    windupDuration_ = config.attackWindupMs / 1000.0f;
}

StateStatus AttackState::Update(Registry& registry, EntityID entity, float dt, uint32_t currentTimeMs) {
    timer_ += dt;

    // Phase 1: Windup — animation prelude; hitbox creation is handled server-authoritatively
    if (phase_ == Phase::Windup) {
        if (timer_ >= windupDuration_) {
            phase_ = Phase::Active;
            // Hit validation is performed by LagCompensatedCombat; skip local hitbox creation
        }
        return StateStatus::Continue;
    }

    // Phase 2: Active — damage already applied; just finish
    if (phase_ == Phase::Active) {
        // Damage applied by LagCompensatedCombat; nothing to do here
        return StateStatus::Finish;
    }

    // Should not reach Phase::Cooldown here; handled by state transitions
    return StateStatus::Finish;
}

void AttackState::Exit(Registry& registry, EntityID entity) {
    removeHitbox(registry, entity);
}

State* AttackState::GetNextState(CombatState* /*combat*/) const {
    return new CooldownState();
}

// ---------------------------------------------------------------------------
// Hitbox lifecycle
// ---------------------------------------------------------------------------

void AttackState::createHitbox(Registry& registry, EntityID attacker) {
    // Read attacker's Position (fixed-point) and Rotation
    const Position* pos = registry.try_get<Position>(attacker);
    const Rotation* rot = registry.try_get<Rotation>(attacker);
    if (!pos || !rot) return;

    Hitbox hb;
    hb.radius = 1.5f;  // Melee range in meters

    // Convert fixed-point position to world float
    hb.origin = {
        pos->x * Constants::FIXED_TO_FLOAT,
        pos->y * Constants::FIXED_TO_FLOAT,
        pos->z * Constants::FIXED_TO_FLOAT
    };

    // Offset hitbox origin forward along facing direction (hand reach)
    glm::vec3 forward = glm::vec3(std::sin(rot->yaw), 0.0f, std::cos(rot->yaw));
    hb.origin += forward * (1.5f * Constants::FIXED_TO_FLOAT);

    hb.startTime = std::chrono::steady_clock::now();

    registry.emplace_or_replace<Hitbox>(attacker, hb);
    createdHitbox_ = true;
}

bool AttackState::checkCollision(Registry& registry, EntityID attacker) {
    assert(config_ != nullptr && "AttackState::config_ must be set before Update");
    const Hitbox* hb = registry.try_get<Hitbox>(attacker);
    if (!hb) return false;

    bool anyHit = false;
    // Scan all entities with Position+CombatState for melee collision
    auto view = registry.view<Position, CombatState>();
    view.each([&](EntityID target, const Position& targetPos, CombatState& targetCombat) {
        if (target == attacker) return;            // Don't hit self
        if (targetCombat.isDead) return;           // Skip corpses

        // Distance check (fixed-point -> float); squared for performance
        float dx = (targetPos.x * Constants::FIXED_TO_FLOAT) - hb->origin.x;
        float dy = (targetPos.y * Constants::FIXED_TO_FLOAT) - hb->origin.y;
        float dz = (targetPos.z * Constants::FIXED_TO_FLOAT) - hb->origin.z;
        float distSq = dx*dx + dy*dy + dz*dz;

        if (distSq <= hb->radius * hb->radius) {
            int16_t damage = config_->baseMeleeDamage;
            if (applyDamage(registry, target, attacker, damage, 0)) {
                anyHit = true;
            }
        }
    });
    return anyHit;
}

void AttackState::removeHitbox(Registry& registry, EntityID attacker) {
    registry.remove<Hitbox>(attacker);
}

} // namespace DarkAges::combat::detail

