# PRD-GAP-012: Anti-Cheat System Implementation

**Version:** 1.0
**Date:** 2026-05-03
**Status:** Proposed
**Priority:** P1 - High
**Category:** Security - Game Integrity

---

## Introduction

The AntiCheatSystem.hpp exists with CheatType detection (SPEED_HACK, FLY_HACK, DAMAGE_HACK), BUT certain detection modes may be stubbed or incomplete. Without effective anti-cheat, online play is compromised.

**Problem:** AntiCheatLogger logs detections but bans may not trigger. Some speed/fly hack checks are incomplete.

---

## Goals

- Speed hack detection triggers at movement > player_max_speed
- Fly hack detection triggers at invalid Y position
- Damage hack detection triggers at impossible damage combos
- Ban system activates after threshold violations
- Violations persist to database/replay for GM review

---

## User Stories

### US-001: Speed Hack Detection
**Description:** As a server, I want to flag players moving too fast.

**Acceptance Criteria:**
- [ ] Movement velocity > 12 units/s flags SPEED_HACK
- [ ] Detection logs with timestamp and position
- [ ] 3 violations in 60s triggers warning

### US-002: Fly Hack Detection
**Description:** As a server, I want to flag players in impossible positions.

**Acceptance Criteria:**
- [ ] Y position > 50 or < -10 flags FLY_HACK
- [ ] Detection uses PositionComponent history
- [ ] Anti-gravity logic checks velocity vs ground

### US-003: Damage Hack Detection
**Description:** As a server, I want to flag impossible damage.

**Acceptance Criteria:**
- [ ] Damage > 10x player weapon max flags DAMAGE_HACK
- [ ] Detection includes damage source verification
- [ ] No false positives from critical hits

### US-004: Ban System
**Description:** As a server, I want to ban confirmed cheaters.

**Acceptance Criteria:**
- [ ] 10 violations in 300s triggers automatic ban
- [ ] Ban persists to anti_cheat_violations table
- [ ] GM can review and unban

---

## Functional Requirements

- FR-1: MovementValidator computes velocity from history
- FR-2: AntiCheatLogger records violations with context
- FR-3: AntiCheatSystem triggers warnings at threshold
- FR-4: Auto-ban fires after ban threshold reached
- FR-5: Violations queryable via GM tools

---

## Non-Goals

- Client-side anti-cheat ( Warden ) - defer
- Hardware ID bans - defer
- Replay system for spectating - defer

---

## Technical Considerations

- **Existing Code:**
  - `AntiCheatTypes.hpp` - CheatType enum exists
  - `AntiCheatSystem.hpp` - stub exists
  - `AntiCheatLogger.hpp` - logging exists

- **Integration Points:**
  - ZoneServer tick → MovementValidator → AntiCheatSystem
  - CombatSystem dealDamage → DamageValidator → AntiCheatSystem

---

## Success Metrics

- Zero false positive bans in test
- All speed/fly/damage hacks detected in test suite
- GM can query violation history

---

## Open Questions

- Q: How strict should thresholds be?
  - A: Configurable per-zone in server config