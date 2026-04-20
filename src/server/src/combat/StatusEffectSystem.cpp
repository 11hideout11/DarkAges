// [COMBAT_AGENT] Status effect system implementation
// Handles buffs, debuffs, DoTs, HoTs, and crowd control

#include "combat/StatusEffectSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace DarkAges {

// ============================================================================
// StatusEffectSystem Implementation
// ============================================================================

uint32_t StatusEffectSystem::applyEffect(Registry& registry, EntityID target,
                                          StatusEffect effect, uint32_t currentTimeMs) {
    // Get or add ActiveStatusEffects component
    ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active) {
        registry.emplace<ActiveStatusEffects>(target);
        active = registry.try_get<ActiveStatusEffects>(target);
    }

    if (!active) return 0;

    // Check for stacking - if same template exists, add stacks instead
    if (effect.templateId != 0) {
        for (uint8_t i = 0; i < active->count; ++i) {
            StatusEffect& existing = active->effects[i];
            if (existing.templateId == effect.templateId && existing.source == effect.source) {
                // Stack the effect
                if (existing.stacks < existing.maxStacks) {
                    existing.stacks++;
                    // Refresh duration
                    existing.appliedAtMs = currentTimeMs;
                    if (effect.durationMs > 0) {
                        existing.expiresAtMs = currentTimeMs + effect.durationMs;
                    }
                    active->dirty = true;
                    recomputeModifiers(*active);
                    return existing.effectId;
                } else {
                    // Max stacks - refresh duration only
                    existing.appliedAtMs = currentTimeMs;
                    if (effect.durationMs > 0) {
                        existing.expiresAtMs = currentTimeMs + effect.durationMs;
                    }
                    active->dirty = true;
                    recomputeModifiers(*active);
                    return existing.effectId;
                }
            }
        }
    }

    // Check capacity
    if (active->count >= ActiveStatusEffects::MAX_EFFECTS) {
        std::cerr << "[STATUSEFFECT] Cannot apply effect to entity - max effects reached ("
                  << static_cast<int>(ActiveStatusEffects::MAX_EFFECTS) << ")" << std::endl;
        return 0;
    }

    // Assign unique ID and timing
    effect.effectId = ++nextEffectId_;
    effect.appliedAtMs = currentTimeMs;
    effect.target = target;
    if (effect.durationMs > 0) {
        effect.expiresAtMs = currentTimeMs + effect.durationMs;
    }
    effect.lastTickMs = currentTimeMs;

    // Add to active effects
    active->effects[active->count] = std::move(effect);
    active->count++;
    active->dirty = true;
    recomputeModifiers(*active);

    return active->effects[active->count - 1].effectId;
}

bool StatusEffectSystem::removeEffect(Registry& registry, EntityID target, uint32_t effectId) {
    ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active) return false;

    int idx = active->findIndex(effectId);
    if (idx < 0) return false;

    removeAtIndex(*active, static_cast<uint8_t>(idx));
    return true;
}

uint8_t StatusEffectSystem::removeEffectsByType(Registry& registry, EntityID target,
                                                  StatusEffectType type) {
    ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active) return 0;

    uint8_t removed = 0;
    // Iterate backwards to avoid index issues during removal
    for (int i = static_cast<int>(active->count) - 1; i >= 0; --i) {
        if (active->effects[i].type == type) {
            removeAtIndex(*active, static_cast<uint8_t>(i));
            removed++;
        }
    }
    return removed;
}

uint8_t StatusEffectSystem::cleanse(Registry& registry, EntityID target) {
    ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active) return 0;

    uint8_t removed = 0;
    for (int i = static_cast<int>(active->count) - 1; i >= 0; --i) {
        if (active->effects[i].debuff) {
            removeAtIndex(*active, static_cast<uint8_t>(i));
            removed++;
        }
    }
    return removed;
}

void StatusEffectSystem::clearAll(Registry& registry, EntityID target) {
    ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active) return;

    active->count = 0;
    active->dirty = true;
    active->computedMods.reset();
}

int16_t StatusEffectSystem::update(Registry& registry, uint32_t currentTimeMs) {
    int16_t totalDelta = 0;

    auto view = registry.view<ActiveStatusEffects>();
    view.each([&](EntityID entity, ActiveStatusEffects& active) {
        if (active.count == 0) return;

        // Process effects in reverse for safe removal
        for (int i = static_cast<int>(active.count) - 1; i >= 0; --i) {
            StatusEffect& effect = active.effects[i];

            // Check expiration
            if (effect.isExpired(currentTimeMs)) {
                removeAtIndex(active, static_cast<uint8_t>(i));
                continue;
            }

            // Process periodic ticks (DoT/HoT)
            if (effect.shouldTick(currentTimeMs)) {
                processPeriodicTick(registry, effect, currentTimeMs);
                if (effect.tickAmount < 0) {
                    totalDelta += effect.tickAmount; // DoT damage (negative)
                } else {
                    totalDelta += effect.tickAmount; // HoT healing (positive)
                }
                effect.lastTickMs = currentTimeMs;
            }
        }

        // Recompute modifiers if effects changed
        if (active.dirty) {
            recomputeModifiers(active);
            active.dirty = false;
        }
    });

    return totalDelta;
}

bool StatusEffectSystem::hasEffect(const Registry& registry, EntityID target,
                                    StatusEffectType type) const {
    const ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active) return false;
    return active->hasEffect(type);
}

const StatModifiers& StatusEffectSystem::getModifiers(const Registry& registry,
                                                        EntityID target) const {
    static StatModifiers defaultMods;

    const ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active) return defaultMods;

    return active->computedMods;
}

int16_t StatusEffectSystem::absorbDamage(Registry& registry, EntityID target, int16_t damage) {
    ActiveStatusEffects* active = registry.try_get<ActiveStatusEffects>(target);
    if (!active || damage <= 0) return damage;

    // Find shield effects and absorb damage (reverse order for safe removal)
    for (int i = static_cast<int>(active->count) - 1; i >= 0 && damage > 0; --i) {
        if (active->effects[i].type == StatusEffectType::Shield) {
            StatusEffect& shield = active->effects[i];
            if (shield.shieldAbsorb <= 0) continue;

            int16_t absorbed = std::min(damage, shield.shieldAbsorb);
            shield.shieldAbsorb -= absorbed;
            damage -= absorbed;

            // Remove depleted shield
            if (shield.shieldAbsorb <= 0) {
                removeAtIndex(*active, static_cast<uint8_t>(i));
                // In reverse loop, no index adjustment needed
            }
        }
    }

    // Recompute modifiers so callers see updated shieldRemaining
    recomputeModifiers(*active);

    return damage;
}

void StatusEffectSystem::recomputeModifiers(ActiveStatusEffects& active) {
    active.computedMods.reset();

    for (uint8_t i = 0; i < active.count; ++i) {
        const StatusEffect& effect = active.effects[i];

        switch (effect.type) {
            case StatusEffectType::Buff:
            case StatusEffectType::Debuff: {
                float mod = 1.0f + (effect.modifierPercent * effect.stacks);
                switch (effect.statType) {
                    case StatType::MovementSpeed:
                        active.computedMods.speedMultiplier *= mod;
                        break;
                    case StatType::AttackDamage:
                        active.computedMods.damageMultiplier *= mod;
                        break;
                    case StatType::AttackSpeed:
                        active.computedMods.attackSpeedMultiplier *= mod;
                        break;
                    case StatType::Armor:
                        active.computedMods.armorMultiplier *= mod;
                        break;
                    case StatType::HealthRegen:
                        active.computedMods.healthRegenMultiplier *= mod;
                        break;
                    case StatType::ManaRegen:
                        active.computedMods.manaRegenMultiplier *= mod;
                        break;
                    default:
                        break;
                }
                break;
            }

            case StatusEffectType::Shield:
                active.computedMods.shieldRemaining += effect.shieldAbsorb;
                break;

            case StatusEffectType::Stun:
                active.computedMods.stunned = true;
                break;

            case StatusEffectType::Root:
                active.computedMods.rooted = true;
                break;

            case StatusEffectType::Silence:
                active.computedMods.silenced = true;
                break;

            case StatusEffectType::Slow:
                active.computedMods.speedMultiplier *= (1.0f + effect.modifierPercent);
                break;

            default:
                break;
        }
    }
}

void StatusEffectSystem::processPeriodicTick(Registry& registry, StatusEffect& effect,
                                               uint32_t currentTimeMs) {
    (void)currentTimeMs;

    if (effect.tickAmount == 0) return;

    if (effect.tickAmount < 0) {
        // DoT - deal damage
        CombatState* combat = registry.try_get<CombatState>(effect.target);
        if (combat && !combat->isDead) {
            int16_t damage = -effect.tickAmount;
            combat->health -= damage;
            if (combat->health <= 0) {
                combat->health = 0;
                combat->isDead = true;
                combat->lastAttacker = effect.source;
            }

            if (onPeriodicDamage_) {
                onPeriodicDamage_(effect.target, effect.source, damage);
            }
        }
    } else {
        // HoT - heal
        CombatState* combat = registry.try_get<CombatState>(effect.target);
        if (combat && !combat->isDead) {
            combat->health += effect.tickAmount;
            if (combat->health > combat->maxHealth) {
                combat->health = combat->maxHealth;
            }

            if (onPeriodicHeal_) {
                onPeriodicHeal_(effect.target, effect.tickAmount);
            }
        }
    }
}

void StatusEffectSystem::removeAtIndex(ActiveStatusEffects& active, uint8_t index) {
    if (index >= active.count) return;

    // Swap with last element for O(1) removal
    if (index < active.count - 1) {
        active.effects[index] = std::move(active.effects[active.count - 1]);
    }
    active.count--;
    active.dirty = true;
}

} // namespace DarkAges
