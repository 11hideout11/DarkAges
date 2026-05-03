# PRD: World Data Population

## Introduction

Populate the game world with essential data files for NPC definitions, zone configurations, loot tables, spawn tables, and shop inventories. This provides the content that makes the world playable beyond the core systems.

## Goals

- Create npcs.json with NPC/mob definitions
- Create zones.json with zone configurations
- Create spawns.json with spawn point definitions
- Create loot.json with loot table drops
- Create shops.json with vendor inventories
- Create dialogue.json with NPC dialogues

## User Stories

### US-001: NPC definitions
**Description:** As a game designer, I want to define NPCs so they appear in the world.

**Acceptance Criteria:**
- [ ] npcs.json with level, spawn points, AI types, abilities
- [ ] NPCs have names, models, equipment
- [ ] Boss NPCs marked specially
- [ ] NPC factions (friendly, hostile, neutral)
- [ ] NPC vendor flag for shops

### US-002: Zone configurations
**Description:** As a game designer, I want to configure zones so the world is structured.

**Acceptance Criteria:**
- [ ] zones.json with zone ID, name, min/max level
- [ ] Zone type (overworld, dungeon, city, arena)
- [ ] Player capacity per zone
- [ ] PvP enabled flag
- [ ] Default spawn point per zone

### US-003: Spawn tables
**Description:** As a game designer, I want to define spawns so NPCs appear correctly.

**Acceptance Criteria:**
- [ ] spawns.json with zone-based spawn groups
- [ ] Spawn intervals, counts per wave
- [ ] Respawn timers
- [ ] Patrol routes optional
- [ ] Triggered spawns for events

### US-004: Loot tables
**Description:** As a game designer, I want to define loot so drops are meaningful.

**Acceptance Criteria:**
- [ ] loot.json with mob-level tables
- [ ] Drop rates by item rarity
- [ ] Gold drops by mob level
- [ ] Equipment drop percentages
- [ ] Consumable rare drops

### US-005: Shop inventories
**Description:** As a game designer, I want to define vendor inventories so shops work.

**Acceptance Criteria:**
- [ ] shops.json with vendor ID to items mapping
- [ ] Categories: weapons, armor, consumables
- [ ] Price multipliers by shop type
- [ ] Stock levels (unlimited, limited)
- [ ] Restock timers

### US-006: NPC dialogues
**Description:** As a game designer, I want to write NPC dialogues so the world feels alive.

**Acceptance Criteria:**
- [ ] dialogue.json with NPC ID to dialogue tree
- [ ] Greeting, quest offer, shop, repair options
- [ ] Dialogue branching
- [ ] Variable substitution
- [ ] Questgiver flags integrated

## Functional Requirements

- FR-1: data/npcs.json validated by NpcDefinition schema
- FR-2: data/zones.json validated by ZoneDefinition schema
- FR-3: data/spawns.json validated by SpawnEntry schema
- FR-4: data/loot.json validated by LootTable schema
- FR-5: data/shops.json validated by ShopInventory schema
- FR-6: data/dialogues.json validated by DialogueTree schema

## Success Metrics

- All JSON files validate against schemas
- Server loads all data on startup
- No missing references in data