#include "combat/CollisionLayerManager.hpp"

#include <utility> // for std::swap
#include "ecs/CoreTypes.hpp" // for CollisionLayerMask

namespace DarkAges::combat {

uint64_t CollisionLayerManager::makePair(uint32_t a, uint32_t b) {
    if (a > b) std::swap(a, b);
    return (static_cast<uint64_t>(a) << 32) | b;
}

void CollisionLayerManager::registerLayer(const std::string& name, uint32_t bitmask) {
    layerNames_[name] = bitmask;
}

void CollisionLayerManager::setCollisionRule(uint32_t layerA, uint32_t layerB, bool canCollide) {
    uint64_t key = makePair(layerA, layerB);
    if (canCollide) {
        allowedPairs_.insert(key);
    } else {
        allowedPairs_.erase(key);
    }
}

bool CollisionLayerManager::canCollide(uint32_t layerA, uint32_t layerB) const {
    uint64_t key = makePair(layerA, layerB);
    return allowedPairs_.find(key) != allowedPairs_.end();
}

CollisionLayerManager& getGlobalCollisionLayerManager() {
    static CollisionLayerManager instance;
    static bool initialized = false;
    if (!initialized) {
        // Optional: register layer names for debugging (not required)
        // instance.registerLayer("DEFAULT", CollisionLayerMask::DEFAULT);
        // instance.registerLayer("PLAYER", CollisionLayerMask::PLAYER);
        // instance.registerLayer("NPC", CollisionLayerMask::NPC);
        // instance.registerLayer("HITBOX", CollisionLayerMask::HITBOX);
        // instance.registerLayer("HURTBOX", CollisionLayerMask::HURTBOX);
        // instance.registerLayer("PROJECTILE", CollisionLayerMask::PROJECTILE);
        // instance.registerLayer("STATIC", CollisionLayerMask::STATIC);
        // instance.registerLayer("TRIGGER", CollisionLayerMask::TRIGGER);

        // Establish collision rules per the official collision matrix
        // Hitbox collides with Hurtbox only
        instance.setCollisionRule(CollisionLayerMask::HITBOX, CollisionLayerMask::HURTBOX, true);
        initialized = true;
    }
    return instance;
}

} // namespace DarkAges::combat
