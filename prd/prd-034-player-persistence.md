# PRD-034: Player Save System - Persistence

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Prerequisite:** Player account system exists

---

## Introduction

The DarkAges MMO lacks player save/persistence. When players log out, their progress is lost. This is critical for an MMO where progression must persist across sessions.

## Goals

- Save player character data on logout
- Restore character data on login
- Save inventory and equipment
- Save quest progress
- Save player position (zone + coordinates)

## User Stories

### US-001: Character Save
**Description:** As a player, I want my progress saved so that I keep my items and levels.

**Acceptance Criteria:**
- [ ] Save triggered on logout/disconnect
- [ ] All character data saved to database
- [ ] Save includes: level, XP, gold, skills

### US-002: Character Load
**Description:** As a player, I want my progress restored so that I continue where I left off.

**Acceptance Criteria:**
- [ ] Load triggered on login
- [ ] All character data restored
- [ ] Position restored to saved zone/location

### US-003: Inventory Persistence
**Description:** As a player, I want my inventory saved so that I don't lose items.

**Acceptance Criteria:**
- [ ] Inventory items saved (all 24 slots)
- [ ] Equipment saved (all 8 slots)
- [ ] Stack counts preserved
- [ ] Gold amount preserved

### US-004: Quest Progress Persistence
**Description:** As a player, I want my quest progress saved so that I don't repeat objectives.

**Acceptance Criteria:**
- [ ] Active quests saved
- [ ] Completed quests saved
- [ ] Objective progress saved (0-100%)

### US-005: Auto-Save
**Description:** As a player, I want auto-save so that I don't lose progress on crashes.

**Acceptance Criteria:**
- [ ] Auto-save every 5 minutes
- [ ] Save on zone transition
- [ ] Save on major events (level up, quest complete)

## Functional Requirements

- FR-1: Database saves must complete in <500ms
- FR-2: Load must complete in <1s
- FR-3: All player entities stored in database
- FR-4: Redis cache for active players
- FR-5: ScyllaDB for long-term storage

## Non-Goals

- Cloud save (deferred)
- Cross-server transfer (deferred)
- Backup/restore UI (deferred)

## Technical Considerations

- Integration: Redis for active, Scylla for cold storage
- Use existing PlayerSessionManager
- Save on DISCONNECT event

---

**PRD Status:** Proposed  
**Author:** OpenHands Gap Analysis  
**Next Step:** Implement SavePlayer() in PlayerManager