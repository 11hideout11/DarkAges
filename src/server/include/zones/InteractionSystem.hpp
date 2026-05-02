// ============================================================================
// Interaction System - Handles player interaction with game entities
// ============================================================================

#pragma once

#include "ecs/InteractionComponent.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <vector>
#include <optional>
#include <memory>
#include <functional>

namespace DarkAges {

// Forward declarations
class ZoneServer;
class DialogueSystem;

// Interaction event types
enum class InteractionEventType : uint8_t {
    None = 0,
    Start = 1,           // Begin interaction
    Complete = 2,        // Interaction complete
    Cancel = 3,          // Interaction cancelled
    DistanceTooFar = 4,    // Target out of range
    Cooldown = 5,         // On cooldown
    MissingRequirement = 6  // Missing required item/quest
};

// Interaction result
struct InteractionResult {
    InteractionEventType eventType{InteractionEventType::None};
    EntityID player{entt::null};
    EntityID target{entt::null};
    InteractionType targetType{InteractionType::None};
    uint32_t dialogueTreeId{0};
    std::string message;  // For failure messages
};

// Interaction system - manages entity interactions
class InteractionSystem {
public:
    // Callback for interaction events
    using InteractionCallback = std::function<void(const InteractionResult&)>;
    
public:
    InteractionSystem();
    ~InteractionSystem();
    
    // Initialize with zone server
    void initialize(ZoneServer* zoneServer);
    
    // Update - called every tick
    void update(uint32_t currentTick);
    
    // Process interaction attempt from player
    // Returns interaction result
    InteractionResult processInteraction(EntityID player, EntityID target);
    
    // Check if player can interact with target (without executing)
    // Returns the result of the check (eventType indicates success/failure)
    InteractionResult checkInteraction(EntityID player, EntityID target) const;
    
    // Get interactable entities within range of position
    std::vector<EntityID> getInteractablesInRange(
        const Position& position,
        float maxRange
    ) const;
    
    // Set callback for interaction events
    void setOnInteraction(InteractionCallback callback);
    
    // Send interaction result to client
    void sendInteractionResult(ConnectionID connectionId, const InteractionResult& result);
    
    // Trigger dialogue for NPC interaction
    void startDialogue(EntityID player, EntityID npc, uint32_t dialogueTreeId);
    
    // Handle interaction key press (E key)
    void handleInteractionKey(EntityID player);
    
    // Clear cooldowns for entity
    void clearCooldowns(EntityID entity);
    
private:
    // Check distance between entities
    bool isInRange(EntityID player, EntityID target, float maxRange) const;
    
    // Check and apply cooldown
    bool checkAndApplyCooldown(EntityID target);
    
    // Check required quest
    bool hasRequiredQuest(EntityID player, uint32_t questId) const;
    
    // Check required item
    bool hasRequiredItem(EntityID player, uint32_t itemId) const;
    
private:
    ZoneServer* zoneServer_{nullptr};
    std::optional<InteractionEventType> eventType;
    InteractionCallback onInteraction_;
    std::unordered_map<EntityID, uint32_t> cooldowns_;
    std::unordered_map<EntityID, EntityID> nearestInteractable_;
};

} // namespace DarkAges