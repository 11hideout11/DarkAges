// ============================================================================
// Interaction Component - Marks entities as interactable
// ============================================================================

#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace DarkAges {

// Interaction types
enum class InteractionType : uint8_t {
    None = 0,
    NPC = 1,              // Can talk to NPC
    Chest = 2,            // Loot container
    Door = 3,             // Open/close door
    Sign = 4,              // Read sign
    Portal = 5,             // Teleport to other zone
    CraftingStation = 6,     // Crafting interface
    Merchant = 7,           // Buy/sell interface
    Vendor = 8,              // Item vendor
    Banker = 9,             // Banking interface
    Trainer = 10             // Skill training
};

// Interaction component - attach to entities that can be interacted with
struct InteractionComponent {
    InteractionType type{InteractionType::None};
    
    // Interaction settings
    float interactionRange{3.0f};           // Max distance to interact (meters)
    uint32_t requiredItemId{0};              // Item required to interact (0 = none)
    uint32_t requiredQuestId{0};              // Quest required to interact (0 = none)
    
    // Display
    char promptText[64]{"Press E to interact"};   // UI prompt text
    uint32_t dialogueTreeId{0};               // Dialogue tree for NPCs
    
    // State
    bool isActive{true};                       // Can currently interact
    uint8_t cooldownSeconds{0};              // Cooldown between interactions
    
    // Default constructor
    InteractionComponent() = default;
    
    // Constructor with type
    explicit InteractionComponent(InteractionType interactionType)
        : type(interactionType) {}
    
    // Full constructor
    InteractionComponent(InteractionType interactionType, float range, uint32_t dialogueId)
        : type(interactionType)
        , interactionRange(range)
        , dialogueTreeId(dialogueId) {}
};

// Helper to check if entity is interactable
[[nodiscard]] inline bool isInteractable(const InteractionComponent& ic) noexcept {
    return ic.type != InteractionType::None && ic.isActive;
}

// Helper to get prompt text
[[nodiscard]] inline std::string_view getPrompt(const InteractionComponent& ic) noexcept {
    return std::string_view(ic.promptText);
}

} // namespace DarkAges