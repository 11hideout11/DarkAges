# PRD-GAP-005: World Data Population

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P1 - Medium  
**Category:** Content - Game Data

---

## Introduction

PRD-033 (World Data Population) identifies the need for game content population (items, abilities, quests, zones, NPCs), BUT the data may be incomplete or not fully integrated. The demo needs sufficient content for a playable experience.

**Problem:** Some content exists in JSON but may be missing critical data (item stats, ability cooldowns, NPC templates) that prevents proper gameplay.

---

## Goals

- Items have complete stats for gameplay (damage, armor, effects)
- Abilities have complete data (cooldown, cost, targeting)
- NPC templates have complete data (stats, behaviors, loot)
- Zone configs have all required fields
- All content integrates with systems

---

## User Stories

### US-001: Item Data Completeness
**Description:** As a designer, I want item data to be complete so gameplay works.

**Acceptance Criteria:**
- [ ] All weapons have damage, speed, range
- [ ] All armor has defense values
- [ ] Consumables have effects defined
- [ ] Items reference correct icon/texture

### US-002: Ability Data Completeness
**Description:** As a designer, I want ability data to be complete so combat works.

**Acceptance Criteria:**
- [ ] Abilities have cooldown times
- [ ] Abilities have resource costs
- [ ] Abilities have targeting (self, ground, unit)
- [ ] Abilities have effect definitions

### US-003: NPC Template Completeness
**Description:** As a designer, I want NPC templates complete so spawns work.

**Acceptance Criteria:**
- [ ] NPCs have health, damage, armor stats
- [ ] NPCs have behavior enum
- [ ] NPCs have loot table reference
- [ ] NPCs have ability list

### US-004: Zone Config Completeness
**Description:** As a designer, I want zone configs complete so loading works.

**Acceptance Criteria:**
- [ ] All required spawn_points defined
- [ ] NPC presets reference valid archetypes
- [ ] Loot tables reference valid items
- [ ] Wave configs have all fields

---

## Functional Requirements

- FR-1: Items.json has all weapon/armor/consumable fields
- FR-2: Abilities.json has all ability fields
- FR-3: NPCs.json has all template fields
- FR-4: Zone JSONs validate against schema
- FR-5: LoadAllContent() loads and validates

---

## Technical Considerations

- **Content Files:**
  - `data/items.json` - weapons, armor, consumables
  - `data/abilities.json` - player and NPC abilities
  - `data/npcs.json` - NPC templates  
  - `tools/demo/content/zones/*.json` - zone configs

- **Validation:**
  - Required fields enforced on load
  - Missing reference errors logged
  - Default values for optional fields

---

## Success Metrics

- **Functional:**
  - All content loads without errors
  - No missing reference warnings
  - Gameplay works with all content

- **Coverage:**
  - Minimum 10 weapon types
  - Minimum 5 ability types
  - Minimum 3 NPC types per zone