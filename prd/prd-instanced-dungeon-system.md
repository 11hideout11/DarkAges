# PRD: Instanced Dungeon System

## Introduction

Implement an instanced dungeon system that enables private zone instances for party/guild runs, raids, and solo challenges. The system should handle instance creation, management, expiration, player entry/exit, and instance persistence for mid-dungeon disconnects.

## Goals

- Add InstanceComponent for instance metadata (owner, party, max_players, duration)
- Implement instance creation on party entry (if configured)
- Add instance-specific zone data (enemies, objectives unique to instance)
- Handle player entry/exit from instances
- Add instance timeout and cleanup
- Integrate with existing PartyComponent and GuildComponent
- Support raid instances (10-40 players)

## User Stories

### US-001: Instance creation
**Description:** As a party leader, I want to create a dungeon instance so my party can run together privately.

**Acceptance Criteria:**
- [ ] Party leader initiates dungeon entry
- [ ] Instance created with unique ID
- [ ] Party members transferred to instance
- [ ] Instance tracks party_id for entry
- [ ] Instance owner shown in party UI

### US-002: Instance entry/exit
**Description:** As a player, I want to enter and exit instances so I can participate in dungeon runs.

**Acceptance Criteria:**
- [ ] Portal or command to enter (/enter <dungeon>)
- [ ] Entry restricted by level, quest, party size
- [ ] Exit returns to entry location
- [ ] Kicked from instance on logout
- [ ] Re-entry allowed if instance active

### US-003: Instance persistence
**Description:** As an operator, I want instances to persist so players don't lose progress on disconnect.

**Acceptance Criteria:**
- [ ] Instance saved to database periodically
- [ ] Party reconnection restores instance state
- [ ] Instance timeout at 30 minutes of inactivity
- [ ] Timeout warning at 5 minutes
- [ ] Auto-cleanup on party disband

### US-004: Raid instances
**Description:** As a raid leader, I want raid instances so large groups can tackle challenging content.

**Acceptance Criteria:**
- [ ] Raid capacity: 10-40 players
- [ ] Instance scales to player count
- [ ] Raid loot distribution rules
- [ ] Multiple instance leaders supported
- [ ] Ready check system before entry

### US-005: Instance scaling
**Description:** As a game designer, I want instances to scale so difficulty matches party size.

**Acceptance Criteria:**
- [ ] Enemy count scales linearly with player count
- [ ] Boss HP scales with player count
- [ ] Bonus rewards for full group
- [ ] Difficulty modes: normal, hard, nightmare
- [ ] Rewards scaled by difficulty

## Functional Requirements

- FR-1: InstanceComponent with id, owner, zone, players, state
- FR-2: InstanceManager creating/cleanup instances
- FR-3: InstanceEntryComponent handling transfer
- FR-4: InstancePersistence to database
- FR-5: InstanceTimeout cleanup logic
- FR-6: DifficultyScaler for scaling

## Non-Goals

- No cross-server instances
- No persistent house instances (future)
- No arena match instances
- No procedurally generated instances (static only)

## Technical Considerations

- Instance uses zone_id + instance_id compound key
- Instance state serialized to Redis
- Separate tick loop per instance

## Success Metrics

- Instance creation in < 1 second
- 10+ concurrent instances
- No lost progress on disconnect

## Open Questions

- Instance cap per server?
- Entry restrictions per instance?
- Loot rules?