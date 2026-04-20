#include "combat/ExperienceSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <cmath>

namespace DarkAges {

// ============================================================================
// Experience System Implementation
// ============================================================================

uint64_t ExperienceSystem::xpForLevel(uint32_t level) {
    // Scaling formula: base * level^1.5
    // Level 1 -> 100 XP, Level 2 -> 283, Level 5 -> 1118, Level 10 -> 3162
    if (level <= 1) return 100;
    double base = 100.0;
    double scaled = base * std::pow(static_cast<double>(level), 1.5);
    return static_cast<uint64_t>(scaled);
}

bool ExperienceSystem::awardKillXP(Registry& registry, EntityID killer, EntityID victim) {
    // Only players gain XP
    if (!registry.all_of<PlayerTag>(killer)) return false;

    // Get XP reward from victim
    const NPCStats* npcStats = registry.try_get<NPCStats>(victim);
    if (!npcStats) return false;

    return awardXP(registry, killer, npcStats->xpReward);
}

bool ExperienceSystem::awardXP(Registry& registry, EntityID player, uint64_t xpAmount) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return false;

    prog->currentXP += xpAmount;

    // Fire XP gain callback
    if (xpGainCallback_) {
        xpGainCallback_(player, xpAmount);
    }

    // Check for level-up(s) — could gain multiple levels at once
    bool leveledUp = false;
    while (prog->currentXP >= prog->xpToNextLevel) {
        prog->currentXP -= prog->xpToNextLevel;
        prog->level++;
        prog->statPoints += 3;  // 3 stat points per level
        prog->xpToNextLevel = xpForLevel(prog->level);
        leveledUp = true;

        // Fire level-up callback
        if (levelUpCallback_) {
            levelUpCallback_(player, prog->level);
        }
    }

    return leveledUp;
}

bool ExperienceSystem::checkAndApplyLevelUp(Registry& registry, EntityID player) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return false;

    if (prog->currentXP >= prog->xpToNextLevel) {
        return awardXP(registry, player, 0);  // Re-check with 0 XP to trigger level-up
    }

    return false;
}

// ============================================================================
// Loot System Implementation
// ============================================================================

uint32_t LootSystem::generateLoot(Registry& registry, EntityID npc, EntityID killer) {
    const LootTable* table = registry.try_get<LootTable>(npc);
    const Position* npcPos = registry.try_get<Position>(npc);
    if (!table || !npcPos) return 0;

    uint32_t dropped = 0;

    for (uint32_t i = 0; i < table->count && i < MAX_LOOT_ENTRIES; ++i) {
        const LootEntry& entry = table->entries[i];

        // Roll for drop chance
        float roll = static_cast<float>(std::rand()) / RAND_MAX;
        if (roll > entry.dropChance) continue;

        // Determine quantity
        uint32_t quantity = entry.minQuantity;
        if (entry.maxQuantity > entry.minQuantity) {
            quantity += static_cast<uint32_t>(std::rand()) % (entry.maxQuantity - entry.minQuantity + 1);
        }

        // Create loot drop entity at NPC position with slight offset
        auto lootEntity = registry.create();

        Position lootPos = *npcPos;
        // Offset slightly so items don't stack
        float offsetX = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f;
        float offsetZ = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f;
        lootPos.x += static_cast<Constants::Fixed>(offsetX * Constants::FLOAT_TO_FIXED);
        lootPos.z += static_cast<Constants::Fixed>(offsetZ * Constants::FLOAT_TO_FIXED);

        registry.emplace<Position>(lootEntity, lootPos);
        registry.emplace<LootDropTag>(lootEntity);

        LootDropData data;
        data.itemId = entry.itemId;
        data.quantity = quantity;
        data.goldAmount = 0.0f;
        data.ownerPlayer = killer;
        data.despawnTimeMs = 0;  // Will be set by caller
        registry.emplace<LootDropData>(lootEntity, data);

        // Set collision layer so players can interact
        registry.emplace<CollisionLayer>(lootEntity, CollisionLayer::makeStatic());

        dropped++;

        if (lootDropCallback_) {
            lootDropCallback_(lootEntity, entry.itemId, quantity, 0.0f);
        }
    }

    // Gold drop
    if (table->goldDropMax > 0.0f) {
        float goldAmount = table->goldDropMin;
        if (table->goldDropMax > table->goldDropMin) {
            goldAmount += (static_cast<float>(std::rand()) / RAND_MAX) *
                          (table->goldDropMax - table->goldDropMin);
        }

        auto goldEntity = registry.create();
        registry.emplace<Position>(goldEntity, *npcPos);
        registry.emplace<LootDropTag>(goldEntity);

        LootDropData goldData;
        goldData.itemId = 0;  // 0 = gold
        goldData.quantity = 0;
        goldData.goldAmount = goldAmount;
        goldData.ownerPlayer = killer;
        goldData.despawnTimeMs = 0;
        registry.emplace<LootDropData>(goldEntity, goldData);
        registry.emplace<CollisionLayer>(goldEntity, CollisionLayer::makeStatic());

        dropped++;

        if (lootDropCallback_) {
            lootDropCallback_(goldEntity, 0, 0, goldAmount);
        }
    }

    return dropped;
}

bool LootSystem::pickupLoot(Registry& registry, EntityID player, EntityID lootEntity) {
    if (!registry.all_of<PlayerTag>(player)) return false;
    if (!registry.all_of<LootDropTag>(lootEntity)) return false;

    LootDropData* data = registry.try_get<LootDropData>(lootEntity);
    if (!data) return false;

    // Check ownership (null = anyone can pick up)
    if (data->ownerPlayer != entt::null && data->ownerPlayer != player) {
        return false;
    }

    // Fire pickup callback
    if (lootPickupCallback_) {
        lootPickupCallback_(player, data->itemId, data->quantity, data->goldAmount);
    }

    // Destroy the loot entity
    registry.destroy(lootEntity);
    return true;
}

void LootSystem::update(Registry& registry, uint32_t currentTimeMs) {
    auto view = registry.view<LootDropData>();
    std::vector<EntityID> toRemove;

    view.each([&](EntityID entity, LootDropData& data) {
        if (data.despawnTimeMs > 0 && currentTimeMs >= data.despawnTimeMs) {
            toRemove.push_back(entity);
        }
    });

    for (auto entity : toRemove) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
}

} // namespace DarkAges