#include "combat/CombatSystem.hpp"
#include "combat/detail/AttackState.hpp"
#include "combat/detail/CooldownState.hpp"

namespace DarkAges::combat::detail {

// Free function: simple damage application for AttackState hit resolution
// This is a simplified version of CombatSystem::applyDamage to avoid system coupling.
static bool applyDamage(Registry& registry, EntityID target, EntityID attacker, int16_t damage, uint32_t /*currentTimeMs*/) {
    if (CombatState* combat = registry.try_get<CombatState>(target)) {
        if (combat->isDead) return false;
        combat->health -= damage;
        if (combat->health < 0) combat->health = 0;
        if (combat->health == 0) {
            combat->isDead = true;
            // Death callbacks would be invoked here in full system
        }
        return true;
    }
    return false;
}

AttackState::AttackState()
    : timer_(0.0f), windupDuration_(0.1f), activeDuration_(0.033f),
      phase_(Phase::Windup), createdHitbox_(false) {}

void AttackState::Enter(Registry& registry, uint32_t entity) {
    timer_ = 0.0f;
    phase_ = Phase::Windup;
    createdHitbox_ = false;

    // Pull attack windup from entity's CombatConfig
    if (const CombatConfig* cfg = registry.try_get<CombatConfig>(entity)) {
        windupDuration_ = cfg->attackWindupMs / 1000.0f;
    }
    // TODO: Play windup animation on client via network
}

StateStatus AttackState::Update(Registry& registry, uint32_t entity, float dt) {
    timer_ += dt;

    if (phase_ == Phase::Windup) {
        if (timer_ >= windupDuration_) {
            phase_ = Phase::Active;
            createHitbox(registry, entity);
        }
        return StateStatus::Continue;
    }

    if (phase_ == Phase::Active) {
        // Active: one tick, check collision then advance
        checkCollision(registry, entity);
        return StateStatus::Finish;
    }

    // Should not reach here; safety fallback
    return StateStatus::Finish;
}

void AttackState::Exit(Registry& registry, uint32_t entity) {
    // Ensure hitbox is cleaned up if state exits prematurely
    removeHitbox(registry, entity);
}

State* AttackState::GetNextState(CombatState* /*combat*/) const {
    return new CooldownState();
}

void AttackState::createHitbox(Registry& registry, uint32_t attacker) {
    const Position* pos = registry.try_get<Position>(attacker);
    const Rotation* rot = registry.try_get<Rotation>(attacker);
    if (!pos || !rot) return;

    Hitbox hb;
    hb.radius = 1.5f; // melee range
    hb.origin = { pos->x * FIXED_TO_FLOAT, pos->y * FIXED_TO_FLOAT, pos->z * FIXED_TO_FLOAT };

    glm::vec3 forward = getForwardVector(rot->yaw);
    hb.origin += forward * (1.5f * FIXED_TO_FLOAT);

    hb.startTime = std::chrono::steady_clock::now();
    registry.emplace_or_replace<Hitbox>(attacker, hb);
}

void AttackState::checkCollision(Registry& registry, uint32_t attacker) {
    const Hitbox* hb = registry.try_get<Hitbox>(attacker);
    if (!hb) return;

    auto view = registry.view<Position, CombatState>();
    view.each([&](EntityID target, const Position& targetPos, CombatState& targetCombat) {
        if (target == attacker) return;
        if (targetCombat.isDead) return;

        float dx = (targetPos.x * FIXED_TO_FLOAT) - hb->origin.x;
        float dy = (targetPos.y * FIXED_TO_FLOAT) - hb->origin.y;
        float dz = (targetPos.z * FIXED_TO_FLOAT) - hb->origin.z;
        float distSq = dx*dx + dy*dy + dz*dz;

        if (distSq <= hb->radius * hb->radius) {
            // HIT — apply damage server-authoritatively
            applyDamage(registry, target, attacker, 25, 0);
        }
    });
}

void AttackState::removeHitbox(Registry& registry, uint32_t attacker) {
    registry.remove<Hitbox>(attacker);
}

} // namespace DarkAges::combat::detail
