#include "combat/TargetLockSystem.hpp"

namespace DarkAges {

void TargetLockSystem::update(Registry& registry, uint32_t currentTimeMs) {
    // Iterate all entities with TargetLock component
    auto view = registry.view<TargetLock>();
    view.each([&](EntityID /*entity*/, TargetLock& lock) {
        // Already no lock? Nothing to do
        if (lock.lockState == LockState::None) {
            return;
        }

        uint32_t elapsed = currentTimeMs - lock.lockTimeMs;

        if (lock.lockState == LockState::Confirmed) {
            // Confirmed locks expire after LOCK_TIMEOUT_MS
            if (elapsed >= LOCK_TIMEOUT_MS) {
                lock.lockState = LockState::None;
                lock.lockedTarget = entt::null;
                lock.lockTimeMs = 0;
            }
        } else if (lock.lockState == LockState::Pending) {
            // Pending requests timeout faster
            if (elapsed >= PENDING_TIMEOUT_MS) {
                lock.lockState = LockState::None;
                lock.lockedTarget = entt::null;
                lock.lockTimeMs = 0;
            }
        }
    });
}

bool TargetLockSystem::hasConfirmedLock(const Registry& registry, EntityID entity, EntityID target) {
    if (auto* lock = registry.try_get<TargetLock>(entity)) {
        return lock->lockState == LockState::Confirmed && lock->lockedTarget == target;
    }
    return false;
}

EntityID TargetLockSystem::getLockedTarget(const Registry& registry, EntityID entity) {
    if (auto* lock = registry.try_get<TargetLock>(entity)) {
        if (lock->lockState == LockState::Confirmed) {
            return lock->lockedTarget;
        }
    }
    return entt::null;
}

void TargetLockSystem::clearLock(Registry& registry, EntityID entity) {
    if (auto* lock = registry.try_get<TargetLock>(entity)) {
        lock->lockState = LockState::None;
        lock->lockedTarget = entt::null;
        lock->lockTimeMs = 0;
    }
}

} // namespace DarkAges
