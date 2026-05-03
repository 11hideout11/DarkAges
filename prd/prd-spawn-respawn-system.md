# PRD: Spawn Point & Respawn System

## Introduction

Implement a comprehensive spawn point and respawn system that handles player death, corpse retrieval (if applicable), respawn location selection, death penalties, and bound locations. The system should handle both immediate respawn and timed respawn with death countdown.

## Goals

- Add SpawnPointComponent storing bound locations per player
- Implement death state machine with death → respawn timer → respawn flow
- Support multiple respawn options: nearest, bind, town, cemetery
- Add death penalty system (optional XP loss, durability loss)
- Integrate with existing ZoneObjectiveSystem for death tracking
- Add client-side death screen with respawn button

## User Stories

### US-001: Death and respawn flow
**Description:** As a player, when I die I want to respawn so I can continue playing.

**Acceptance Criteria:**
- [ ] Player enters death state on HP <= 0
- [ ] Death screen displays with respawn button after 3-second delay
- [ ] Respawn at nearest valid spawn point
- [ ]Corpse remains at death location for retrieval (optional)
- [ ] Death counter increments for achievements

### US-002: Bind points and home city
**Description:** As a player, I want to set a home point so I can respawn there after death.

**Acceptance Criteria:**
- [ ] Bind locations stored: bind_point (1), bind_list (5 max)
- [ ] /bind command sets current location as bind point
- [ ] InneTavern bind at level 1 automatically
- [ ] Respawn menu shows available bind points
- [ ] Home point marked on minimap

### US-003: Death penalties
**Description:** As a game designer, I want death to have consequences so players value survival.

**Acceptance Criteria:**
- [ ] XP debt system: lose XP, not levels (cannot drop below level 1 XP)
- [ ] Durability loss on equipment
- [ ] Optional gold loss (10% of carried gold)
- [ ] Death streak tracking (increased penalty after 3+ deaths)
- [ ] Pvp death no penalty in safe zones

### US-004: Corpse retrieval
**Description:** As a player, I want to retrieve my corpse so I don't lose items.

**Acceptance Criteria:**
- [ ] Corpse spawns at death location with inventory copy
- [ ] Corpse despawns after 5 minutes (or configurable)
- [ ] Other players cannot loot your corpse
- [ ] Resurrection at corpse returns HP to threshold
- [ ] Corpse retrieval counted as objective progress

### US-005: PvP death handling
**Description:** As a PvP designer, I want PvP deaths to have different penalties so combat is balanced.

**Acceptance Criteria:**
- [ ] PvP death: reduced/negligible penalty
- [ ] Killer gets honor points / contribution
- [ ] No corpse from PvP death
- [ ] Respawn at battlefield entry point
- [ ] Kill streak bonuses for multiple kills

## Functional Requirements

- FR-1: SpawnPointComponent storing bind locations
- FR-2: DeathState managing died → timer → respawn flow
- FR-3: DeathPenaltyCalculator for XP/durability/gold
- FR-4: CorpseComponent for death location persistence
- FR-5: BindCommandComponent for /bind command
- FR-6: Respawn menu UI with options
- FR-7: ZoneObjectiveSystem integration for death tracking

## Non-Goals

- No graveyard/skeleton running (simple corpse only)
- No resurrection spells from other players
- No deathmatch/arena specific spawns
- No instanced dungeon spawns (use zone defaults)

## Technical Considerations

- Death state stored in PlayerComponent
- Respawn position calculated on respawn
- Death timer uses server tick counting
- Corpse uses timed entity lifetime

## Success Metrics

- Respawn completes in < 2 seconds
- Death penalty applies correctly
- Bind saves persist across sessions

## Open Questions

- Corpse retrieval: worth the complexity for MVP?
- XP debt: how to handle level 1 players?
- PvP honor: store where (PlayerComponent)?