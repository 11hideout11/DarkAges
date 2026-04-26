#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>

// Target Lock System — manages lock-on targeting lifecycle
// Handles lock timeouts, pending request expiration, and cleanup
//
// Design:
// - TargetLock component attached to the player/locker entity
// - Server authoritative: clients send LockOnRequest, server validates and confirms
// - Locks expire after 30s of inactivity; pending requests time out after 5s
// - Rate limiting: minimum 1s between lock attempts

namespace DarkAges {

class TargetLockSystem {
public:
    // Configuration (tunable)
    static constexpr uint32_t LOCK_TIMEOUT_MS = 30000;      // Confirmed lock expires after 30s
    static constexpr uint32_t PENDING_TIMEOUT_MS = 5000;    // Pending request times out after 5s
    static constexpr uint32_t MIN_LOCK_INTERVAL_MS = 1000;  // Rate limit: 1s between attempts

    TargetLockSystem() = default;

    // Update: process all locks in the registry, apply timeouts
    void update(Registry& registry, uint32_t currentTimeMs);

    // Check if an entity currently has a confirmed lock on a target
    [[nodiscard]] static bool hasConfirmedLock(const Registry& registry, EntityID entity, EntityID target);

    // Get the currently locked target (if any)
    [[nodiscard]] static EntityID getLockedTarget(const Registry& registry, EntityID entity);

    // Clear a lock (used when target dies, despawns, or zone transition)
    static void clearLock(Registry& registry, EntityID entity);
};

} // namespace DarkAges
