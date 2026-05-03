# PRD: Global Cooldown System

## Introduction

The DarkAges combat system lacks a global cooldown (GCD) mechanism. Currently, attacks can be chained infinitely without timing constraints, which breaks combat pacing and removes meaningful decision-making. This PRD implements a server-authoritative 1.2 second GCD to enforce proper combat rhythm.

## Goals

- Implement 1.2 second global cooldown between attack actions
- Server-authoritative validation (client cannot bypass)
- Prevent attack spam while maintaining responsive combat feel
- Sync cooldown state to client for UI feedback

## User Stories

### GCD-001: Enforce Attack Timing
**Description:** As a combat designer, I want attack actions throttled so players cannot spam attacks infinitely.

**Acceptance Criteria:**
- [ ] Server tracks GCD per player entity
- [ ] Attack inputs rejected when GCD active (with clear error)
- [ ] GCD resets after 1.2 seconds
- [ ] All attack abilities respect GCD

### GCD-002: Client Cooldown Feedback
**Description:** As a player, I want to know when I can attack again.

**Acceptance Criteria:**
- [ ] Cooldown remaining sent to client each snapshot
- [ ] Visual indicator (button overlay or cooldown bar)
- [ ] Client prediction accounts for GCD

### GCD-003: Ability-Specific Cooldowns
**Description:** As a player, I want certain abilities to have individual cooldowns separate from GCD.

**Acceptance Criteria:**
- [ ] Ability registry supports per-ability cooldown config
- [ ] Special abilities can have longer cooldowns
- [ ] Cooldown persists across deaths (optional)

## Functional Requirements

- FR-1: Add GCD component to player entity (uint64_t lastAttackTimeMs)
- FR-2: Attack input validation rejects if (now - lastAttackTimeMs) < 1200ms
- FR-3: Cooldown state included in entity snapshot
- FR-4: Ability config supports cooldown duration override
- FR-5: Server log GCD violations for anti-cheat

## Non-Goals

- No cooldown reduction from items/buffs (Phase 2+)
- No GCD on movement/dodge actions
- No shared cooldown groups between abilities
- No client-side GCD enforcement (server authoritative only)

## Technical Considerations

### Server Implementation
```cpp
// In CombatSystem.hpp
struct GlobalCooldown {
    uint64_t lastAttackTimeMs{0};
    static constexpr uint16_t DEFAULT_GCD_MS = 1200;
};

// In InputHandler.hpp validation
bool canAttack(EntityID attacker) {
    auto* gcd = registry.try_get<GlobalCooldown>(attacker);
    if (!gcd) return true;
    uint64_t now = getCurrentTimeMs();
    return (now - gcd->lastAttackTimeMs) >= GCD_DURATION_MS;
}
```

### Protocol Extension
- Add cooldownRemainingMs to AttackInputResult packet
- Add abilityCooldownDuration to AbilityDefinition

### Performance
- GCD check is O(1) component lookup
- No per-attack allocation

## Success Metrics

- Attack spam reduced to max 0.83 attacks/second per player
- No GCD bypass possible via packet manipulation
- Client receives cooldown sync within 1 tick

## Open Questions

- Should GCD apply to all abilities or just attacks?
- Should we add short cooldown (0.5s) for basic attacks vs long for specials?
- Do we need GCD penalty for mis-timing (delayed reset)?