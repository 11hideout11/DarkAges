# PRD: Player Persistence System

## Introduction

Implement player data persistence so player progress (level, inventory, achievements, faction) persists across sessions. Currently players lose all progress on disconnect - this is unacceptable for an MMO.

## Goals

- Player data persists across server restarts
- Support full inventory, equipment, currency, achievements, quest progress, faction standing
- Support character slots (multiple characters per account)
- Handle offline progression (mail, achievements, faction)
- Atomic saves to prevent data loss on crash

## User Stories

### US-001: Player Data Model
**Description:** As a developer, I need a persistent data model so player progress survives restarts.

**Acceptance Criteria:**
- [ ] Player profile table with: account_id, character_id, name, level, experience, currency, faction_standing (JSON/column)
- [ ] Inventory table: item_id, slot, quantity, enchantments (JSONB)
- [ ] Quest progress table: quest_id, status, objectives_completed (JSON)
- [ ] Achievements table: achievement_id, timestamp, progress
- [ ] Support multiple characters per account (character slots)

### US-002: Save/Load API
**Description:** As a player, I want my progress saved automatically so I don't lose it when logging off.

**Acceptance Criteria:**
- [ ] Auto-save on: logout, zone transition, inventory change, quest complete, level up (debounced, max once per 30s)
- [ ] Load on spawn: fetch persisted data, apply to entity, handle missing fields gracefully
- [ ] Save is atomic: all related tables update in single transaction

### US-003: Character Selection
**Description:** As a returning player, I want to see my characters so I can choose which to play.

**Acceptance Criteria:**
- [ ] Character list endpoint returns all characters for account with: name, class, level, last_login
- [ ] Character create/delete endpoints with validation (max 4 characters)
- [ ] New character gets starter items from data/spawns.json starter_loadout

### US-004: Offline Processing
**Description:** As a developer, I need to handle events that occur while player is offline.

**Acceptance Criteria:**
- [ ] Mail delivery works for offline players (queued and delivered on next login)
- [ ] Achievement授予 works for offline players
- [ ] Faction standing changes from world events queue for offline

## Functional Requirements

- FR-1: Redis profile cache with Scylla persistence (when DB enabled)
- FR-2: JSON serialization for complex fields (inventory, quest progress)
- FR-3: Debounced auto-save prevents save storm on rapid actions
- FR-4: Character slots limit: 4 per account, configurable
- FR-5: Save validation: verify data integrity before apply to ECS
- FR-6: Handle migration: existing players get new fields with defaults

## Non-Goals

- Cloud sync across devices (future feature)
- Cross-region character transfer
- GM character management UI (separate PRD)

## Technical Considerations

- Reuse existing Redis infrastructure when ENABLE_REDIS=ON
- FlatBuffers for profile serialization (extend Protocol.fbs)
- Integrate with ZoneServer::OnPlayerDisconnect
- Use transaction batching for related saves

## Success Metrics

- Player data persists after server crash (verified by test)
- < 100ms load time for typical character
- Zero data loss in 1000 auto-save cycles (stress test)

## Open Questions

- Should we use incremental delta saves or full profile saves?
- Lifetime of unsaved data in Redis before Scylla dump?