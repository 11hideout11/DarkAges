# PRD-117: World Data Population - NPC/Zone Definitions

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical  
**Prerequisite:** None - required for gameplay

---

## Introduction

The DarkAges MMO is missing essential world configuration data files. Without these, there's no content for players to interact with:

- ❌ NPCs.json - Mob/NPC definitions  
- ❌ zones.json - Zone configurations
- ❌ spawns.json - Spawn point definitions
- ❌ loot.json - Loot table drops
- ❌ shops.json - Vendor inventories
- ❌ dialogue.json - NPC dialogues

The server systems exist (CombatSystem, QuestSystem, SpawnSystem), but they have no data to work with.

## Goals

- Create npcs.json with NPC/mob definitions
- Create zones.json with zone configurations  
- Create spawns.json with spawn tables
- Create loot.json with drop rates
- Create shops.json with vendor inventories
- Create dialogue.json with NPC conversations

## User Stories

### US-001: NPC Definitions
**Description:** As a game designer, I want to define NPCs so they appear in the world.

**Acceptance Criteria:**
- [ ] data/npcs.json exists with 50+ NPC entries
- [ ] Each NPC has: id, name, level, faction, ai_type
- [ ] Mob templates include: health, damage, abilities
- [ ] Boss NPCs marked with is_boss: true
- [ ] Vendor NPCs marked with is_vendor: true

### US-002: Zone Configurations  
**Description:** As a game designer, I want to configure zones so the world is structured.

**Acceptance Criteria:**
- [ ] data/zones.json exists with 10+ zone entries
- [ ] Each zone has: id, name, type, level_range, player_cap
- [ ] Zone types: overworld, dungeon, city, arena
- [ ] PvP_enabled flag per zone
- [ ] Default spawn points per zone

### US-003: Spawn Tables
**Description:** As a game designer, I want to define spawns so NPCs appear correctly.

**Acceptance Criteria:**
- [ ] data/spawns.json exists
- [ ] Spawn groups per zone
- [ ] Wave configurations
- [ ] Respawn timers (seconds)
- [ ] Patrol waypoints (optional)

### US-004: Loot Tables
**Description:** As a game designer, I want to define loot so drops are meaningful.

**Acceptance Criteria:**
- [ ] data/loot.json exists
- [ ] Loot tables per NPC type
- [ ] Drop rates (percentage)
- [ ] Item rarity weights
- [ ] Gold drop calculations

### US-005: Shop Inventories
**Description:** As a game designer, I want vendor shops so players can buy items.

**Acceptance Criteria:**
- [ ] data/shops.json exists
- [ ] Shop inventories per vendor
- [ ] Item prices in gold
- [ ] Buy/sell ratios (e.g., 50%)
- [ ] Restock intervals

### US-006: NPC Dialogues
**Description:** As a game designer, I want dialogues so NPCs have personality.

**Acceptance Criteria:**
- [ ] data/dialogue.json exists
- [ ] Quest giver dialogues
- [ ] Shopkeeper lines
- [ ] Gossip conversations
- [ ] Branching options

## Functional Requirements

- FR-1: All JSON files must be valid (parse without errors)
- FR-2: NPC IDs must reference existing items/abilities
- FR-3: Zone IDs must match spawn configurations
- FR-4: Shop prices must be balanced (< 1M gold for rare items)
- FR-5: Loot tables must total 100% or less (no guaranteed drop)

## Non-Goals

- Voice acting (text only)
- Dynamic events (static spawns first)
- Seasonal content
- UGC support

## Technical Considerations

- JSON Schema validation for each file
- Integration: Load via ZoneManager, SpawnSystem
- Demo data: Use tutorial zone (98), combat zone (99), boss (100)

## Success Metrics

- All 6 JSON files created
- 50+ NPCs defined
- 10+ zones configured
- Valid JSON (no parse errors)
- Demo zones playable

---

**Status:** ✅ Complete — 22 abilities, 51 items, 10 quests, 3 spawn zones, dialogue data all populated
**Author:** OpenHands Gap Analysis  
**Next Step:** Create data/npcs.json and data/zones.json