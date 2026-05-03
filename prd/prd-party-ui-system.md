# PRD: Party UI System

## Introduction

Implement a party UI system that displays party members, party management, party chat, and party loot rules. The system should integrate with the existing PartySystem server component.

## Goals

- Display party member list with status
- Party creation and invite UI
- Party loot rules configuration
- Party chat view

## User Stories

### US-001: Party display
**Description:** As a party member, I want to see my party.

**Acceptance Criteria:**
- [ ] Shows party members (max 5)
- [ ] Member name, class, HP, ready status
- [ ] Leader highlighted
- [ ] Kick button for leader

### US-002: Party invite
**Description:** As a player, I want to invite others to party.

**Acceptance Criteria:**
- [ ] "Invite" button
- [ ] Target player selection
- [ ] Invite sent notification
- [ ] Cannot exceed 5 members

### US-003: Loot rules
**Description:** As party leader, I want to set loot rules.

**Acceptance Criteria:**
- [ ] Loot: Free, Master, Round Robin
- [ ] Item distribution display
- [ ] Leader can change rules

### US-004: Party ready check
**Description:** As a party leader, I want to check ready status.

**Acceptance Criteria:**
- [ ] "Ready Check" button
- [ ] Member shows ready/not ready
- [ ] All ready notifies leader

## Functional Requirements

- FR-1: PartyUI.tscn scene
- FR-2: PartyMemberComponent
- FR-3: PartyLootComponent
- FR-4: PartyReadyComponent

## Success Metrics

- Party UI updates in real-time
- Ready check completes in < 5s
- No sync issues