#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <chrono>

namespace DarkAges::combat::detail {

struct Hitbox {
    float radius;                       // Melee range in world units (meters)
    glm::vec3 origin;                   // World-space center of the hitbox
    std::chrono::steady_clock::time_point startTime;
};

} // namespace DarkAges::combat::detail
