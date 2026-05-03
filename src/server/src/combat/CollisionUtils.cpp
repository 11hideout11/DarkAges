#include "combat/HitboxComponent.hpp"
#include "combat/HurtboxComponent.hpp"
#include "combat/CollisionLayerManager.hpp"
#include "ecs/CoreTypes.hpp"
#include <cmath>
#include <glm/glm.hpp>

namespace DarkAges::combat {

bool checkHitboxHurtboxCollision(glm::vec3 hitboxWorldPos, float hitboxRadius, float hitboxHeight,
                                  glm::vec3 hurtboxWorldPos, float hurtboxRadius, float hurtboxHeight,
                                  uint32_t hitboxLayer, uint32_t hurtboxLayer,
                                  const CollisionLayerManager& layerManager) {
    // Layer compatibility
    if (!layerManager.canCollide(hitboxLayer, hurtboxLayer)) {
        return false;
    }

    // Horizontal distance (XZ plane)
    float dx = hitboxWorldPos.x - hurtboxWorldPos.x;
    float dz = hitboxWorldPos.z - hurtboxWorldPos.z;
    float horizDistSq = dx*dx + dz*dz;
    float radiusSum = hitboxRadius + hurtboxRadius;
    if (horizDistSq > radiusSum * radiusSum) {
        return false;
    }

    // Vertical overlap (assuming cylinders)
    float halfH1 = hitboxHeight * 0.5f;
    float halfH2 = hurtboxHeight * 0.5f;
    float dy = std::abs(hitboxWorldPos.y - hurtboxWorldPos.y);
    if (dy > (halfH1 + halfH2)) {
        return false;
    }

    return true;
}

} // namespace DarkAges::combat
