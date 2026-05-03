# PRD: Experience & Leveling System

## Introduction

Implement a comprehensive experience and leveling system that handles XP gain from kills, quests, crafting, exploration; level-up progression with stat increases; skill point allocation; and max level gating. The system should be flexible enough to support multiple progression paths and future expansion.

## Goals

- Add LevelComponent storing level, current XP, XP to next level, skill points
- Implement XP gain sources: kills, quests, crafting, exploration, Events
- Add level-up stat increases per class archetype
- Add skill point allocation system (active skills)
- Integrate with existing QuestSystem and AbilitySystem
- Add level-based stat multipliers for balance

## User Stories

### US-001: Level progression
**Description:** As a player, I want to gain experience and level up so my character grows stronger.

**Acceptance Criteria:**
- [ ] Level range: 1-99 with configurable max
- [ ] XP curve: exponential (100, 300, 600, 1000... scaled)
- [ ] Level-up triggers stat increases
- [ ] Level-up notification (chat + screen effect)
- [ ] Max level message when at cap

### US-002: XP from kills
**Description:** As a player, I want to gain XP from defeating enemies so combat is rewarding.

**Acceptance Criteria:**
- [ ] XP per kill based on mob level relation (level +/- 5 range)
- [ ] Bonus XP for solo kills, party-shared XP
- [ ] Boss kill XP bonus (10x standard)
- [ ] Kill event integrates with XP system
- [ ] XP distribution via party share formula

### US-003: XP from quests
**Description:** As a player, I want to gain XP from completing quests so questing is rewarding.

**Acceptance Criteria:**
- [ ] Quest rewards include XP amount
- [ ] Quest turn-in triggers XP gain
- [ ] XP scales with quest difficulty tier
- [ ] Chain quest XP bonus
- [ ] Repeatable quest XP cap per day

### US-004: Skill point allocation  
**Description:** As a player, I want to allocate skill points so I can customize my build.

**Acceptance Criteria:**
- [ ] Skill points awarded on level-up (1-3 per level)
- [ ] Skill tree UI showing available skills
- [ ] Skill activation requires point spent
- [ ] Skill respec potion (rare item, costs gold)
- [ ] Max 20 active skill slots

### US-005: XP from exploration
**Description:** As a player, I want to gain XP for exploring new areas so exploration is rewarding.

**Acceptance Criteria:**
- [ ] Exploration zones: first visit bonus XP
- [ ] Hidden object discovery XP
- [ ] New map region reveal bonus
- [ ] Exploration tracked per zone/bool flag
- [ ] Achievement tracks total zones explored

## Functional Requirements

- FR-1: LevelComponent with level, xp, xp_to_level, skill_points, stat_bonuses
- FR-2: XPCalculator with kill/quest/crafting/exploration sources
- FR-3: LevelUpEvent triggers stat application
- FR-4: SkillPointComponent for allocation
- FR-5: StatIncreaseComponent applying level bonuses
- FR-6: PartyXPShareCalculator

## Non-Goals

- No prestige/re-spec at max level (future)
- No seasonal/soft resets
- No macro farming detection (anti-cheat scope)
- No auction house or trading post XP

## Technical Considerations

- LevelComponent serialized to database
- XP thresholds stored in Constants.hpp
- Skill tree uses AbilitySystem integration

## Success Metrics

- Level-up processes in < 100ms
- XP distribution correct in parties
- No XP loss on disconnect

## Open Questions

- Class system: warrior/mage/rogue bonuses?
- XP curve: flat or scaled per tier?
- Max level: 60, 80, or 99?