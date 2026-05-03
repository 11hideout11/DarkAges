#pragma once

#include <glm/glm.hpp>
#include <cstdint>

#include "ecs/CoreTypes.hpp"

namespace DarkAges::combat {

class CollisionLayerManager; // forward declaration

struct HitboxComponent {
    glm::vec3 offset{0.0f, 0.9f, 0.0f};
    float radius{1.5f};
    float height{1.8f};
    uint32_t layer{CollisionLayerMask::HITBOX};
    bool isActive{false};

    HitboxComponent() = default;
    HitboxComponent(glm::vec3 o, float r, float h, uint32_t l, bool active = true)
        : offset(o), radius(r), height(h), layer(l), isActive(active) {}
};

bool checkHitboxHurtboxCollision(glm::vec3 hitboxWorldPos, float hitboxRadius, float hitboxHeight,
                                  glm::vec3 hurtboxWorldPos, float hurtboxRadius, float hurtboxHeight,
                                  uint32_t hitboxLayer, uint32_t hurtboxLayer,
                                  const CollisionLayerManager& layerManager);

} // namespace DarkAges::combat
