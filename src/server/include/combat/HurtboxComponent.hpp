#pragma once

#include <glm/glm.hpp>
#include <cstdint>

#include "ecs/CoreTypes.hpp"

namespace DarkAges::combat {

struct HurtboxComponent {
    glm::vec3 offset{0.0f, 0.9f, 0.0f};
    float radius{0.5f};
    float height{1.8f};
    uint32_t layer{CollisionLayerMask::HURTBOX};
    bool isActive{false};

    HurtboxComponent() = default;
    HurtboxComponent(glm::vec3 o, float r, float h, uint32_t l, bool active = true)
        : offset(o), radius(r), height(h), layer(l), isActive(active) {}
};

} // namespace DarkAges::combat

#endif // DARKAGES_COMBAT_HURTBOXCOMPONENT_HPP
