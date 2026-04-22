#include "combat/SpawnSystem.hpp"
#include "combat/NPCAISystem.hpp"
#include "combat/CombatSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace DarkAges {

// ============================================================================
// Spawn System Implementation
// ============================================================================

SpawnSystem::SpawnSystem() {
    std::random_device rd;
    rng_ = std::mt19937(rd());
}

void SpawnSystem::registerSpawnGroup(const SpawnGroup& group) {
    spawnGroups_[group.groupId] = group;
}

void SpawnSystem::registerSpawnRegion(uint32_t zoneId, float x, float z, 
                                      float radius, uint32_t spawnGroupId) {
    SpawnRegionComponent region(zoneId, x, z, radius, spawnGroupId);
    spawnRegions_[spawnGroupId] = region;
}

Position SpawnSystem::getSpawnPosition(uint32_t spawnGroupId, uint32_t zoneId,
                                      const std::vector<ZoneDefinition>& zones) {
    // Try spawn region first
    auto regionIt = spawnRegions_.find(spawnGroupId);
    if (regionIt != spawnRegions_.end()) {
        return getRandomPositionInRegion(spawnGroupId);
    }
    
    // Fall back to zone spawn position
    for (const auto& zone : zones) {
        if (zone.zoneId == zoneId) {
            return zone.getSpawnPosition();
        }
    }
    
    // Default to origin if zone not found
    return Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
}

void SpawnSystem::update(Registry& registry, uint32_t currentTimeMs) {
    // Find all spawnable entities that should respawn
    auto view = registry.view<SpawnableComponent>();
    
    for (auto entity : view) {
        auto& spawnable = registry.get<SpawnableComponent>(entity);
        
        // Skip if already spawned or not set to respawn
        if (spawnable.isSpawned || !spawnable.shouldRespawn) {
            continue;
        }
        
        // Check if respawn time has passed
        if (currentTimeMs >= spawnable.respawnTimeMs) {
            // Find zone ID from position
            // For simplicity, we assume zone 1 - in production would lookup
            uint32_t zoneId = 1;
            std::vector<ZoneDefinition> emptyZones;
            
            // Try to respawn
            EntityID spawned = spawnEntity(registry, spawnable.spawnGroupId, 
                                          zoneId, emptyZones);
            if (spawned != entt::null) {
                // Mark original as no longer tracking
                spawnable.isSpawned = true;
            }
        }
    }
}

EntityID SpawnSystem::spawnEntity(Registry& registry, uint32_t spawnGroupId,
                                 uint32_t zoneId,
                                 const std::vector<ZoneDefinition>& zones) {
    auto groupIt = spawnGroups_.find(spawnGroupId);
    if (groupIt == spawnGroups_.end()) {
        return entt::null;
    }
    
    auto& group = groupIt->second;  // Non-const reference
    
    // Check max alive limit
    if (group.maxAlive > 0) {
        uint32_t alive = getAliveCount(registry, spawnGroupId);
        if (alive >= group.maxAlive) {
            return entt::null;
        }
    }
    
    // Select spawn config
    const NPCSpawnConfig* config = selectRandomConfig(group);
    if (!config) {
        return entt::null;
    }
    
    // Get spawn position
    Position spawnPos = getSpawnPosition(spawnGroupId, zoneId, zones);
    
    // Set level from config
    uint8_t level = config->minLevel;
    if (config->maxLevel > config->minLevel) {
        std::uniform_int_distribution<uint8_t> dist(config->minLevel, 
                                                   config->maxLevel);
        level = dist(rng_);
    }
    
    // Create NPC from template
    EntityID npc = entt::null;
    if (createNPCFromTemplate(registry, config->npcTemplateId, spawnPos, level)) {
        // Get the created entity - we need to track it
        // For now return a new entity with spawn component
        npc = registry.create();
        registry.emplace<Position>(npc, spawnPos);
        registry.emplace<Velocity>(npc);
        registry.emplace<Rotation>(npc);
        registry.emplace<BoundingVolume>(npc);
        
        CombatState combat;
        combat.health = 100;
        combat.maxHealth = 100;
        registry.emplace<CombatState>(npc, combat);
        
        registry.emplace<Mana>(npc);
        registry.emplace<NPCTag>(npc);
        registry.emplace<CollisionLayer>(npc, CollisionLayer::makeNPC());
        
        NPCAIState ai;
        ai.spawnPoint = spawnPos;
        registry.emplace<NPCAIState>(npc, ai);
        
        NPCStats stats;
        stats.baseDamage = 10;
        stats.xpReward = 50;
        stats.respawnTimeMs = config->respawnTimeMs;
        registry.emplace<NPCStats>(npc, stats);
        
        // Add spawnable component with respawn time
        SpawnableComponent spawnable;
        spawnable.spawnGroupId = spawnGroupId;
        spawnable.templateId = config->npcTemplateId;
        spawnable.respawnTimeMs = config->respawnTimeMs;  // Respawn delay from config
        spawnable.spawnPosition = spawnPos;
        spawnable.level = level;
        spawnable.isSpawned = true;
        spawnable.shouldRespawn = true;
        registry.emplace<SpawnableComponent>(npc, spawnable);
        
        if (spawnCallback_) {
            spawnCallback_(npc, spawnGroupId);
        }
    }
    
    return npc;
}

void SpawnSystem::onEntityDeath(Registry& registry, EntityID entity) {
    // Get spawnable component if present
    auto* spawnable = registry.try_get<SpawnableComponent>(entity);
    if (!spawnable) {
        return;
    }
    
    // Get respawn time from NPC stats if available
    uint32_t respawnMs = DEFAULT_RESPAWN_TIME_MS;
    if (auto* stats = registry.try_get<NPCStats>(entity)) {
        respawnMs = stats->respawnTimeMs;
    }
    if (respawnMs == 0) {
        respawnMs = DEFAULT_RESPAWN_TIME_MS;
    }
    
    // Get current time from registry or use default
    // Note: in production would get actual game time
    uint32_t currentTimeMs = 0;
    
    // Update spawnable to mark as dead and set respawn time
    spawnable->isSpawned = false;
    spawnable->respawnTimeMs = currentTimeMs + respawnMs;
    
    // Get the group for callback
    uint32_t groupId = spawnable->spawnGroupId;
    
    if (deathCallback_) {
        deathCallback_(entity, groupId);
    }
}

bool SpawnSystem::createNPCFromTemplate(Registry& registry, uint32_t templateId,
                                       const Position& spawnPos, uint8_t level) {
    // Template-based NPC creation
    // In production, would look up template data and create appropriately
    // For now, create a basic NPC
    
    auto entity = registry.create();
    registry.emplace<Position>(entity, spawnPos);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<BoundingVolume>(entity);
    
    // Scale health with level
    uint32_t baseHealth = 100 + (level * 10);
    CombatState combat;
    combat.health = baseHealth;
    combat.maxHealth = baseHealth;
    registry.emplace<CombatState>(entity, combat);
    
    registry.emplace<Mana>(entity);
    registry.emplace<NPCTag>(entity);
    registry.emplace<CollisionLayer>(entity, CollisionLayer::makeNPC());
    
    NPCAIState ai;
    ai.spawnPoint = spawnPos;
    registry.emplace<NPCAIState>(entity, ai);
    
    NPCStats stats;
    stats.baseDamage = 5 + (level * 2);
    stats.xpReward = 10 + (level * 5);
    stats.respawnTimeMs = 60000;  // 1 minute default
    registry.emplace<NPCStats>(entity, stats);
    
    return true;
}

const SpawnGroup* SpawnSystem::getSpawnGroup(uint32_t groupId) const {
    auto it = spawnGroups_.find(groupId);
    if (it != spawnGroups_.end()) {
        return &it->second;
    }
    return nullptr;
}

uint32_t SpawnSystem::getAliveCount(Registry& registry, uint32_t spawnGroupId) const {
    uint32_t count = 0;
    
    auto view = registry.view<SpawnableComponent>();
    for (auto entity : view) {
        const auto& spawnable = registry.get<SpawnableComponent>(entity);
        if (spawnable.spawnGroupId == spawnGroupId && spawnable.isSpawned) {
            count++;
        }
    }
    
    return count;
}

void SpawnSystem::forceSpawnGroup(Registry& registry, uint32_t spawnGroupId,
                                   uint32_t zoneId,
                                   const std::vector<ZoneDefinition>& zones) {
    auto* group = getSpawnGroup(spawnGroupId);
    if (!group) {
        return;
    }
    
    // Spawn all NPCs in the group immediately
    for (const auto& config : group->npcs) {
        for (uint8_t i = 0; i < config.spawnCount; ++i) {
            Position spawnPos = getSpawnPosition(spawnGroupId, zoneId, zones);
            
            // Add some jitter to position
            float jitterX = std::uniform_real_distribution<float>(-5.0f, 5.0f)(rng_);
            float jitterZ = std::uniform_real_distribution<float>(-5.0f, 5.0f)(rng_);
            spawnPos.x += jitterX;
            spawnPos.z += jitterZ;
            
            uint8_t level = config.minLevel;
            if (config.maxLevel > config.minLevel) {
                std::uniform_int_distribution<uint8_t> dist(config.minLevel,
                                                             config.maxLevel);
                level = dist(rng_);
            }
            
            createNPCFromTemplate(registry, config.npcTemplateId, spawnPos, level);
        }
    }
}

// ============================================================================
// Private Methods
// ============================================================================

const NPCSpawnConfig* SpawnSystem::selectRandomConfig(SpawnGroup& group) {
    if (group.npcs.empty()) {
        return nullptr;
    }
    
    // Weighted selection
    float totalWeight = 0.0f;
    for (const auto& config : group.npcs) {
        totalWeight += config.spawnWeight;
    }
    
    if (totalWeight <= 0.0f) {
        return &group.npcs[0];
    }
    
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(rng_);
    
    float cumulative = 0.0f;
    for (const auto& config : group.npcs) {
        cumulative += config.spawnWeight;
        if (roll <= cumulative) {
            return &config;
        }
    }
    
    return &group.npcs.back();
}

Position SpawnSystem::getRandomPositionInRegion(uint32_t spawnGroupId) {
    auto it = spawnRegions_.find(spawnGroupId);
    if (it == spawnRegions_.end()) {
        return Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
    }
    
    const auto& region = it->second;
    
    // Random position within circle
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> radiusDist(0.0f, region.radius);
    
    float angle = angleDist(rng_);
    float radius = radiusDist(rng_);
    
    float x = region.centerX + radius * std::cos(angle);
    float z = region.centerZ + radius * std::sin(angle);
    
    return Position::fromVec3(glm::vec3(x, 0.0f, z));
}

} // namespace DarkAges