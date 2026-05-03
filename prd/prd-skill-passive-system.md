# PRD: Skill & Passive System

## Introduction

Implement a skill and passive system that handles active skill activation (spells/abilities in combat), passive skill bonuses (always-on effects), skill trees by class archetype, skill unlock requirements, and skill cooldown management. This extends the existing AbilitySystem with deeper RPG progression.

## Goals

- Add SkillComponent for unlocked skills and passive bonuses
- Implement skill activation in combat (extends AbilitySystem)
- Add passive skill application (stat bonuses always active)
- Support skill trees: Warrior (Strength), Mage (Intelligence), Rogue (Dexterity)
- Add skill unlock requirements (level, previous skill, quest)
- Integrate skill usage with combat state machine

## User Stories

### US-001: Skill activation in combat
**Description:** As a player, I want to activate skills in combat so I can use my character's abilities.

**Acceptance Criteria:**
- [ ] Skill hotbar shows 12 active skill slots
- [ ] Skills mapped to hotkeys (1-6, Shift+1-6)
- [ ] Skill activation respects global cooldown
- [ ] Skill animation plays on activation
- [ ] Skill effect applies to target(s)
- [ ] Skill goes on cooldown after use

### US-002: Passive skills
**Description:** As a player, I want passive skills so I get bonuses without active use.

**Pass-ive acceptance criteria:**
- [ ] Passive skills always apply when unlocked
- [ ] Passive bonuses shown in character stats UI
- [ ] No cooldown or mana cost for passives
- [ ] Passive effects recalc on stat change
- [ ] Max 10 passive skills

### US-003: Skill tree
**Description:** As a player, I want a skill tree so I can invest in a class path.

**Acceptance criteria:**
- [ ] Three trees: Warrior, Mage, Rogue
- [ ] Skills have prerequisites (unlock 1-2 before 3)
- [ ] Visual skill tree UI with connecting lines
- [ ] Points spent tracked per tree
- [ ] Cannot respec (hard earned) or respec potion available

### US-004: Skill unlocks
**Description:** As a game designer, I want skill unlock requirements so progression matters.

**Acceptance Criteria:**
- [ ] Level requirement per skill (5, 10, 15, 20...)
- [ ] Previous skill requirement (skill A for skill B)
- [ ] Quest completion requirement
- [ ] Item requirement (learned from scroll)
- [ ] Cannot use locked skills

### US-005: Cooldown management
**Description:** As a game designer, I want skill cooldowns so abilities aren't spammable.

**Acceptance Criteria:**
- [ ] Individual skill cooldowns (5s, 30s, 2m...)
- [ ] Global cooldown (GCD) of 1 second
- [ ] Cooldown display in skill icons
- [ ] Cooldown persists across death
- [ ] Cooldown resets on login

## Functional Requirements

- FR-1: SkillComponent tracking unlocked skills, passive skills
- FR-2: SkillTreeComponent per archetype
- FR-3: SkillActivationComponent triggers effects
- FR-4: PassiveBonusComponent applies stat modifiers
- FR-5: CooldownManagerComponent tracks timer
- FR-6: GlobalCooldownComponent (GCD)
- FR-7: SkillHotbarComponent maps keys

## Non-Goals

- No skill crafted via crafting system
- No skill scrolls for other players
- No pets or minions (separate)
- No stance or form switching

## Technical Considerations

- Skills extend AbilitySystem (reuse serialize/deserialize)
- Skills use CombatEventSystem for effects
- Passive bonuses applied as stat modifiers

## Success Metrics

- Skill activation < 50ms latency
- 50+ skills functional
- No double activation exploits

## Open Questions

- How many skills per tree? (10-20)
- Max skill points: earn or purchase?
- Prestige respec: allowed?