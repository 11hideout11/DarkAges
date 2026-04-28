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
    uint8_t interact{0};  // Interact with NPCs (E key)

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
    uint8_t tradeAction{0};     // 0=no trade, 1=request, 2=accept, 3=decline, 4=cancel, 5=lock, 6=unlock, 7=confirm
    uint8_t tradeSlotIndex{0};  // For addItem: inventory slot, for removeItem: trade slot
    uint32_t tradeItemId{0};    // Item to add to trade offer (0=none)
    uint32_t tradeQuantity{0};  // Quantity for trade item
    float tradeGoldOffer{0.0f}; // Gold to offer in trade

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
    uint32_t lastGlobalCooldownTime{0};  // Last time any ability/attack was used (for GCD)
    bool isDead{false};

    // FSM state
    uint8_t currentState{0};     // 0=Idle,1=AttackWindup,2=AttackActive,3=Cooldown,4=Stunned,5=Dead
    float stateTimer{0.0f};      // Seconds elapsed in current state

    [[nodiscard]] float healthPercent() const {
        return static_cast<float>(health) / static_cast<float>(maxHealth) * 100.0f;
    }
};

// [COMBAT_AGENT] Target lock state for lock-on targeting system
enum class LockState : uint8_t {
    None = 0,       // No lock active
    Pending = 1,    // Lock request sent, awaiting confirmation
    Confirmed = 2   // Lock confirmed by server
};

// [COMBAT_AGENT] Component storing target lock information
struct TargetLock {
    EntityID lockedTarget{entt::null};   // Currently locked target entity
    LockState lockState{LockState::None}; // Current lock state
    uint32_t lockTimeMs{0};              // Timestamp when lock was confirmed
    uint32_t lastLockAttempt{0};         // Timestamp of last lock attempt (for rate limiting)
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


 struct Interactable {
     float interactionRange{3.0f};          // Max interaction distance (meters)
     char promptText[64]{"Press E to interact"}; // Prompt shown to player
     uint32_t dialogueTreeId{0};           // Associated dialogue tree ID
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

 // Player entity tag (marks an entity as a player character)
 struct PlayerComponent {};

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
    constexpr uint32_t HITBOX   = 1 << 2;   // 4 — attack hit detection area
    constexpr uint32_t HURTBOX  = 1 << 3;   // 8 — damage reception area
    constexpr uint32_t PROJECTILE = 1 << 4; // 16
    constexpr uint32_t STATIC   = 1 << 5;   // 32
    constexpr uint32_t TRIGGER  = 1 << 6;   // 64

    // Default collidesWith masks
    constexpr uint32_t PLAYER_DEFAULT   = PLAYER | NPC | STATIC | HITBOX | HURTBOX;
    constexpr uint32_t NPC_DEFAULT      = PLAYER | NPC | PROJECTILE | STATIC | HITBOX | HURTBOX;
    constexpr uint32_t PROJECTILE_DEFAULT = PLAYER | NPC | STATIC | HURTBOX;
    constexpr uint32_t STATIC_DEFAULT   = PLAYER | NPC;
    constexpr uint32_t HITBOX_DEFAULT     = HURTBOX | STATIC;
    constexpr uint32_t HURTBOX_DEFAULT    = HITBOX | PROJECTILE;
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
    static CollisionLayer makeHitbox(EntityID owner = entt::null) {
        return CollisionLayer{CollisionLayerMask::HITBOX, CollisionLayerMask::HITBOX_DEFAULT, owner};
    }
    static CollisionLayer makeHurtbox(EntityID owner = entt::null) {
        return CollisionLayer{CollisionLayerMask::HURTBOX, CollisionLayerMask::HURTBOX_DEFAULT, owner};
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

// ============================================================================
// PARTY SYSTEM
// ============================================================================

// Party role
enum class PartyRole : uint8_t {
    None    = 0,  // Not in a party
    Member  = 1,  // Regular member
    Leader  = 2   // Party leader
};

// Party component — attached to players in a party
struct PartyComponent {
    uint32_t partyId{0};                // 0 = not in a party
    PartyRole role{PartyRole::None};
    bool shareXP{true};                 // Whether XP is shared with party
};

// Party configuration
static constexpr uint32_t MAX_PARTY_SIZE = 5;
static constexpr float PARTY_XP_SHARE_RANGE = 100.0f;  // Meters — XP sharing range

// ============================================================================
// GUILD SYSTEM
// ============================================================================

// Guild rank
enum class GuildRank : uint8_t {
    None    = 0,  // Not in a guild
    Member  = 1,  // Regular member
    Officer = 2,  // Officer — can invite/kick members
    Leader  = 3   // Guild leader — full control
};

// Guild component — attached to players in a guild
struct GuildComponent {
    uint32_t guildId{0};                // 0 = not in a guild
    GuildRank rank{GuildRank::None};
    uint32_t joinTimeMs{0};             // When the player joined
};

// Guild configuration
static constexpr uint32_t MAX_GUILD_SIZE = 100;
static constexpr uint32_t GUILD_NAME_MAX = 32;

// ============================================================================
// TRADING SYSTEM
// ============================================================================

// Trade state machine
enum class TradeState : uint8_t {
    None       = 0,  // Not trading
    Pending    = 1,  // Trade request sent, awaiting response
    Active     = 2,  // Both players in trade, adding items
    Locked     = 3,  // One side locked (waiting for other)
    BothLocked = 4,  // Both sides locked, awaiting confirmation
    Confirmed  = 5   // Both confirmed, trade executing
};

// Maximum items a player can offer in a single trade
static constexpr uint32_t MAX_TRADE_SLOTS = 8;

// A single offered item in a trade
struct TradeSlot {
    uint32_t itemId{0};
    uint32_t quantity{0};

    bool isEmpty() const { return itemId == 0 || quantity == 0; }
};

// Per-player trade state
struct TradeComponent {
    TradeState state{TradeState::None};
    EntityID tradePartner{entt::null};         // The other player
    TradeSlot offeredItems[MAX_TRADE_SLOTS];   // Items this player is offering
    float offeredGold{0.0f};                   // Gold this player is offering
    bool locked{false};                         // Player has locked their offer
    bool confirmed{false};                      // Player has confirmed the trade

    // Count non-empty offered items
    uint32_t offeredItemCount() const {
        uint32_t count = 0;
        for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
            if (!offeredItems[i].isEmpty()) count++;
        }
        return count;
    }

    // Find first empty trade slot
    int32_t findEmptySlot() const {
        for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
            if (offeredItems[i].isEmpty()) return static_cast<int32_t>(i);
        }
        return -1;
    }

    // Reset to clean state
    void reset() {
        state = TradeState::None;
        tradePartner = entt::null;
        for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
            offeredItems[i] = TradeSlot{};
        }
        offeredGold = 0.0f;
        locked = false;
        confirmed = false;
    }
};

// Trade configuration
struct TradeConfig {
    float maxTradeDistance{10.0f};   // Max distance between trading players (meters)
    uint32_t tradeTimeoutMs{60000}; // Trade expires after 60s of inactivity
};

// ============================================================================
// ZONE EVENT SYSTEM
// ============================================================================

// Zone event type — determines the event structure
enum class ZoneEventType : uint8_t {
    WorldBoss     = 0,  // Single powerful boss spawns, all players can attack
    WaveDefense   = 1,  // Waves of enemies, survive all waves
    Territory     = 2,  // Capture and hold a point
    Invasion      = 3,  // NPCs attack a location, players defend
    TimedKill     = 4   // Kill as many as possible in a time limit
};

// Event objective types
enum class EventObjectiveType : uint8_t {
    KillNPC       = 0,  // Kill N NPCs of a specific archetype
    KillBoss      = 1,  // Kill the world boss
    SurviveTime   = 2,  // Survive for N seconds
    TotalDamage   = 3,  // Deal N total damage
    PlayerDeaths  = 4,  // Keep player deaths below N
    ProtectEntity = 5   // Keep an entity alive
};

// A single objective within an event phase
struct ZoneEventObjective {
    EventObjectiveType type{EventObjectiveType::KillNPC};
    uint32_t targetId{0};           // NPC archetype ID, boss entity ID, etc.
    uint32_t requiredCount{1};      // Target count to complete
    char description[64]{0};        // Human-readable description
};

// Maximum objectives per phase
static constexpr uint32_t MAX_EVENT_OBJECTIVES = 4;

// Event phase definition — one stage of a multi-phase event
struct ZoneEventPhaseDefinition {
    uint32_t phaseId{0};
    char name[48]{0};
    char description[128]{0};
    uint32_t durationMs{0};         // Phase time limit (0 = no limit)
    uint32_t npcArchetypeId{0};     // NPC type to spawn this phase (0 = none)
    uint32_t npcCount{0};           // How many NPCs to spawn
    uint8_t npcLevel{1};            // Level of spawned NPCs
    uint32_t bossNpcArchetypeId{0}; // Boss NPC to spawn (0 = none)
    uint8_t bossLevel{1};           // Boss level
    ZoneEventObjective objectives[MAX_EVENT_OBJECTIVES];
    uint32_t objectiveCount{0};
};

// Maximum phases per event
static constexpr uint32_t MAX_EVENT_PHASES = 4;

// Event reward
struct ZoneEventReward {
    uint32_t xpReward{0};           // XP for participation
    uint32_t goldReward{0};         // Gold for participation
    uint32_t bonusItemId{0};        // Bonus item for top contributor (0 = none)
    uint32_t bonusItemQuantity{0};
};

// Zone event definition — describes a complete event
struct ZoneEventDefinition {
    uint32_t eventId{0};
    char name[48]{0};
    char description[128]{0};
    ZoneEventType type{ZoneEventType::WorldBoss};
    ZoneEventPhaseDefinition phases[MAX_EVENT_PHASES];
    uint32_t phaseCount{0};
    ZoneEventReward reward{};
    uint32_t cooldownMs{300000};    // Time before event can repeat (5 min default)
    float spawnX{0.0f};             // Event spawn location
    float spawnZ{0.0f};
    float spawnRadius{20.0f};       // Spawn area radius
    uint32_t requiredPlayers{1};    // Min players to start (0 = auto-start)
    uint32_t maxParticipants{0};    // Max participants (0 = unlimited)
};

// Event state
enum class ZoneEventState : uint8_t {
    Inactive    = 0,  // Not running
    Announcing  = 1,  // Announcing event start to players
    Active      = 2,  // Running, players participating
    Completing  = 3,  // All objectives met, distributing rewards
    Cooldown    = 4   // Cooling down before next availability
};

// Per-player participation tracking
struct EventParticipation {
    uint64_t playerId{0};
    uint32_t totalDamage{0};        // Total damage dealt during event
    uint32_t kills{0};              // Kills during event
    uint32_t deaths{0};             // Deaths during event
    bool contributed{false};        // Whether player did anything meaningful
};

// Maximum participants tracked per event
static constexpr uint32_t MAX_EVENT_PARTICIPANTS = 50;

// Active event instance state
struct ActiveZoneEvent {
    ZoneEventState state{ZoneEventState::Inactive};
    const ZoneEventDefinition* definition{nullptr};  // Pointer to definition (not owned)
    uint32_t currentPhase{0};                           // Current phase index
    uint32_t phaseStartTimeMs{0};                       // When current phase started
    uint32_t eventStartTimeMs{0};                       // When event started
    uint32_t lastStateChangeMs{0};                      // Last state transition time
    uint32_t announcementEndTimeMs{0};                  // When announcement phase ends
    EntityID bossEntity{entt::null};                    // Spawned boss entity (if any)
    EventParticipation participants[MAX_EVENT_PARTICIPANTS];
    uint32_t participantCount{0};

    // Per-objective progress (aggregated across all participants)
    uint32_t objectiveProgress[MAX_EVENT_OBJECTIVES]{0, 0, 0, 0};

    bool isActive() const {
        return state == ZoneEventState::Active || state == ZoneEventState::Announcing;
    }

    bool isComplete() const {
        return state == ZoneEventState::Completing;
    }

    void reset() {
        state = ZoneEventState::Inactive;
        definition = nullptr;
        currentPhase = 0;
        phaseStartTimeMs = 0;
        eventStartTimeMs = 0;
        lastStateChangeMs = 0;
        announcementEndTimeMs = 0;
        bossEntity = entt::null;
        participantCount = 0;
        for (uint32_t i = 0; i < MAX_EVENT_OBJECTIVES; ++i) {
            objectiveProgress[i] = 0;
        }
        for (uint32_t i = 0; i < MAX_EVENT_PARTICIPANTS; ++i) {
            participants[i] = EventParticipation{};
        }
    }

    // Find or add participant, returns pointer to participation record
    EventParticipation* findParticipant(uint64_t playerId) {
        for (uint32_t i = 0; i < participantCount; ++i) {
            if (participants[i].playerId == playerId) return &participants[i];
        }
        return nullptr;
    }

    const EventParticipation* findParticipant(uint64_t playerId) const {
        for (uint32_t i = 0; i < participantCount; ++i) {
            if (participants[i].playerId == playerId) return &participants[i];
        }
        return nullptr;
    }

    EventParticipation* addParticipant(uint64_t playerId) {
        if (participantCount >= MAX_EVENT_PARTICIPANTS) return nullptr;
        if (auto* existing = findParticipant(playerId)) return existing;
        EventParticipation& p = participants[participantCount++];
        p.playerId = playerId;
        return &p;
    }
};

// Zone event configuration
struct ZoneEventConfig {
    uint32_t announcementDurationMs{15000};     // How long to announce before start (15s)
    float participationRange{100.0f};           // Max distance to count as participant
    uint32_t minDamageForReward{100};           // Min damage dealt to earn rewards
    float topContributorBonusMultiplier{2.0f};  // Reward multiplier for top contributor
};

// Per-player event component — tracks current event participation
struct ZoneEventComponent {
    uint32_t activeEventId{0};      // Event the player is in (0 = none)
    bool inEvent{false};            // Whether player is participating
    uint32_t joinTimeMs{0};         // When player joined
};

// ============================================================================
// NPC DIALOGUE SYSTEM
// ============================================================================

// Dialogue condition types — controls when nodes/responses are available
enum class DialogueConditionType : uint8_t {
    None          = 0,  // Always available
    HasQuest      = 1,  // Player has a specific active quest
    QuestComplete = 2,  // Player has completed a specific quest
    QuestNotStarted = 3, // Player has NOT started a specific quest
    MinLevel      = 4,  // Player level >= required level
    HasItem       = 5,  // Player has a specific item in inventory
    NoCondition   = 6   // Explicit no-condition (alias for None)
};

// A condition that gates dialogue nodes or responses
struct DialogueCondition {
    DialogueConditionType type{DialogueConditionType::None};
    uint32_t targetId{0};       // Quest ID, item ID, or level depending on type
    uint32_t quantity{1};       // Item quantity (for HasItem)
};

// Dialogue action types — triggered when a response is selected
enum class DialogueActionType : uint8_t {
    None            = 0,  // No action
    GiveQuest       = 1,  // Give a quest to the player
    CompleteQuest   = 2,  // Complete a quest (turn-in)
    GiveItem        = 3,  // Give an item to the player
    TakeItem        = 4,  // Remove an item from the player
    GiveGold        = 5,  // Give gold to the player
    CloseDialogue   = 6   // End the conversation
};

// An action triggered by selecting a dialogue response
struct DialogueAction {
    DialogueActionType type{DialogueActionType::None};
    uint32_t targetId{0};       // Quest ID, item ID, etc.
    uint32_t quantity{1};       // Item quantity or gold amount
};

// Maximum responses per dialogue node
static constexpr uint32_t MAX_DIALOGUE_RESPONSES = 6;
// Maximum actions per response
static constexpr uint32_t MAX_DIALOGUE_ACTIONS = 3;
// Maximum conditions per node or response
static constexpr uint32_t MAX_DIALOGUE_CONDITIONS = 3;
// Maximum nodes per dialogue tree
static constexpr uint32_t MAX_DIALOGUE_NODES = 16;
// Max dialogue text length
static constexpr uint32_t DIALOGUE_TEXT_MAX_LEN = 256;
// Max NPC name length
static constexpr uint32_t NPC_NAME_MAX_LEN = 32;

// A player response option within a dialogue node
struct DialogueResponse {
    char text[128]{0};                                          // Response text shown to player
    uint32_t nextNodeId{0};                                     // Node to transition to (0 = end conversation)
    DialogueCondition conditions[MAX_DIALOGUE_CONDITIONS];      // Conditions to show this response
    uint32_t conditionCount{0};
    DialogueAction actions[MAX_DIALOGUE_ACTIONS];               // Actions triggered on selection
    uint32_t actionCount{0};
};

// A single node in a dialogue tree
struct DialogueNode {
    uint32_t nodeId{0};                                         // Unique within tree
    char text[DIALOGUE_TEXT_MAX_LEN]{0};                        // NPC says this
    DialogueResponse responses[MAX_DIALOGUE_RESPONSES];         // Player choices
    uint32_t responseCount{0};
    DialogueCondition conditions[MAX_DIALOGUE_CONDITIONS];      // Conditions to show this node
    uint32_t conditionCount{0};
    bool isEnd{false};                                          // If true, conversation ends after this node
};

// Complete dialogue tree — attached to an NPC
struct DialogueTree {
    uint32_t treeId{0};                                         // Unique dialogue tree ID
    char npcName[NPC_NAME_MAX_LEN]{0};                          // NPC display name
    DialogueNode nodes[MAX_DIALOGUE_NODES];                     // All nodes in the tree
    uint32_t nodeCount{0};
    uint32_t greetingNodeId{0};                                 // Starting node ID

    const DialogueNode* findNode(uint32_t nodeId) const {
        for (uint32_t i = 0; i < nodeCount; ++i) {
            if (nodes[i].nodeId == nodeId) return &nodes[i];
        }
        return nullptr;
    }
};

// Per-player dialogue state — tracks active conversation
struct DialogueComponent {
    uint32_t activeTreeId{0};       // Active dialogue tree (0 = no conversation)
    uint32_t currentNodeId{0};      // Current node in the tree
    EntityID npcEntity{entt::null}; // NPC being talked to
    bool inConversation{false};     // Whether player is in a dialogue

    void startConversation(uint32_t treeId, uint32_t startNodeId, EntityID npc) {
        activeTreeId = treeId;
        currentNodeId = startNodeId;
        npcEntity = npc;
        inConversation = true;
    }

    void endConversation() {
        activeTreeId = 0;
        currentNodeId = 0;
        npcEntity = entt::null;
        inConversation = false;
    }
};

} // namespace DarkAges
