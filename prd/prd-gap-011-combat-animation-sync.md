# PRD-GAP-011: Combat-Animation Synchronization

**Version:** 1.0
**Date:** 2026-05-03
**Status:** Proposed
**Priority:** P0 - Critical
**Category:** Gameplay - Combat Feel

---

## Introduction

When a server sends a combat event (damage dealt, hit detected, ability cast), the client must play appropriate animations to give players visual feedback. Without this sync, combat feels "floaty" - players hit enemies but don't see impact effects.

**Problem:** Server CombatEvent fires, but client may not play HitEffect, SpellEffect, or combat animations in sync with server damage.

---

## Goals

- Hit effects play when server confirms damage dealt
- Knockback animation triggers on knockback events
- Death animations play when enemy dies
- Ability cast animations sync with ability activation
- No desync between server damage and client visual feedback

---

## User Stories

### US-001: Hit Effect Sync
**Description:** As a player, I want to see hit effects when I damage an enemy.

**Acceptance Criteria:**
- [ ] HitEffect.tscn plays within 50ms of server damage notification
- [ ] Hit effect spawns at damage location
- [ ] Different hit effects for different damage types (slash, pierce, impact)

### US-002: Knockback Sync
**Description:** As a player, I want to see enemies knocked back when hit.

**Acceptance Criteria:**
- [ ] Knockback animation triggers when server sends knockback event
- [ ] Knockback distance matches server calculation
- [ ] No floating/clipping during knockback

### US-003: Death Animation Sync
**Description:** As a player, I want enemies to play death animations when killed.

**Acceptance Criteria:**
- [ ] DeathEffect.tscn plays on enemy death
- [ ] Death animation duration: 1-2 seconds
- [ ] Corpse fades or despawns after animation

### US-004: Ability Cast Animation
**Description:** As a player, I want to see ability cast animations.

**Acceptance Criteria:**
- [ ] SpellEffect.tscn plays at cast location
- [ ] Cast animation duration matches ability cooldown
- [ ] Client prediction plays immediately, server confirms

---

## Functional Requirements

- FR-1: NetworkManager dispatches combat event signals to UI
- FR-2: CombatEventSystem listens and instantiates effects
- FR-3: HitEffect/SpellEffect/DeathEffect scenes exist
- FR-4: Effect position syncs with entity position
- FR-5: Effect lifetime matches animation duration

---

## Non-Goals

- Server-side only, no client prediction (use existing)
- Audio sync (use AudioSystem PRD)
- Particle systems beyond basic effects (defer)
- Cinematic kill cams (defer)

---

## Technical Considerations

- **Existing Code:**
  - `HitEffect.tscn` - exists
  - `DeathEffect.tscn` - exists  
  - `SpellEffect.tscn` - exists
  - `NetworkManager` - already parses combat events

- **Integration Points:**
  ```
  Server: CombatSystem::dealDamage() → send PACKET_DAMAGE
  → NetworkManager::receivePacket() → emit signal
  → CombatEventSystem::OnDamageReceived() → spawn effect
  → HitEffect.Play()
  ```

---

## Success Metrics

- Hit effects visible within 50ms of damage
- Zero "ghost hit" complaints (damage shown but no animation)
- Death animations play for all enemy types

---

## Open Questions

- Q: Client prediction or server-authoritative?
  - A: Both - predict locally, correct if server differs