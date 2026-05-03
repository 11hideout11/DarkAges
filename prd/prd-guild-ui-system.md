# PRD: Guild UI System

## Introduction

Implement a guild UI system that displays guild roster, guild management, guild chat, guild storage, and guild rankings. The system should integrate with the existing GuildSystem server component.

## Goals

- Display guild roster with member roles
- Guild creation and management UI
- Guild chat integration
- Guild storage view
- Guild ranks configuration

## User Stories

### US-001: Guild roster
**Description:** As a guild member, I want to see my guild members.

**Acceptance Criteria:**
- [ ] Shows all guild members
- [ ] Columns: Name, Rank, Level, Status
- [ ] Online indicator
- [ ] Sort by rank/level

### US-002: Guild management
**Description:** As guild leader, I want to manage guild.

**Acceptance Criteria:**
- [ ] Edit guild description
- [ ] Manage member ranks
- [ ] Kick members
- [ ] Promote/demote
- [ ] Transfer leadership

### US-003: Guild storage
**Description:** As a guild member, I want to access guild storage.

**Acceptance Criteria:**
- [ ] Opens via button
- [ ] Shows item list
- [ ] Deposit/withdraw items
- [ ] Permission levels per rank

### US-004: Guild creation
**Description:** As a player, I want to create a guild.

**Acceptance Criteria:**
- [ ] "Create Guild" button
- [ ] Name input (max 32 chars)
- [ ] Emblem selection
- [ ] Creation cost shown
- [ ] Creation completes

## Functional Requirements

- FR-1: GuildUI.tscn scene
- FR-2: GuildRosterComponent
- FR-3: GuildStorageComponent
- FR-4: GuildManageComponent
- FR-5: GuildCreateComponent

## Success Metrics

- Guild loads in < 1s
- Storage operations work
- Permissions enforced