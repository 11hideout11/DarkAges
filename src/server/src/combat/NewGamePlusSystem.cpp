#include "combat/NewGamePlusSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include "ecs/Component.hpp"
#include "combat/ProgressionCalculator.hpp"

namespace DarkAges {

// Component to store NG+ level on player entity
struct NGPlusComponent {
    uint32_t level{0};  // 0 = normal play, 1 = NG+, 2 = NG++ etc.
    bool isNewGamePlusUnlocked{false};  // Whether player has unlocked NG+ (completed all zones once)
};

uint32_t NewGamePlusSystem::getNGPlusLevel(entt::registry& registry, EntityID player) const {
    if (registry.has<NGPlusComponent>(player)) {
        return registry.get<NGPlusComponent>(player).level;
    }
    return 0;
}

void NewGamePlusSystem::increaseNGPlusLevel(entt::registry& registry, EntityID player) {
    NGPlusComponent& comp = registry.get_or_emplace<NGPlusComponent>(player);
    comp.level++;
    // When player enters NG+, unlock new game plus mode (optional)
    comp.isNewGamePlusUnlocked = true;
    
    // Recalculate player stats to account for increased difficulty scaling
    if (calculator_) {
        calculator_->recalculateAllStats(registry, player);
    }
}

void NewGamePlusSystem::resetNGPlusLevel(entt::registry& registry, EntityID player) {
    if (registry.has<NGPlusComponent>(player)) {
        registry.patch<NGPlusComponent>(player, [](auto& comp) {
            comp.level = 0;
            comp.isNewGamePlusUnlocked = false;
        });
    }
    // Recalculate stats after reset
    if (calculator_) {
        calculator_->recalculateAllStats(registry, player);
    }
}

} // namespace DarkAges
