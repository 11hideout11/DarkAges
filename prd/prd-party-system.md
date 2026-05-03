# PRD: Party System Implementation

## Introduction

The DarkAges client currently has a hidden party panel but no party system functionality. Players cannot form parties, share loot, or coordinate together. This PRD implements a complete party system with shared experience, loot rules, and party chat.

## Goals

- Allow party creation and invitation
- Share experience with party members
- Implement loot rules (free-for-all, round-robin, need/greed)
- Party chat channel
- Party UI visibility

## User Stories

### PTY-001: Party Creation
**Description:** As a player, I want to create a party so I can play with friends.

**Acceptance Criteria:**
- [ ] Create party command (invites players)
- [ ] Party leader can invite others
- [ ] Max 6 members per party
- [ ] Party created with unique ID

### PTY-002: Party Messaging
**Description:** As a party member, I want to chat only with my party.

**Acceptance Criteria:**
- [ ] /party chat prefix
- [ ] Party chat visible only to members
- [ ] Chat UI shows party icon

### PTY-003: Shared Experience
**Description:** As a party member, I want shared XP when killing mobs.

**Acceptance Criteria:**
- [ ] XP shared among party members in range
- [ ] Bonus for being in parties
- [ ] Kill credit to attacker

### PTY-004: Loot System
**Description:** As a party member, I want loot rules so it's fair.

**Acceptance Criteria:**
- [ ] Round-robin loot distribution
- [ ] Need/greed voting (optional)
- [ ] Auto-pass on non-desired items

## Functional Requirements

- FR-1: PartyCreatePacket / PartyInvitePacket
- FR-2: Server-side party state management
- FR-3: PartyChatPacket for chat routing
- FR-4: XP share calculation in ExperienceSystem
- FR-5: Loot roll system integration

## Non-Goals

- No raid system (40-player)
- No party finder/lfg system
- No instant join (invite only)

## Technical Considerations

### Server Party State
```cpp
struct Party {
    uint32_t partyId;
    EntityID leader;
    std::vector<EntityID> members;
    LootRule lootRule;
    uint32_t createdAt;
};
```

### Network Protocol
- PACKET_PARTY_CREATE (type=20)
- PACKET_PARTY_INVITE (type=21)
- PACKET_PARTY_JOIN (type=22)
- PACKET_PARTY_LEAVE (type=23)
- PACKET_PARTY_CHAT (type=24)

## Success Metrics

- Party can form with 2-6 players
- Party chat works
- Shared XP visible

## Open Questions

- Default loot rule?
- Party XP bonus percentage?