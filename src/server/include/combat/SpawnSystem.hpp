#pragma once

#include "ecs/CoreTypes.hpp"
#include "zones/ZoneDefinition.hpp"
#include "physics/NavigationGrid.hpp"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <random>

namespace DarkAges {

// ============================================================================
// Spawn Configuration
// ============================================================================

// Configuration for an NPC spawn entry
struct NPCSpawnConfig {
    uint32_t npcTemplateId;     // NPC type to spawn
    float spawnWeight;         // Relative spawn probability (higher = more common)
    uint8_t minLevel;          // Minimum level to spawn
    uint8_t maxLevel;          // Maximum level to spawn
    uint8_t spawnCount;        // Number to spawn in this spawn group
    uint32_t respawnTimeMs;    // Respawn time in milliseconds
    
    NPCSpawnConfig(uint32_t templateId = 0, float weight = 1.0f,
                  uint8_t minLv = 1, uint8_t maxLv = 100,
                  uint8_t count = 1, uint32_t respawnMs = 60000)
        : npcTemplateId(templateId)
        , spawnWeight(weight)
        , minLevel(minLv)
        , maxLevel(maxLv)
        , spawnCount(count)
        , respawnTimeMs(respawnMs)
    {}
};

// Spawn group - a collection of NPCs that spawn together
struct SpawnGroup {
    uint32_t groupId;
    std::string groupName;
    std::vector<NPCSpawnConfig> npcs;
    uint32_t respawnTimeMs;      // Group-level respawn override (0 = use NPC config)
    uint8_t maxAlive;           // Maximum alive at once (0 = unlimited)
    bool isElite;             // Elite spawn group (bosses, etc.)
    
    SpawnGroup(uint32_t id = 0, std::string name = "", uint32_t respawnMs = 0, 
              uint8_t max = 0, bool elite = false)
        : groupId(id)
        , groupName(std::move(name))
        , respawnTimeMs(respawnMs)
        , maxAlive(max)
        , isElite(elite)
    {}
};

// ============================================================================
// Spawn System Components (ECS)
// ============================================================================

// Component tracking spawn state for an entity
struct SpawnableComponent {
    uint32_t spawnGroupId{0};       // Which spawn group this belongs to
    uint32_t templateId{0};         // NPC template type
    uint32_t respawnTimeMs{0};     // When this can respawn (delay in ms)
    uint8_t level{1};              // Spawn level
    Position spawnPosition;        // Where to spawn
    bool shouldRespawn{true};     // Whether to respawn after death
    bool isSpawned{false};        // Currently spawned (vs waiting to spawn)
    
    SpawnableComponent() = default;
    
    SpawnableComponent(uint32_t groupId, uint32_t npcTemplateId, uint32_t respawnMs,
                       uint8_t lv, const Position& pos)
        : spawnGroupId(groupId)
        , templateId(npcTemplateId)
        , respawnTimeMs(respawnMs)
        , level(lv)
        , spawnPosition(pos)
        , isSpawned(true)
    {}
};

// Component for tracking spawn regions (where groups spawn)
struct SpawnRegionComponent {
    uint32_t zoneId{0};           // Zone this region is in
    float centerX{0};             // Region center X
    float centerZ{0};            // Region center Z
    float radius{50.0f};         // Spawn radius
    uint32_t spawnGroupId{0};      // Which spawn group spawns here
    
    SpawnRegionComponent() = default;
    
    SpawnRegionComponent(uint32_t zone, float x, float z, float r, uint32_t groupId)
        : zoneId(zone), centerX(x), centerZ(z), radius(r), spawnGroupId(groupId)
    {}
};

// ============================================================================
// Spawn System - handles NPC spawn/respawn and spawn wave management
// ============================================================================

class SpawnSystem {
public:
    SpawnSystem();
    
    // Register a spawn group
    void registerSpawnGroup(const SpawnGroup& group);
    
    // Register a spawn region for a group
    void registerSpawnRegion(uint32_t zoneId, float x, float z, float radius, uint32_t spawnGroupId);
    
    // Get spawn position for a spawn group in a zone
    // Uses configured spawn positions or selects a random position in the spawn region
    Position getSpawnPosition(uint32_t spawnGroupId, uint32_t zoneId, 
                              const std::vector<ZoneDefinition>& zones);
    
    // Check and spawn due NPCs (called each tick)
    void update(Registry& registry, uint32_t currentTimeMs);
    
    // Spawn a dead NPC that is due to respawn
    // Returns the spawned entity or INVALID_ENTITY if nothing spawned
    EntityID spawnEntity(Registry& registry, uint32_t spawnGroupId, uint32_t zoneId,
                       const std::vector<ZoneDefinition>& zones);
    
    // Register an entity as dead (starts respawn timer)
    void onEntityDeath(Registry& registry, EntityID entity);
    
    // Get NPC template for a spawn config (called by spawnEntity)
    // Returns the created entity ID, or entt::null if creation failed
    EntityID createNPCFromTemplate(Registry& registry, uint32_t templateId, 
                                  const Position& spawnPos, uint8_t level);
    
    // Get config for a spawn group
    const SpawnGroup* getSpawnGroup(uint32_t groupId) const;
    
    // Get number of alive NPCs in a spawn group
    uint32_t getAliveCount(Registry& registry, uint32_t spawnGroupId) const;
    
    // Force spawn a group immediately (for GM commands, world boss spawns)
    void forceSpawnGroup(Registry& registry, uint32_t spawnGroupId, uint32_t zoneId,
                      const std::vector<ZoneDefinition>& zones);
    
    // Set callback for spawn events (for logging, notifications)
    using SpawnCallback = std::function<void(EntityID entity, uint32_t spawnGroupId)>;
    void setSpawnCallback(SpawnCallback cb) { spawnCallback_ = std::move(cb); }
    
    // Set callback for death events (before respawn)
    using DeathCallback = std::function<void(EntityID entity, uint32_t spawnGroupId)>;
    void setDeathCallback(DeathCallback cb) { deathCallback_ = std::move(cb); }

    // Notify spawn system that an NPC was manually spawned (fires spawn callback)
    void notifySpawned(EntityID entity, uint32_t spawnGroupId) {
        if (spawnCallback_) {
            spawnCallback_(entity, spawnGroupId);
        }
    }

    // Get navigation grid pointer (may be nullptr)
    [[nodiscard]] NavigationGrid* getNavigationGrid() const { return navGrid_; }

    // Set navigation grid for spawn position validation
    void setNavigationGrid(NavigationGrid* grid) { navGrid_ = grid; }

    // Set the current zone ID for respawn position lookups
    void setZoneId(uint32_t zoneId) { currentZoneId_ = zoneId; }
    [[nodiscard]] uint32_t getZoneId() const { return currentZoneId_; }

private:
    // Select a random spawn config from group based on weights
    const NPCSpawnConfig* selectRandomConfig(SpawnGroup& group);
    
    // Get a random position within spawn region
    Position getRandomPositionInRegion(uint32_t spawnGroupId);
    
    // Storage
    std::unordered_map<uint32_t, SpawnGroup> spawnGroups_;
    std::unordered_map<uint32_t, SpawnRegionComponent> spawnRegions_;
    
    // Random number generator
    std::mt19937 rng_;
    
    // Callbacks
    SpawnCallback spawnCallback_;
    DeathCallback deathCallback_;

    // Navigation grid for spawn position validation
    NavigationGrid* navGrid_{nullptr};

    // Current zone ID for respawn position lookups
    uint32_t currentZoneId_{1};

    // Default respawn time if not specified (60 seconds)
    static constexpr uint32_t DEFAULT_RESPAWN_TIME_MS = 60000;
};

} // namespace DarkAges