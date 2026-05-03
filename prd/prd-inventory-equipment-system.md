# PRD: Inventory & Equipment System
## Introduction
The current inventory system is minimal with placeholder functionality. A proper equipment system with stats, gear slots, and item management is required for progression depth and loot systems.
**Problem Statement:** Players have no equipment slots, no stat system, and no meaningful loot. This limits progression motivation and gameplay variety.
---

## Goals
- Create equipment slots with stat modifiers
- Implement item database with 50+ items
- Support gear score and power scaling
- Provide drag-and-drop inventory UI
- Enable server-validated item transactions
- Implement loot drops and merchant transactions
---

## User Stories
### US-001: Create Equipment System
**Description:** As a player, I want equipment slots so I can gear my character for combat.
**Acceptance Criteria:**
- [ ] Equipment slots: Head, Chest, Hands, Legs, Feet, MainHand, OffHand
- [ ] Item drag-and-drop to slots
- [ ] Item stats displayed on hover
- [ ] Stat summary (damage, defense, health)
- [ ] Equipment set bonuses

### US-002: Implement Item Database
**Description:** As a developer, I need an item database so I can add items with consistent structure.
**Acceptance Criteria:**
- [ ] Item JSON schema defined
- [ ] Item types: weapon, armor, consumable, quest, key
- [ ] Rarity levels with colors
- [ ] Item icons and descriptions
- [ ] Equipment level requirements

### US-003: Build Inventory UI
**Description:** As a player, I want an inventory UI so I can manage my items efficiently.
**Acceptance Criteria:**
- [ ] 4x8 bag grid
- [ ] Equipment panel with slots
- [ ] Item tooltip on hover
- [ ] Item sorting options
- [ ] Currency display

### US-004: Implement Loot System
**Description:** As a player, I want loot drops so defeating enemies is rewarding.
**Acceptance Criteria:**
- [ ] Loot drop tables per NPC type
- [ ] Drop rate percentages
- [ ] Loot window on enemy death
- [ ] Auto-loot option
- [ ] Currency and item drops
---

## Functional Requirements
- FR-1: Create EquipmentSlot component
- FR-2: Define Item schema in database
- FR-3: Implement stat calculation system
- FR-4: Build inventory UI panel
- FR-5: Create item tooltip system
- FR-6: Implement loot drop tables
- FR-7: Add merchant NPC interaction
- FR-8: Validate transactions server-side
- FR-9: Add inventory unit tests
- FR-10: Document item design patterns
---

## Non-Goals
- No crafting system
- No enchanting system
- No auction house
- No shared inventory (MVP scope)
- No item gem slots
---

## Open Questions
1. What stats are calculated (STR, DEX, INT, VIT)?
2. Is equipment scaling linear or exponential?
3. Should loot be per-player or party-shared?
4. What is the itemization philosophy (stats vs. unique)?
---