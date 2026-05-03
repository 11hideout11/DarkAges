# PRD: Guild System Implementation

## Introduction

DarkAges lacks any guild/organization system. Players cannot form persistent groups with shared identity, guild chat, or member management. This PRD implements a basic guild system for long-term social gameplay.

## Goals

- Guild creation with name and tagline
- Member roster management
- Guild chat channel
- Guild rank system
- Shared guild bank (future)

## User Stories

### GLD-001: Guild Creation
**Description:** As a player, I want to create a guild with a unique name.

**Acceptance Criteria:**
- [ ] Create guild command with name
- [ ] Name uniqueness check
- [ ] Guild founder becomes leader
- [ ] Tagline optional

### GLD-002: Member Management
**Description:** As a guild leader, I want to manage members.

**Acceptance Criteria:**
- [ ] Invite players to guild
- [ ] Remove members
- [ ] Promote/demote ranks
- [ ] View online members

### GLD-003: Guild Chat
**Description:** As a guild member, I want to chat with all guild members.

**Acceptance Criteria:**
- [ ] /guild chat prefix
- [ ] Cross-zone chat
- [ ] Guild member list shown

### GLD-004: Guild Ranks
**Description:** As a guild leader, I want ranks with different permissions.

**Acceptance Criteria:**
- [ ] Default ranks (Leader, Officer, Member)
- [ ] Custom rank names
- [ ] Permission per rank

## Functional Requirements

- FR-1: Guild database in ScyllaDB
- PACKET_GUILD_CREATE (type=30)
- PACKET_GUILD_INVITE (type=31)
- PACKET_GUILD_CHAT (type=32)
- FR-2: Guild rank permissions

## Non-Goals

- No guild bank
- No guild upgrades
- No guild wars

## Technical Considerations

### Database Schema
```sql
CREATE TABLE guilds (
    guild_id BIGINT PRIMARY KEY,
    name TEXT,
    tagline TEXT,
    leader_id BIGINT,
    created_at TIMESTAMP,
    ranks JSON
);

CREATE TABLE guild_members (
    guild_id BIGINT,
    player_id BIGINT,
    rank INT,
    joined_at TIMESTAMP,
    PRIMARY KEY (guild_id, player_id)
);
```

## Success Metrics

- Guild can be created
- Members can join
- Guild chat works

## Open Questions

- Max guild size?
- Creation cost (in-game gold)?