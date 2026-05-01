# PRD-009: Demo Zones System

**Version:** 1.0
**Status:** Not Started
**Owner:** SERVER_AGENT
**Priority:** CRITICAL
**Dependencies:** PRD-004 (Sharding), PRD-005 (Client)

---

## 1. Overview

### 1.1 Purpose
Create 3 demo zones with curated gameplay experiences: Tutorial, Combat Arena, and Boss Arena. Replace the single zone 99 with a multi-zone playable demo.

### 1.2 Scope
- Tutorial Zone (zone 1): Learning zone with safe PvE encounters
- Combat Arena (zone 2): PvP arena with arena rules
- Boss Zone (zone 3): Raid-style boss encounter with loot mechanics

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| ZON-001 | Tutorial Zone (zone 1) | P0 | Tutorial quests, safe training |
| ZON-002 | Combat Arena (zone 2) | P0 | PvP arena with respawn |
| ZON-003 | Boss Zone (zone 3) | P0 | Boss encounter with mechanics |
| ZON-004 | Zone transitions | P0 | Seamless handoff |
| ZON-005 | Zone-specific rules | P1 | Arena rules, PvE vs PvP |

### 2.2 Zone Definitions

**Tutorial Zone (zone 1):**
- Safe area: no PvP damage
- Training dummies
- Tutorial quest giver NPCs
- Exit portal to Arena

**Combat Arena (zone 2):**
- PvP enabled
- Team rules (Blue vs Red)
- 5v5 format
- Respawn on timer

**Boss Zone (zone 3):**
- PvE encounter
- Boss with 3 phases
- Loot drops on kill

---

## 3. Current Gap

**Gap:** Only zone 99 exists. No multi-zone demo available.

**Location:** src/server/zones/ (single zone implementation)

---

## 4. Deliverables

Zone configurations in src/server/zones/configs/:
- tutorial.json (zone 1)
- arena.json (zone 2)
- boss.json (zone 3)

Zone scripts:
- ZoneServer.cs (zone rule engine)
- TutorialRules.gd
- ArenaRules.gd
- BossRules.gd

---

## 5. Acceptance Criteria

- [ ] 3 zones operational
- [ ] Tutorial zone: quest system works
- [ ] Arena zone: PvP combat functional
- [ ] Boss zone: boss encounter works
- [ ] Zone transitions: seamless handoff

---

*Last Updated: 2026-05-01*
