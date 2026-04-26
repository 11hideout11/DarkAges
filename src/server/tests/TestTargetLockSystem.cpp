#include <catch2/catch_test_macros.hpp>
#include "ecs/CoreTypes.hpp"
#include "combat/TargetLockSystem.hpp"

using namespace DarkAges;

TEST_CASE("[target-lock] TargetLock component default values", "[ecs]") {
    TargetLock lock;
    REQUIRE((lock.lockedTarget == entt::null));
    REQUIRE(lock.lockState == LockState::None);
    REQUIRE(lock.lockTimeMs == 0);
    REQUIRE(lock.lastLockAttempt == 0);
}

TEST_CASE("[target-lock] LockState enum values", "[ecs]") {
    REQUIRE(static_cast<uint8_t>(LockState::None) == 0);
    REQUIRE(static_cast<uint8_t>(LockState::Pending) == 1);
    REQUIRE(static_cast<uint8_t>(LockState::Confirmed) == 2);
}

TEST_CASE("[target-lock] TargetLockSystem clears expired confirmed locks", "[ecs]") {
    Registry registry;
    EntityID locker = registry.create();
    EntityID target = registry.create();

    // Setup: locker has TargetLock with Confirmed state
    auto& lock = registry.emplace<TargetLock>(locker);
    lock.lockedTarget = target;
    lock.lockState = LockState::Confirmed;
    lock.lockTimeMs = 1000;
    lock.lastLockAttempt = 1000;

    TargetLockSystem system;
    uint32_t now = 40000; // 30+ seconds later (default timeout = 30s)
    system.update(registry, now);

    // Lock should be cleared (lockState back to None)
    auto& after = registry.get<TargetLock>(locker);
    REQUIRE(after.lockState == LockState::None);
    REQUIRE((after.lockedTarget == entt::null));
}

TEST_CASE("[target-lock] TargetLockSystem respects lock timeout duration", "[ecs]") {
    Registry registry;
    EntityID locker = registry.create();
    EntityID target = registry.create();

    auto& lock = registry.emplace<TargetLock>(locker);
    lock.lockedTarget = target;
    lock.lockState = LockState::Confirmed;
    lock.lockTimeMs = 1000;
    lock.lastLockAttempt = 900;

    TargetLockSystem system;
    // Default timeout is 30 seconds; test at 29 seconds — should still be locked
    uint32_t now = 1000 + 29000;
    system.update(registry, now);

    auto& after = registry.get<TargetLock>(locker);
    REQUIRE(after.lockState == LockState::Confirmed); // still locked
}

TEST_CASE("[target-lock] TargetLockSystem clears stale pending locks", "[ecs]") {
    Registry registry;
    EntityID locker = registry.create();
    EntityID target = registry.create();

    auto& lock = registry.emplace<TargetLock>(locker);
    lock.lockedTarget = target;
    lock.lockState = LockState::Pending;
    lock.lockTimeMs = 1000;
    lock.lastLockAttempt = 1000;

    TargetLockSystem system;
    // Pending timeout is 5 seconds; advance 6 seconds
    uint32_t now = 1000 + 6000;
    system.update(registry, now);

    auto& after = registry.get<TargetLock>(locker);
    REQUIRE(after.lockState == LockState::None);
}
