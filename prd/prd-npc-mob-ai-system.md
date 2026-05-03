# PRD: NPC/Mob AI System

## Introduction

Implement a comprehensive AI system for NPCs and mobs that provides diverse enemy behaviors, boss mechanics, patrol patterns, and responseAI. The AI system should support multiple difficulty tiers, agro ranges, ability usage, and scripted behavior sequences.

## Goals

-Create reusable AI behavior trees that can be composed for different enemy types
- Support multiple AI types: idle/patrol, agro/chase, combat, flee, boss phases
- Integrate with existing AbilitySystem for mob ability usage
- Provide boss-specific mechanics (phase transitions, enrage timers, summons)
- Add unit tests for all AI behaviors
- Performance target: 100+ simultaneous AI entities at 60 FPS

## User Stories

### US-001: Basic mob AI behaviors
**Description:** As a game designer, I want mobs to have basic AI behaviors so they can patrol, detect players, and engage in combat.

**Acceptance Criteria:**
- [ ] MobComponent with AIType enum (None, Passive, Aggressive, Boss, Guard, Shopkeeper)
- [ ] Detection radius configurable per mob template
- [ ] State machine: Idle → Aggro → Combat → Victory/Retreat
- [ ] Simple pathfinding (A* or navigation) to player when aggroed
- [ ] AI responds to damage with agro (threat/threat table)

### US-002: Boss AI with phase transitions
**Description:** As a game designer, I want bosses to have multiple phases with different abilities so that encounters are dynamic and challenging.

**Acceptance Criteria:**
- [ ] BossComponent with phase count, current phase, enrage timer
- [ ] Phase transition triggers at health thresholds (e.g., 75%, 50%, 25%)
- [ ] Each phase has different ability pools
- [ ] Enrage timer triggers ultimate attack at timeout
- [ ] Visual indicators for phase changes (animation, particle, chat message)

### US-003: NPC shopkeepers and quest givers
**Description:** As a player, I want to interact with NPCs for trading and quests so the world feels alive.

**Acceptance Criteria:**
- [ ] ShopInterfaceComponent for vendors with buy/sell tabs
- [ ] Interaction radius and greeting dialog
- [ ] QuestGiverComponent with available quest list
- [ ] Quest progress tracking displayed on talk

### US-004: Mob spawn waves
**Description:** As a zone designer, I want spawn waves that trigger based on time or player actions so dynamic encounters exist.

**Acceptance Criteria:**
- [ ] WaveSpawnerComponent with mob templates, count, spawn radius
- [ ] Wave triggers: timer, kill count, event emit
- [ ] Spawn delay configurable per mob
- [ ] Wave completion triggers next wave or victory

### US-005: AI integration with ZoneObjectiveSystem
**Description:** As a zone designer, I want AI mobs to contribute to zone objectives so kills and boss defeats progress objectives.

**Acceptance Criteria:**
- [ ] Kill event emits to ZoneObjectiveSystem
- [ ] Boss kill triggers boss_defeated objective
- [ ] NPC rescue triggers npc_interaction objective
- [ ] Wave completion triggers wave_complete objective

## Functional Requirements

- FR-1: MobComponent with AIType, threat table, detection radius
- FR-2: FiniteStateMachine managing Idle/Aggro/Combat/Flee states
- FR-3: BossComponent with phases, enrage timer, transitions
- FR-4: AbilitySystem integration for mob ability casting
- FR-5: Pathfinding integration for mob movement
- FR-6: WaveSpawnerComponent with configurable waves
- FR-7: QuestGiverComponent and ShopInterfaceComponent
- FR-8: EmitEvent integration for objective tracking

## Non-Goals

- No player companion AI (pets, summons) - separate feature
- No procedural dungeon generation
- No AI personality/vocalization
- No complex pathfinding (use simple seeking)

## Technical Considerations

- Reuse existing ECS architecture with EnTT
- MobComponent lives on entity with Position, Velocity
- AI tick rate: 10Hz (every 6 ticks) for performance
- Behavior trees use simple if/else chains first

## Success Metrics

- 100+ simultaneous AI entities at 60 FPS
- Boss encounter phases work correctly
- Zone objectives track AI interactions

## Open Questions

- Should mob AI be server-authoritative for remote players?
- Pathfinding: use navmesh or simple vector seeking?
- How to serialize AI state for mid-combat disconnects?