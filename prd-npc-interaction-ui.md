# PRD: NPC Interaction UI

## Introduction

Implement an NPC interaction UI that displays dialogue, quests, shops, and services when interacting with NPCs. The system should integrate with the DialogueSystem server component.

## Goals

- Display dialogue interface
- Show quest offers and turn-ins
- Display shop interface
- Show repair service
- Integration with interaction prompts

## User Stories

### US-001: Dialogue UI
**Description:** As a player, I want to talk to NPCs.

**Acceptance Criteria:**
- [ ] Opens when near NPC
- [ ] Shows dialogue text
- [ ] Shows response options
- [ ] Click response to continue
- [ ] Close with ESC

### US-002: Quest UI
**Description:** As a player, I want to receive quests from NPCs.

**Acceptance Criteria:**
- [ ] "Available Quests" tab
- [ ] Quest details shown
- [ ] "Accept" button
- [ ] Quest tracker updated

### US-003: Shop UI
**Description:** As a player, I want to buy from NPCs.

**Acceptance Criteria:**
- [ ] "Shop" button on vendor
- [ ] Item list with prices
- [ ] Buy button
- [ ] Gold updated
- [ ] Inventory updated

### US-004: Repair UI
**Description:** As a player, I want to repair gear.

**Acceptance Criteria:**
- [ ] "Repair" button
- [ ] Shows damaged items
- [ ] Shows repair cost
- [ ] Accepts repair
- [ ] Gold deducted

## Functional Requirements

- FR-1: NPCInteractionUI.tscn scene
- FR-2: DialogueComponent
- FR-3: QuestOfferComponent
- FR-4: ShopInterfaceComponent
- FR-5: RepairComponent

## Success Metrics

- UI opens in < 200ms
- All interactions work
- No missing items