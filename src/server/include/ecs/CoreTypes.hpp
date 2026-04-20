#pragma once

#include "Constants.hpp"
#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <cmath>

// [PHYSICS_AGENT] Core ECS types and components
// All components are POD (Plain Old Data) for cache efficiency

namespace DarkAges {

using EntityID = entt::entity;
using Registry = entt::registry;
using ConnectionID = uint32_t;  // Network connection handle

// ============================================================================
// TRANSFORM COMPONENTS
// ============================================================================

// [PHYSICS_AGENT] Position using fixed-point for determinism
struct Position {
    Constants::Fixed x{0};
    Constants::Fixed y{0};
    Constants::Fixed z{0};
    uint32_t timestamp_ms{0};  // Server tick timestamp

    // Helper methods for float conversion
    [[nodiscard]] glm::vec3 toVec3() const {
        return glm::vec3(
            x * Constants::FIXED_TO_FLOAT,
            y * Constants::FIXED_TO_FLOAT,
            z * Constants::FIXED_TO_FLOAT
        );
    }

    static Position fromVec3(const glm::vec3& v, uint32_t timestamp = 0) {
        return Position{
            static_cast<Constants::Fixed>(v.x * Constants::FLOAT_TO_FIXED),
            static_cast<Constants::Fixed>(v.y * Constants::FLOAT_TO_FIXED),
            static_cast<Constants::Fixed>(v.z * Constants::FLOAT_TO_FIXED),
            timestamp
        };
    }

    // Distance squared (for performance - avoid sqrt when possible)
    [[nodiscard]] Constants::Fixed distanceSqTo(const Position& other) const {
        const Constants::Fixed dx = x - other.x;
        const Constants::Fixed dy = y - other.y;
        const Constants::Fixed dz = z - other.z;
        // Cast to int64_t to avoid overflow for large distances (200m = 200000 units)
        // 200000 * 200000 = 40,000,000,000 which overflows int32_t (max ~2.1 billion)
        const int64_t dx64 = dx;
        const int64_t dy64 = dy;
        const int64_t dz64 = dz;
        return static_cast<Constants::Fixed>((dx64 * dx64 + dy64 * dy64 + dz64 * dz64) / Constants::FIXED_PRECISION);
    }
};

// [PHYSICS_AGENT] Velocity (also fixed-point)
struct Velocity {
    Constants::Fixed dx{0};
    Constants::Fixed dy{0};
    Constants::Fixed dz{0};

    [[nodiscard]] float speed() const {
        float fx = dx * Constants::FIXED_TO_FLOAT;
        float fy = dy * Constants::FIXED_TO_FLOAT;
        float fz = dz * Constants::FIXED_TO_FLOAT;
        return std::sqrt(fx * fx + fy * fy + fz * fz);
    }

    [[nodiscard]] float speedSq() const {
        float fx = dx * Constants::FIXED_TO_FLOAT;
        float fy = dy * Constants::FIXED_TO_FLOAT;
        float fz = dz * Constants::FIXED_TO_FLOAT;
        return fx * fx + fy * fy + fz * fz;
    }
};

// [PHYSICS_AGENT] Rotation (yaw only for top-down/3rd person)
struct Rotation {
    float yaw{0.0f};    // Radians, 0 = +Z, PI/2 = +X
    float pitch{0.0f};  // Radians, for looking up/down
};

// ============================================================================
// INPUT COMPONENT
// ============================================================================

// [NETWORK_AGENT] Bit-packed input state from client
struct InputState {
    // Bit flags (1 byte total)
    uint8_t forward : 1;
    uint8_t backward : 1;
    uint8_t left : 1;
    uint8_t right : 1;
    uint8_t jump : 1;
    uint8_t attack : 1;
    uint8_t block : 1;
    uint8_t sprint : 1;

    // Camera rotation
    float yaw{0.0f};
    float pitch{0.0f};

    // Networking metadata
    uint32_t sequence{0};      // Monotonic counter for reconciliation
    uint32_t timestamp_ms{0};  // Client send time
    uint32_t targetEntity{0};  // Target entity ID for abilities/combat
    uint8_t abilitySlot{0};    // 0=melee attack, 1-4=ability slot cast
    uint8_t itemSlot{0};       // 0=no item use, 1-24=inventory slot to consume
    uint8_t chatChannel{0};    // 0=no chat, 1=local, 2=global, 3=whisper, 4=party, 5=guild
    uint32_t craftingRecipeId{0}; // 0=no craft, >0=recipe ID to craft

    // Initialize all bits to 0
    InputState() : forward(0), backward(0), left(0), right(0),
                   jump(0), attack(0), block(0), sprint(0) {}

    // Helper to check if any movement input is active
    [[nodiscard]] bool hasMovementInput() const {
        return forward || backward || left || right;
    }

    // Get input direction vector (normalized, before rotation)
    [[nodiscard]] glm::vec3 getInputDirection() const {
        glm::vec3 dir(0.0f);
        if (forward)  dir.z -= 1.0f;
        if (backward) dir.z += 1.0f;
        if (left)     dir.x -= 1.0f;
        if (right)    dir.x += 1.0f;

        if (glm::length(dir) > 0.0f) {
            dir = glm::normalize(dir);
        }
        return dir;
    }
};

// ============================================================================
// COMBAT COMPONENTS
// ============================================================================

// [COMBAT_AGENT] Combat state
struct CombatState {
    int16_t health{10000};      // 0-10000 (100.0 health)
    int16_t maxHealth{10000};
    uint8_t teamId{0};
    uint8_t classType{0};
    EntityID lastAttacker{entt::null};
    uint32_t lastAttackTime{0};
    bool isDead{false};

    [[nodiscard]] float healthPercent() const {
        return static_cast<float>(health) / static_cast<float>(maxHealth) * 100.0f;
    }
};

// ============================================================================
// SPATIAL COMPONENTS
// ============================================================================

// [PHYSICS_AGENT] Spatial hash cell tracking
struct SpatialCell {
    int32_t cellX{0};
    int32_t cellZ{0};
    uint32_t zoneId{0};
};

// [PHYSICS_AGENT] Bounding volume for collision
struct BoundingVolume {
    float radius{0.5f};      // Cylinder radius
    float height{1.8f};      // Cylinder height

    [[nodiscard]] bool intersects(const Position& posA, const Position& posB) const {
        // 2D circle intersection (XZ plane)
        float dx = (posA.x - posB.x) * Constants::FIXED_TO_FLOAT;
        float dz = (posA.z - posB.z) * Constants::FIXED_TO_FLOAT;
        float distSq = dx * dx + dz * dz;
        float minDist = radius * 2.0f;
        return distSq < (minDist * minDist);
    }
};

// ============================================================================
// PLAYER COMPONENTS
// ============================================================================

// [DATABASE_AGENT] Player identification
struct PlayerInfo {
    uint64_t playerId{0};      // Persistent player ID from DB
    uint32_t connectionId{0};  // Network connection handle
    char username[32]{0};
    uint64_t sessionStart{0};
};

// [NETWORK_AGENT] Network connection state
struct NetworkState {
    uint32_t lastInputSequence{0};
    uint32_t lastInputTime{0};
    uint32_t rttMs{0};
    float packetLoss{0.0f};
    uint32_t snapshotSequence{0};
};

// [SECURITY_AGENT] Anti-cheat tracking
struct AntiCheatState {
    Position lastValidPosition;
    uint32_t lastValidationTime{0};
    uint32_t suspiciousMovements{0};
    float maxRecordedSpeed{0.0f};
    uint32_t inputCount{0};
    uint32_t inputWindowStart{0};
};

// [NETWORK_AGENT] Entity state for network serialization (WP-8-6)
enum class EntityType : uint8_t {
    PLAYER = 0,
    PROJECTILE = 1,
    LOOT = 2,
    NPC = 3
};

enum class AnimationState : uint8_t {
    IDLE = 0,
    WALK = 1,
    RUN = 2,
    ATTACK = 3,
    BLOCK = 4,
    DEAD = 5
};

struct EntityState {
    uint32_t id{0};
    EntityType type{EntityType::PLAYER};
    Position position{};  // Uses Fixed default constructor
    Velocity velocity{};  // Uses Fixed default constructor
    Rotation rotation{};  // Uses float default constructor
    uint8_t healthPercent{100};
    AnimationState animState{AnimationState::IDLE};
    uint32_t teamId{0};
};

// ============================================================================
// ABILITY COMPONENTS
// ============================================================================

// [COMBAT_AGENT] Individual ability data
struct Ability {
    uint32_t abilityId{0};
    float cooldownRemaining{0.0f};
    float manaCost{0.0f};
};

// [COMBAT_AGENT] Collection of abilities for an entity
struct Abilities {
    static constexpr uint32_t MAX_ABILITIES = 8;
    Ability abilities[MAX_ABILITIES];
    uint32_t count{0};
};

// [COMBAT_AGENT] Mana/resource pool
struct Mana {
    float current{100.0f};
    float max{100.0f};
    float regenerationRate{1.0f};  // per second
};

// ============================================================================
 // ENTITY TAGS (empty types for tagging)
 // ============================================================================

 struct PlayerTag {};      // Entity is a player
 struct NPCTag {};         // Entity is an NPC
 struct ProjectileTag {};  // Entity is a projectile
 struct StaticTag {};      // Entity is static world geometry

 // ============================================================================
 // NPC AI COMPONENTS
 // ============================================================================

 // AI behavior states for NPCs
 enum class NPCBehavior : uint8_t {
     Idle = 0,      // Standing still, scanning for targets
     Wander = 1,    // Random movement within leash radius
     Chase = 2,     // Pursuing a target
     Attack = 3,    // Engaging target in melee
     Flee = 4,      // Running away at low health
     Dead = 5       // Waiting for respawn
 };

 // NPC combat archetype — determines behavior and combat style
 enum class NPCArchetype : uint8_t {
     Melee   = 0,  // Close-range fighter, chases and attacks in melee
     Ranged  = 1,  // Keeps distance, attacks from range
     Caster  = 2,  // Uses abilities (AoE, debuffs), lower HP
     Boss    = 3   // Elite mob, high stats, multiple abilities
 };

 // NPC AI state — controls behavior tree
 struct NPCAIState {
     NPCBehavior behavior{NPCBehavior::Idle};
     EntityID target{entt::null};              // Current aggro target
     float aggroRange{15.0f};                  // Meters — detect players
     float leashRange{30.0f};                  // Meters — return to spawn if exceeded
     float attackRange{2.0f};                  // Meters — melee range
     float fleeHealthPercent{20.0f};           // Flee below this HP%
     float wanderRadius{10.0f};               // Meters — wander from spawn
     float preferredRange{2.0f};              // Preferred engagement distance (ranged/caster)
     float retreatRange{0.0f};                // Back away if closer than this (ranged/caster)
     uint32_t behaviorTimerMs{0};             // Time in current behavior (ms)
     uint32_t attackCooldownMs{1500};          // Attack interval
     uint32_t lastAttackTimeMs{0};            // Last attack timestamp
     uint32_t wanderCooldownMs{3000};          // Min idle before wandering
     uint32_t lastAbilityTimeMs{0};            // Last special ability use
     Position spawnPoint{0, 0, 0, 0};         // Original spawn position
 };

 // NPC stat modifiers for different mob types
 struct NPCStats {
     uint8_t level{1};
     uint16_t baseDamage{10};                 // Base melee damage
     float attackSpeed{1.0f};                 // Attacks per second
     uint32_t xpReward{50};                   // XP given to killer
     uint32_t respawnTimeMs{10000};           // Respawn delay (10s default)
     NPCArchetype archetype{NPCArchetype::Melee}; // Combat archetype
 };

 // Loot table entry
 struct LootEntry {
     uint32_t itemId{0};
     uint32_t minQuantity{1};
     uint32_t maxQuantity{1};
     float dropChance{1.0f};                  // 0.0 to 1.0
 };

 // Loot table attached to NPC
 static constexpr uint32_t MAX_LOOT_ENTRIES = 8;
 struct LootTable {
     LootEntry entries[MAX_LOOT_ENTRIES];
     uint32_t count{0};
     float goldDropMin{0.0f};
     float goldDropMax{0.0f};
 };

 // Player experience / progression
 struct PlayerProgression {
     uint32_t level{1};
     uint64_t currentXP{0};
     uint64_t xpToNextLevel{100};             // XP needed for next level
     uint32_t statPoints{0};                  // Unspent stat points
 };

 // Dropped loot entity tag (for items on the ground)
 struct LootDropTag {};                       // Entity is a dropped item
 struct LootDropData {
     uint32_t itemId{0};
     uint32_t quantity{1};
     float goldAmount{0.0f};
     EntityID ownerPlayer{entt::null};        // Who can pick it up (null = anyone)
     uint32_t despawnTimeMs{0};               // When this loot despawns
 };

 // ============================================================================
 // COLLISION LAYERS
 // ============================================================================

 // [PHYSICS_AGENT] Collision layer bitmasks
 namespace CollisionLayerMask {
     constexpr uint32_t NONE     = 0;
     constexpr uint32_t PLAYER   = 1 << 0;   // 1
     constexpr uint32_t NPC      = 1 << 1;   // 2
     constexpr uint32_t PROJECTILE = 1 << 2; // 4
     constexpr uint32_t STATIC   = 1 << 3;   // 8
     constexpr uint32_t TRIGGER  = 1 << 4;   // 16

     // Default collidesWith masks
     constexpr uint32_t PLAYER_DEFAULT   = PLAYER | NPC | STATIC;
     constexpr uint32_t NPC_DEFAULT      = PLAYER | NPC | PROJECTILE | STATIC;
     constexpr uint32_t PROJECTILE_DEFAULT = PLAYER | NPC | STATIC;
     constexpr uint32_t STATIC_DEFAULT   = PLAYER | NPC;
 }

 // [PHYSICS_AGENT] Collision layer for entities
 struct CollisionLayer {
     uint32_t layer{0};         // Which layer this entity is on
     uint32_t collidesWith{0}; // Which layers this entity can collide with
     EntityID ownerEntity{entt::null}; // For projectiles: the entity that fired them

     static CollisionLayer makePlayer() {
         return CollisionLayer{CollisionLayerMask::PLAYER, CollisionLayerMask::PLAYER_DEFAULT, entt::null};
     }
     static CollisionLayer makeNPC() {
         return CollisionLayer{CollisionLayerMask::NPC, CollisionLayerMask::NPC_DEFAULT, entt::null};
     }
     static CollisionLayer makeProjectile(EntityID owner = entt::null) {
         return CollisionLayer{CollisionLayerMask::PROJECTILE, CollisionLayerMask::PROJECTILE_DEFAULT, owner};
     }
    static CollisionLayer makeStatic() {
        return CollisionLayer{CollisionLayerMask::STATIC, CollisionLayerMask::STATIC_DEFAULT, entt::null};
    }
};

// ============================================================================
// NPC ARCHETYPE CONFIG
// ============================================================================

// Archetype-specific NPC configuration
struct NPCArchetypeConfig {
    NPCArchetype archetype{NPCArchetype::Melee};
    float preferredRange{2.0f};       // Preferred engagement distance
    float retreatRange{1.5f};         // Back away if target closer than this (ranged/caster)
    float attackRange{20.0f};         // Max attack range (ranged/caster = 20, melee = 2)
    uint32_t abilityCooldownMs{3000}; // Special ability cooldown
    float abilityChance{0.3f};        // Chance per attack to use special ability
    int16_t healthScalePercent{100};  // HP multiplier vs base (boss = 500, caster = 70)
    int16_t damageScalePercent{100};  // Damage multiplier vs base (boss = 200, ranged = 80)
};

// ============================================================================
// ITEM SYSTEM
// ============================================================================

// Item rarity tiers
enum class ItemRarity : uint8_t {
    Common    = 0,
    Uncommon  = 1,
    Rare      = 2,
    Epic      = 3,
    Legendary = 4
};

// Item type categories
enum class ItemType : uint8_t {
    Weapon    = 0,  // Increases damage
    Armor     = 1,  // Increases defense/HP
    Accessory = 2,  // Utility bonuses
    Consumable = 3, // Potions, food (single use)
    Material  = 4,  // Crafting components
    Quest     = 5   // Quest items
};

// Equipment slot
enum class EquipSlot : uint8_t {
    None       = 0,
    MainHand   = 1,
    OffHand    = 2,
    Head       = 3,
    Chest      = 4,
    Legs       = 5,
    Feet       = 6,
    Ring       = 7,
    Amulet     = 8
};

// Stat modifiers an item can grant
struct ItemStats {
    int16_t damageBonus{0};       // Flat damage increase
    int16_t healthBonus{0};       // Flat HP increase
    int16_t manaBonus{0};         // Flat mana increase
    float speedBonus{0.0f};       // Movement speed multiplier
    float critChanceBonus{0.0f};  // Critical hit chance bonus
};

// Item definition — describes an item type
struct ItemDefinition {
    uint32_t itemId{0};
    char name[48]{0};
    char description[128]{0};
    ItemType type{ItemType::Material};
    ItemRarity rarity{ItemRarity::Common};
    EquipSlot equipSlot{EquipSlot::None};
    ItemStats stats{};
    uint32_t maxStackSize{1};     // How many per inventory slot
    uint32_t buyPrice{0};         // Vendor buy price (gold)
    uint32_t sellPrice{0};        // Vendor sell price (gold)
    bool tradable{true};
    uint32_t requiredLevel{1};    // Minimum level to use
};

// Inventory slot — one item stack
struct InventorySlot {
    uint32_t itemId{0};
    uint32_t quantity{0};

    bool isEmpty() const { return quantity == 0 || itemId == 0; }
};

// Player inventory component
static constexpr uint32_t INVENTORY_SIZE = 24;
struct Inventory {
    InventorySlot slots[INVENTORY_SIZE];
    float gold{0.0f};
    uint32_t slotCount{INVENTORY_SIZE}; // Active slots (can be expanded)

    // Find first empty slot, returns -1 if full
    int32_t findEmptySlot() const {
        for (uint32_t i = 0; i < slotCount; ++i) {
            if (slots[i].isEmpty()) return static_cast<int32_t>(i);
        }
        return -1;
    }

    // Find slot with same item and room to stack
    int32_t findStackableSlot(uint32_t itemId, uint32_t maxStack) const {
        for (uint32_t i = 0; i < slotCount; ++i) {
            if (slots[i].itemId == itemId && slots[i].quantity < maxStack) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    // Add item to inventory, returns quantity that couldn't fit (0 = all fit)
    uint32_t addItem(uint32_t itemId, uint32_t quantity, uint32_t maxStackSize) {
        uint32_t remaining = quantity;

        // Try stacking first
        while (remaining > 0) {
            int32_t stackSlot = findStackableSlot(itemId, maxStackSize);
            if (stackSlot < 0) break;
            uint32_t canAdd = maxStackSize - slots[stackSlot].quantity;
            uint32_t toAdd = std::min(remaining, canAdd);
            slots[stackSlot].quantity += toAdd;
            remaining -= toAdd;
        }

        // Fill empty slots
        while (remaining > 0) {
            int32_t emptySlot = findEmptySlot();
            if (emptySlot < 0) break; // Inventory full
            uint32_t toAdd = std::min(remaining, maxStackSize);
            slots[emptySlot].itemId = itemId;
            slots[emptySlot].quantity = toAdd;
            remaining -= toAdd;
        }

        return remaining; // 0 = success
    }

    // Remove items from inventory, returns true if successful
    bool removeItem(uint32_t itemId, uint32_t quantity) {
        // Count total available
        uint32_t total = 0;
        for (uint32_t i = 0; i < slotCount; ++i) {
            if (slots[i].itemId == itemId) {
                total += slots[i].quantity;
            }
        }
        if (total < quantity) return false;

        // Remove from slots
        uint32_t remaining = quantity;
        for (uint32_t i = 0; i < slotCount && remaining > 0; ++i) {
            if (slots[i].itemId == itemId) {
                uint32_t toRemove = std::min(remaining, slots[i].quantity);
                slots[i].quantity -= toRemove;
                remaining -= toRemove;
                if (slots[i].quantity == 0) {
                    slots[i].itemId = 0;
                }
            }
        }
        return true;
    }

    // Count total quantity of an item
    uint32_t countItem(uint32_t itemId) const {
        uint32_t total = 0;
        for (uint32_t i = 0; i < slotCount; ++i) {
            if (slots[i].itemId == itemId) {
                total += slots[i].quantity;
            }
        }
        return total;
    }
};

// Equipped items component — what a player is wearing
struct Equipment {
    uint32_t mainHand{0};
    uint32_t offHand{0};
    uint32_t head{0};
    uint32_t chest{0};
    uint32_t legs{0};
    uint32_t feet{0};
    uint32_t ring{0};
    uint32_t amulet{0};

    // Equip item to slot, returns previously equipped item (0 = empty slot)
    uint32_t equip(EquipSlot slot, uint32_t itemId) {
        uint32_t* target = nullptr;
        switch (slot) {
            case EquipSlot::MainHand: target = &mainHand; break;
            case EquipSlot::OffHand:  target = &offHand; break;
            case EquipSlot::Head:     target = &head; break;
            case EquipSlot::Chest:    target = &chest; break;
            case EquipSlot::Legs:     target = &legs; break;
            case EquipSlot::Feet:     target = &feet; break;
            case EquipSlot::Ring:     target = &ring; break;
            case EquipSlot::Amulet:   target = &amulet; break;
            default: return 0;
        }
        uint32_t old = *target;
        *target = itemId;
        return old;
    }
};

// Player ability loadout — what abilities they have slotted
static constexpr uint32_t MAX_ABILITY_SLOTS = 4;
struct AbilityLoadout {
    uint32_t abilityIds[MAX_ABILITY_SLOTS]{0, 0, 0, 0};

    uint32_t getAbilityInSlot(uint8_t slot) const {
        if (slot == 0 || slot > MAX_ABILITY_SLOTS) return 0;
        return abilityIds[slot - 1]; // slot is 1-indexed
    }
};

// ============================================================================
// QUEST SYSTEM
// ============================================================================

// Quest objective types
enum class QuestObjectiveType : uint8_t {
    KillNPC     = 0,  // Kill N NPCs of a specific type
    CollectItem = 1,  // Collect N of a specific item
    TalkToNPC   = 2,  // Interact with an NPC
    ReachLevel  = 3,  // Reach a specific level
    ExploreZone = 4   // Visit a specific zone
};

// NPC archetype for kill objectives (reuse existing enum)
// Uses NPCArchetype directly

// A single objective within a quest
struct QuestObjective {
    QuestObjectiveType type{QuestObjectiveType::KillNPC};
    uint32_t targetId{0};          // NPC archetype ID, item ID, NPC entity ID, etc.
    uint32_t requiredCount{1};     // How many needed
    char description[64]{0};       // Human-readable description
};

// Quest reward
struct QuestReward {
    uint32_t xpReward{0};          // Experience points
    uint32_t goldReward{0};        // Gold
    uint32_t itemId{0};            // Item reward (0 = none)
    uint32_t itemQuantity{0};      // How many of the item
};

// Quest definition — describes a quest
static constexpr uint32_t MAX_QUEST_OBJECTIVES = 4;
struct QuestDefinition {
    uint32_t questId{0};
    char name[48]{0};
    char description[128]{0};
    char startDialogue[256]{0};    // NPC dialogue when giving quest
    char completionDialogue[256]{0}; // NPC dialogue when turning in
    uint32_t requiredLevel{1};     // Minimum level to accept
    uint32_t prerequisiteQuestId{0}; // Must complete this quest first (0 = none)
    QuestObjective objectives[MAX_QUEST_OBJECTIVES];
    uint32_t objectiveCount{0};
    QuestReward reward{};
    bool repeatable{false};
};

// Per-objective progress tracking
struct ObjectiveProgress {
    uint32_t currentCount{0};
    bool completed{false};
};

// Player's progress on an active quest
static constexpr uint32_t MAX_ACTIVE_QUESTS = 20;
static constexpr uint32_t MAX_COMPLETED_QUESTS = 100;

struct QuestProgress {
    uint32_t questId{0};
    bool accepted{false};
    bool completed{false};
    ObjectiveProgress objectives[MAX_QUEST_OBJECTIVES];
    uint32_t acceptTimeMs{0};      // When the quest was accepted
};

// Player's quest log — tracks active and completed quests
struct QuestLog {
    QuestProgress activeQuests[MAX_ACTIVE_QUESTS];
    uint32_t activeCount{0};
    uint32_t completedQuests[MAX_COMPLETED_QUESTS]; // Just quest IDs
    uint32_t completedCount{0};

    // Check if quest is active
    bool hasActiveQuest(uint32_t questId) const {
        for (uint32_t i = 0; i < activeCount; ++i) {
            if (activeQuests[i].questId == questId && activeQuests[i].accepted) return true;
        }
        return false;
    }

    // Check if quest was completed
    bool hasCompletedQuest(uint32_t questId) const {
        for (uint32_t i = 0; i < completedCount; ++i) {
            if (completedQuests[i] == questId) return true;
        }
        return false;
    }

    // Get active quest progress, nullptr if not found
    QuestProgress* getActiveQuest(uint32_t questId) {
        for (uint32_t i = 0; i < activeCount; ++i) {
            if (activeQuests[i].questId == questId) return &activeQuests[i];
        }
        return nullptr;
    }

    const QuestProgress* getActiveQuest(uint32_t questId) const {
        for (uint32_t i = 0; i < activeCount; ++i) {
            if (activeQuests[i].questId == questId) return &activeQuests[i];
        }
        return nullptr;
    }

    // Add an active quest, returns true if added
    bool addActiveQuest(uint32_t questId, uint32_t currentTimeMs) {
        if (activeCount >= MAX_ACTIVE_QUESTS) return false;
        if (hasActiveQuest(questId)) return false;
        QuestProgress& qp = activeQuests[activeCount++];
        qp = QuestProgress{}; // Zero-initialize
        qp.questId = questId;
        qp.accepted = true;
        qp.acceptTimeMs = currentTimeMs;
        return true;
    }

    // Complete a quest (move from active to completed)
    bool completeQuest(uint32_t questId) {
        // Find and remove from active
        for (uint32_t i = 0; i < activeCount; ++i) {
            if (activeQuests[i].questId == questId) {
                // Shift remaining down
                for (uint32_t j = i; j < activeCount - 1; ++j) {
                    activeQuests[j] = activeQuests[j + 1];
                }
                activeCount--;
                // Add to completed
                if (completedCount < MAX_COMPLETED_QUESTS) {
                    completedQuests[completedCount++] = questId;
                }
                return true;
            }
        }
        return false;
    }
};

// ============================================================================
// CHAT SYSTEM
// ============================================================================

// Chat channel types — determines message routing and visibility
enum class ChatChannel : uint8_t {
    System  = 0,  // Server-to-player notifications (no player sender)
    Local   = 1,  // Zone-only broadcast (visible to nearby players)
    Global  = 2,  // All connected players across all zones
    Whisper = 3,  // Direct message between two players
    Party   = 4,  // Party/group members only
    Guild   = 5   // Guild members only
};

// Chat message — fixed-size for cache efficiency
static constexpr uint32_t CHAT_MESSAGE_MAX_LEN = 256;
static constexpr uint32_t CHAT_SENDER_NAME_MAX = 32;

struct ChatMessage {
    uint32_t messageId{0};                          // Unique message ID (monotonic)
    ChatChannel channel{ChatChannel::Local};        // Channel this message was sent on
    uint32_t senderId{0};                           // Player ID of sender (0 for system)
    uint32_t targetId{0};                           // Target player ID (whisper only)
    uint32_t timestampMs{0};                        // Server timestamp
    char senderName[CHAT_SENDER_NAME_MAX]{0};       // Display name of sender
    char content[CHAT_MESSAGE_MAX_LEN]{0};          // Message text
};

// Per-player chat state
static constexpr uint32_t CHAT_HISTORY_SIZE = 50;   // Recent messages to keep

struct ChatComponent {
    ChatMessage recentMessages[CHAT_HISTORY_SIZE];
    uint32_t messageCount{0};                       // Total messages received
    uint32_t lastMessageTimeMs{0};                  // Last time player sent a message
    uint8_t messagesThisWindow{0};                  // Rate limit counter
    uint32_t rateWindowStartMs{0};                  // Start of current rate window
    bool muted{false};                              // Player is muted

    // Add message to history (ring buffer)
    void addMessage(const ChatMessage& msg) {
        uint32_t idx = messageCount % CHAT_HISTORY_SIZE;
        recentMessages[idx] = msg;
        messageCount++;
    }
};

// Chat system configuration
struct ChatConfig {
    uint32_t maxMessagesPerWindow{5};     // Max messages per rate window
    uint32_t rateWindowMs{10000};         // Rate window duration (10 seconds)
    uint32_t whisperRangeLevelDiff{0};    // Max level diff for whispers (0 = unlimited)
    float localChatRange{50.0f};          // Range in meters for local chat
    bool globalChatEnabled{true};         // Whether global chat is active
    bool systemMessagesEnabled{true};     // Whether system messages are active
};

// ============================================================================
// CRAFTING SYSTEM
// ============================================================================

// A single ingredient required for a recipe
struct CraftingIngredient {
    uint32_t itemId{0};          // Item required
    uint32_t quantity{1};        // How many needed
};

// Maximum ingredients per recipe
static constexpr uint32_t MAX_CRAFTING_INGREDIENTS = 6;

// Crafting recipe definition
struct CraftingRecipe {
    uint32_t recipeId{0};
    char name[48]{0};
    char description[128]{0};
    uint32_t outputItemId{0};           // Item produced
    uint32_t outputQuantity{1};         // How many produced
    CraftingIngredient ingredients[MAX_CRAFTING_INGREDIENTS];
    uint32_t ingredientCount{0};
    uint32_t requiredLevel{1};          // Player level required
    uint32_t requiredProfessionLevel{0}; // Profession skill level required (0 = none)
    uint32_t craftTimeMs{0};            // Time to craft in ms (0 = instant)
    uint32_t goldCost{0};               // Gold required to craft
    uint32_t xpReward{10};              // Profession XP awarded on craft completion
    bool discovered{true};              // Whether recipe shows in UI (false = hidden until learned)
};

// Per-player crafting state
struct CraftingComponent {
    uint32_t currentRecipeId{0};        // Recipe being crafted (0 = idle)
    uint32_t craftStartTimeMs{0};       // When crafting started
    uint32_t craftCount{0};             // Total items crafted lifetime
    uint8_t professionLevel{0};         // Crafting profession level
    uint64_t professionXP{0};           // Profession experience points

    bool isCrafting() const { return currentRecipeId != 0; }

    void startCraft(uint32_t recipeId, uint32_t currentTimeMs) {
        currentRecipeId = recipeId;
        craftStartTimeMs = currentTimeMs;
    }

    void finishCraft() {
        currentRecipeId = 0;
        craftStartTimeMs = 0;
        craftCount++;
    }

    void cancelCraft() {
        currentRecipeId = 0;
        craftStartTimeMs = 0;
    }
};

} // namespace DarkAges
