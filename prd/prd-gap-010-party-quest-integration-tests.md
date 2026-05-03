# PRD-GAP-010: Server Integration Tests - Party XP & Quest Tracking

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P2 - Medium  
**Category:** Testing - Server

---

## Introduction

TestIntegration.cpp line 467 has a TODO: "More integration tests — PartyXP sharing, Quest kill tracking with ZoneEvent". The current integration tests cover combat→loot→XP→inventory but don't cover party play and quest tracking with zone events.

**Problem:** No tests exist for:
1. Party XP sharing (multiple players killing same NPC)
2. Quest kill tracking wired to ZoneEvent system

---

## Goals

- Party XP sharing tests pass
- Quest tracking with kill events tests pass
- ZoneEvent integration tests pass

---

## User Stories

### US-001: Party XP Sharing
**Description:** As a party, we want XP shared when killing NPCs.

**Acceptance Criteria:**
- [ ] Two players in party kill NPC → both get XP
- [ ] XP split per party rules
- [ ] Absent party members get reduced/no XP

### US-002: Quest Kill Tracking
**Description:** As a player on a quest, I want kills to count toward objectives.

**Acceptance Criteria:**
- [ ] Quest active → NPC kill increments objective
- [ ] Kill tracking wires to ZoneEvent system
- [ ] Objective completes at correct count

### US-003: ZoneEvent Integration
**Description:** As a system, ZoneEvents trigger correctly.

**Acceptance Criteria:**
- [ ] ZONE_ENTER event fires
- [ ] ZONE_LEAVE event fires
- [ ] Kill events propagate

---

## Functional Requirements

- FR-1: PartySystem::shareKillXP() awards XP to party
- FR-2: QuestComponent tracks kills for active objectives
- FR-3: ZoneEvent emits on entity death
- FR-4: Integration tests wire these together

---

## Technical Considerations

- **Existing Code:**
  - PartySystem (existing)
  - QuestComponent (existing)
  - ZoneEvent system (existing)
  - TestIntegration.cpp has test structure

- **Test Cases to Add:**
  ```cpp
  TEST_CASE("Party XP: Shared on kill") { ... }
  TEST_CASE("Quest: Kill increments objective") { ... }
  TEST_CASE("ZoneEvent: Fires on death") { ... }
  ```

---

## Success Metrics

- All new tests pass
- Party XP correct
- Quest tracking correct