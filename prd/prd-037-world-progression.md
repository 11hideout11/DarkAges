# PRD-037: World Progression System - Complete

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** ✅ Complete — WorldProgressionSystem.hpp/.cpp wired into ZoneServer lifecycle (commit 817bbeb)
**Priority:** P0-Critical  
**Category:** World Progression - Cross-Zone Integration

---

## 1. Introduction/Overview

Create a coherent world progression where players journey through zones in a meaningful order that creates narrative and difficulty progression. Currently zones exist but are not integrated into a world progression path.

### Problem Statement
- Tutorial, Arena, Boss zones exist but are not part of a progression path
- No requirement to clear earlier zones to access harder ones
- No world map showing progression
- Players can jump to any zone (breaking difficulty curve)
- No sense of "world" or journey

### Why This Matters for AAA
- World progression creates the MMO "journey" feeling
- Must feel like exploring a real world
- Creates goals and achievement when progressing

---

## 2. Goals

- Tutorial zone is mandatory entry point (new characters start here)
- Arena unlocks after completing Tutorial
- Boss zone unlocks after completing Arena
- Open world (future) unlocks after Boss
- Zone difficulty scales with progression
- World map UI shows accessible zones
- Locked zones show requirements
- Progression persists per character

---

## 3. User Stories

### US-037-001: Tutorial Entry Gate
**Description:** As a new character, I want to start in Tutorial so that I learn game basics.

**Acceptance Criteria:**
- [ ] New characters spawn in Tutorial zone (zone 98)
- [ ] Tutorial objectives teach: movement, attack, interact, inventory
- [ ] Cannot leave Tutorial until objectives complete
- [ ] Tutorial completion unlocks Arena

### US-037-002: Arena Unlock Gate
**Description:** As a player who completed Tutorial, I want to access the Arena so that I can fight other players.

**Acceptance Criteria:**
- [ ] Arena (zone 99) locked until Tutorial complete
- [ ] World map shows Arena as "locked" with requirement
- [ ] CompletingTutorial criteria: defeat 3 training dummies, speak to NPC
- [ ] Once unlocked, player can return freely

### US-037-003: Boss Unlock Gate
**Description:** As a player who completed Arena, I want to access the Boss lair so that I face the challenge.

**Acceptance Criteria:**
- [ ] Boss zone (zone 100) locked until Arena complete
- [ ] Requires: Arena champion defeated
- [ ] Recommended level: 8+ (warning shown)
- [ ] Boss completion unlocks open world

### US-037-004: World Map Integration
**Description:** As a player, I want to see which zones I can access so that I can plan my journey.

**Acceptance Criteria:**
- [ ] World map UI shows all zones
- [ ] Accessible zones: full color
- [ ] Locked zones: gray with requirement text
- [ ] Current zone highlighted
- [ ] Teleport to accessible zones

### US-037-005: Difficulty Scaling
**Description:** As the game, I want enemies to scale with zone difficulty so that progression feels impactful.

**Acceptance Criteria:**
- [ ] Tutorial enemies: Level 1-3, low damage
- [ ] Arena enemies: Level 4-7, medium damage
- [ ] Boss enemies: Level 8-10, high damage
- [ ] Enemy level shown on hover
- [ ] Level difference affects damage (+10%/level diff)

### US-037-006: Progression Persistence
**Description:** As a returning player, I want my progress to persist so that I don't start over.

**Acceptance Criteria:**
- [ ] Zone unlocks stored in character data
- [ ] Unlocks persist across sessions
- [ ] Party members can join unlocked zones
- [ ] Level requirement shown for locked zones

---

## 4. Functional Requirements

- FR-037-1: Tutorial entry enforced on spawn
- FR-037-2: Zone unlock system (tutorial_complete, arena_complete, boss_complete)
- FR-037-3: World map UI (accessible zones highlighted)
- FR-037-4: Zone lock check when teleporting
- FR-037-5: Enemy level scaling per zone
- FR-037-6: Level damage bonus/penalty
- FR-037-7: Save/load progression
- FR-037-8: Party leader can invite to accessible zones

---

## 5. Non-Goals

- No level-geated zones (unlock by completion, not level)
- No skip Tutorial option in v1
- No dynamic difficulty adjustment
- No cross-server progression
- No "catch-up" mechanics in v1

---

## 6. Technical Considerations

- Zone locks stored in character metadata
- ZoneServer checks lock before allowing entry
- WorldMap.tscn queries available zones
- Enemy stats loaded from zone config
- Difficulty formula in CombatSystem

### Dependencies
- ZoneObjectiveSystem (existing, PRD-009)
- ZoneServer (existing)
- SaveSystem (GAP-014)
- WorldMap UI (existing PRD)

---

## 7. Success Metrics

- New player can't skip Tutorial
- Progression is clearly visible on world map
- Difficulty curve feels challenging but fair
- Players understand how to progress
- No exploits in unlock system

---

## 8. Open Questions

- Level sync for groups with level difference?
- Daily/weekly lock reset for farming?
- Hard mode versions of zones?