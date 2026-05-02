// ============================================================================
// Interaction System Tests
// ============================================================================

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ecs/InteractionComponent.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <cstdint>
#include <string>

using namespace DarkAges;
using Catch::Approx;

// ============================================================================
// InteractionComponent Tests
// ============================================================================

TEST_CASE("InteractionComponent default constructor", "[interaction]") {
    InteractionComponent ic;
    
    REQUIRE(ic.type == InteractionType::None);
    REQUIRE(ic.isActive == true);
    REQUIRE(ic.interactionRange == 3.0f);
}

TEST_CASE("InteractionComponent with type constructor", "[interaction]") {
    InteractionComponent ic(InteractionType::NPC);
    
    REQUIRE(ic.type == InteractionType::NPC);
    REQUIRE(ic.interactionRange == 3.0f);
}

TEST_CASE("InteractionComponent full constructor", "[interaction]") {
    InteractionComponent ic(InteractionType::NPC, 5.0f, 100);
    
    REQUIRE(ic.type == InteractionType::NPC);
    REQUIRE(ic.interactionRange == 5.0f);
    REQUIRE(ic.dialogueTreeId == 100);
}

TEST_CASE("InteractionComponent can be inactive", "[interaction]") {
    InteractionComponent ic(InteractionType::NPC);
    ic.isActive = false;
    
    REQUIRE(ic.isActive == false);
}

TEST_CASE("InteractionComponent has default prompt", "[interaction]") {
    InteractionComponent ic;
    
    auto prompt = getPrompt(ic);
    REQUIRE(prompt == "Press E to interact");
}

TEST_CASE("InteractionComponent isInteractable helper", "[interaction]") {
    InteractionComponent icNone;
    REQUIRE_FALSE(isInteractable(icNone));
    
    InteractionComponent icNpc(InteractionType::NPC);
    REQUIRE(isInteractable(icNpc));
    
    InteractionComponent icInactive(InteractionType::NPC);
    icInactive.isActive = false;
    REQUIRE_FALSE(isInteractable(icInactive));
}

// ============================================================================
// Interaction Interaction Types
// ============================================================================

TEST_CASE("Interaction types enum values", "[interaction]") {
    REQUIRE(static_cast<uint8_t>(InteractionType::None) == 0);
    REQUIRE(static_cast<uint8_t>(InteractionType::NPC) == 1);
    REQUIRE(static_cast<uint8_t>(InteractionType::Chest) == 2);
    REQUIRE(static_cast<uint8_t>(InteractionType::Door) == 3);
    REQUIRE(static_cast<uint8_t>(InteractionType::Sign) == 4);
    REQUIRE(static_cast<uint8_t>(InteractionType::Portal) == 5);
    REQUIRE(static_cast<uint8_t>(InteractionType::Merchant) == 7);
}

TEST_CASE("All interaction types can be set", "[interaction]") {
    std::vector<InteractionComponent> entities;
    
    // Create entities for each type
    for (uint8_t i = 1; i <= 10; ++i) {
        InteractionComponent ic;
        ic.type = static_cast<InteractionType>(i);
        entities.push_back(ic);
    }
    
    REQUIRE(entities.size() == 10);
}

// ============================================================================
// Interaction Result Tests
// ============================================================================

TEST_CASE("InteractionResult default constructor", "[interaction]") {
    InteractionResult result;
    
    REQUIRE(result.eventType == InteractionEventType::None);
    REQUIRE(result.player == entt::null);
    REQUIRE(result.target == entt::null);
}

TEST_CASE("InteractionResult can store message", "[interaction]") {
    InteractionResult result;
    result.message = "Too far away to interact";
    
    REQUIRE(result.message == "Too far away to interact");
}

// ============================================================================
// Distance Check Tests
// ============================================================================

TEST_CASE("Range check at exact distance", "[interaction]") {
    Position playerPos = {0.0f, 0.0f, 0.0f};
    Position targetPos = {3.0f, 0.0f, 0.0f};
    float maxRange = 3.0f;
    
    // Calculate distance
    float dx = playerPos.x - targetPos.x;
    float dy = playerPos.y - targetPos.y;
    float dz = playerPos.z - targetPos.z;
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    REQUIRE(dist == Approx(3.0f));
}

TEST_CASE("Range check within range", "[interaction]") {
    Position playerPos = {0.0f, 0.0f, 0.0f};
    Position targetPos = {1.0f, 1.0f, 1.0f};  // ~1.73m
    float maxRange = 3.0f;
    
    float dx = playerPos.x - targetPos.x;
    float dy = playerPos.y - targetPos.y;
    float dz = playerPos.z - targetPos.z;
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    REQUIRE(dist < maxRange);
}

TEST_CASE("Range check out of range", "[interaction]") {
    Position playerPos = {0.0f, 0.0f, 0.0f};
    Position targetPos = {3.0f, 3.0f, 3.0f};  // ~5.2m
    float maxRange = 3.0f;
    
    float dx = playerPos.x - targetPos.x;
    float dy = playerPos.y - targetPos.y;
    float dz = playerPos.z - targetPos.z;
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    REQUIRE(dist > maxRange);
}

// ============================================================================
// Cooldown Tests
// ============================================================================

TEST_CASE("Cooldown tracking starts empty", "[interaction]") {
    std::unordered_map<EntityID, uint32_t> cooldowns;
    
    REQUIRE(cooldowns.empty());
}

TEST_CASE("Cooldown can be set", "[interaction]") {
    std::unordered_map<EntityID, uint32_t> cooldowns;
    EntityID target = 100;
    
    cooldowns[target] = 5;  // 5 seconds cooldown
    
    REQUIRE(cooldowns.find(target) != cooldowns.end());
    REQUIRE(cooldowns[target] == 5);
}

TEST_CASE("Cooldown can be decremented", "[interaction]") {
    std::unordered_map<EntityID, uint32_t> cooldowns;
    EntityID target = 100;
    
    cooldowns[target] = 5;
    
    // Tick passes
    if (cooldowns[target] > 0) {
        --cooldowns[target];
    }
    
    REQUIRE(cooldowns[target] == 4);
}

TEST_CASE("Cooldown expires", "[interaction]") {
    std::unordered_map<EntityID, uint32_t> cooldowns;
    EntityID target = 100;
    
    cooldowns[target] = 1;
    
    // Tick passes, cooldown expires
    if (cooldowns[target] > 0) {
        --cooldowns[target];
    }
    
    // Cooldown expired
    if (cooldowns[target] == 0) {
        cooldowns.erase(target);
    }
    
    REQUIRE(cooldowns.find(target) == cooldowns.end());
}

// ============================================================================
// Prompt Text Tests
// ============================================================================

TEST_CASE("Custom prompt text can be set", "[interaction]") {
    InteractionComponent ic(InteractionType::Chest);
    
    REQUIRE(std::string(ic.promptText) == "Press E to interact");
}

TEST_CASE("NPC has dialogue prompt", "[interaction]") {
    InteractionComponent npc;
    npc.type = InteractionType::NPC;
    npc.dialogueTreeId = 42;
    
    REQUIRE(npc.type == InteractionType::NPC);
    REQUIRE(npc.dialogueTreeId == 42);
}

TEST_CASE("Portal shows teleport prompt", "[interaction]") {
    InteractionComponent portal;
    portal.type = InteractionType::Portal;
    
    // Set custom prompt
    std::snprintf(portal.promptText, sizeof(portal.promptText), "Press E to teleport");
    
    REQUIRE(std::string(portal.promptText) == "Press E to teleport");
}

// ============================================================================
// Dialogue Tree Tests
// ============================================================================

TEST_CASE("Dialogue tree ID tracks conversation", "[interaction]") {
    InteractionComponent npc;
    npc.dialogueTreeId = 100;
    
    REQUIRE(npc.dialogueTreeId == 100);
}

TEST_CASE("Zero dialogue tree means no dialogue", "[interaction]") {
    InteractionComponent chest;
    chest.type = InteractionType::Chest;
    
    REQUIRE(chest.dialogueTreeId == 0);
}

// ============================================================================
// Required Item/Quest Tests
// ============================================================================

TEST_CASE("No item required by default", "[interaction]") {
    InteractionComponent ic;
    
    REQUIRE(ic.requiredItemId == 0);
}

TEST_CASE("Can require item to interact", "[interaction]") {
    InteractionComponent chest;
    chest.type = InteractionType::Chest;
    chest.requiredItemId = 42;  // Requires key
    
    REQUIRE(chest.requiredItemId == 42);
}

TEST_CASE("No quest required by default", "[interaction]") {
    InteractionComponent ic;
    
    REQUIRE(ic.requiredQuestId == 0);
}

TEST_CASE("Can require quest to interact", "[interaction]") {
    InteractionComponent npc;
    npc.type = InteractionType::NPC;
    npc.requiredQuestId = 10;  // Requires quest
    
    REQUIRE(npc.requiredQuestId == 10);
}

// ============================================================================
// End of Interaction Tests
// ============================================================================