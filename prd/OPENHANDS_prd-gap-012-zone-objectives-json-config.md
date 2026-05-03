# PRD: Zone Objectives Integration with JSON Configs

## Introduction

The ZoneObjectiveSystem exists in the server code and can track player objectives during gameplay, but the zone JSON configuration files have zero objectives defined. This prevents the zone objective feature from functioning in any zone. This PRD specifies adding objectives to zone JSON configs to enable objective tracking and client replication.

## Goals

- Add objectives to at least one zone JSON config (training zone as pilot)
- Objectives displayed on client via QuestTracker.tscn
- Objective progress tracked server-side with events
- Objective completion triggers rewards/persistence

## User Stories

### US-001: Zone Config Objectives Field
**Description:** As a game designer, I want to define objectives in the zone JSON config so they spawn with the zone.

**Acceptance Criteria:**
- [ ] Zone JSON supports "objectives" array field
- [ ] Each objective has fields: id, description, targetCount, targetType (kill|collect|interact)
- [ ] Zone loads objectives on initialization
- [ ] Invalid objective config logs warning, doesn't crash

### US-002: Server-Side Tracking
**Description:** As a server, I need to track player progress toward objectives so I can report completion.

**Acceptance Criteria:**
- [ ] ZoneObjectiveComponent tracks current count per objective per player
- [ ] Kill events increment kill-type objectives
- [ ] Progress checkpoint fires at 25%, 50%, 75%, 100%
- [ ] Objective completion emits event to NetworkManager

### US-003: Client Replication
**Description:** As a player, I want to see my objective progress so I know what to do next.

**Acceptance Criteria:**
- [ ] PACKET_ZONE_OBJECTIVE_UPDATE sends to client on login
- [ ] Update packet sent when player makes progress
- [ ] QuestTracker.tscn displays current objectives with progress (e.g., "Rat King: 3/5")
- [ ] Progress updates within 1 second of completing action

### US-004: Completion Rewards
**Description:** As a player completing an objective, I want to receive rewards so I feel accomplished.

**Acceptance Criteria:**
- [ ] XP awarded on objective completion (configurable amount)
- [ ] Completion event logs for debugging
- [ ] Zone objective completion does not persist (resets on zone exit)

## Functional Requirements

- FR-1: Zone JSON schema supports "objectives" array with required fields
- FR-2: ZoneObjectiveSystem.LoadObjectives parses JSON array
- FR-3: ZoneObjectiveComponent initialized per-player on zone entry
- FR-4: ProcessKill event checks objective progress
- FR-5: NetworkManager sends PACKET_ZONE_OBJECTIVE_UPDATE
- FR-6: QuestTracker.tscn receives and displays updates

## Non-Goals

- Persistent quest tracking across zones (session-only)
- Quest chains or dependencies (single objective per entry)
- Quest rewards beyond XP (loot/items reserved for future)
- Server-to-server objective sharing (local zone only)

## Technical Considerations

- Reuse existing ZoneObjectiveSystem and ZoneObjectiveComponent
- Packet type already exists: PACKET_ZONE_OBJECTIVE_UPDATE (Protocol.cpp)
- QuestTracker.tscn already exists (from PRD-030)
- Objectives use existing event system (no new events needed)

## Success Metrics

- Training zone loads with 2 objectives defined
- Client displays objectives within 5 seconds of zone load
- Kill events increment objective counter within 1 second
- All 3 demo zones support objectives after full implementation

## Open Questions

- What specific objectives should training zone have? (e.g., "Kill 10 Rats", "Defeat 1 Boss")
- Should objectives be party-shared or individual?
- What XP amounts for completion? (Suggestion: 100 XP per objective)

---

*Generated: 2026-05-03*