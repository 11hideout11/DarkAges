# PRD: Boss Zone NPC Spawning Implementation

## Introduction

The boss zone currently has 0 NPC presets defined in `boss.json`, preventing the demo zone from being playable end-to-end. This PRD specifies the requirements for populating the boss zone with a functional boss entity that has combat behavior.

## Goals

- Add at least 1 boss NPC preset to `boss.json` with unique ID and combat stats
- Implement boss entity spawn logic in ZoneServer
- Boss should display health bar and engage in combat with players
- Boss should have attack patterns and phases for engaging gameplay
- Boss death should trigger victory condition and zone completion

## User Stories

### US-001: Boss NPC Preset Definition
**Description:** As a game designer, I need a boss NPC preset defined in the data file so that the spawn system can create the boss entity.

**Acceptance Criteria:**
- [ ] Boss entry exists in `boss.json` with fields: id, name, model, hp, maxHp, damage, defense, attackPatterns, phaseCount
- [ ] Boss HP is significantly higher than regular NPCs (e.g., 5000+ HP vs 100-200 HP for regular mobs)
- [ ] Attack patterns defined (e.g., melee swing, area blast, charge)
- [ ] Phase transitions defined for boss HP thresholds (e.g., 66%, 33% HP)

### US-002: Boss Spawn in Zone
**Description:** As a player entering the boss zone, I need the boss to spawn automatically so combat can begin.

**Acceptance Criteria:**
- [ ] Boss entity spawns at configured spawn point when zone loads
- [ ] Boss appears with correct model and initial stats
- [ ] Boss health bar displays above boss (visible to players)
- [ ] Boss targets nearest player and begins combat AI

### US-003: Combat Behavior
**Description:** As a player fighting the boss, I want engaging combat patterns so the fight feels challenging.

**Acceptance Criteria:**
- [ ] Boss executes attack patterns at random intervals (every 3-5 seconds)
- [ ] Boss changes phases at defined HP thresholds
- [ ] Visual feedback on phase transition (color change, model animation)
- [ ] Boss deals meaningful damage to players (50-100 HP per hit)

### US-004: Victory Condition
**Description:** As a player defeating the boss, I want the zone to register completion so I receive rewards.

**Acceptance Criteria:**
- [ ] Zone completion event triggers when boss HP reaches 0
- [ ] Players in zone receive XP and loot
- [ ] Victory message displayed to all players
- [ ] Zone resets after configurable time (5 minutes)

## Functional Requirements

- FR-1: Add `boss.json` with at least 1 boss preset (5000+ HP, named "Dungeon Lord")
- FR-2: ZoneServer loads boss entity on boss zone initialization
- FR-3: Boss entity uses existing NPC AI system with extended stats
- FR-4: Boss health bar rendered via CombatTextSystem or dedicated UI
- FR-5: Boss death triggers `ZONE_COMPLETE` event with boss_killed flag

## Non-Goals

- Multiple boss types in single zone (single boss per zone initially)
- Boss dialogue or interaction before combat
- Complex multi-phase mechanics beyond HP thresholds
- Boss-specific loot tables (use generic high-value loot for now)

## Technical Considerations

- Reuse existing NPC entity system (Position, Velocity, CombatStats components)
- Boss entity may need dedicated component for phase tracking (BossPhaseComponent)
- Network replication: boss health needs same snapshot sync as player health
- Client: health bar may reuse existing health bar UI component

## Success Metrics

- Boss zone loads with boss entity in under 2 seconds
- Boss engages combat within 5 seconds of player approach
- Boss fight lasts 30-120 seconds for 4-player group

## Open Questions

- Should boss spawn at fixed location or random spawn point?
- Should boss reset if all players die, or require zone restart?
- Any specific attack patterns needed beyond melee?

---

*Generated: 2026-05-03*