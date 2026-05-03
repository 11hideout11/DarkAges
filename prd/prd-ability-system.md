# PRD: Combat Ability System Completion

## Introduction

The DarkAges server has a partial AbilitySystem implementation but lacks ability definitions, client ability execution, and ability UI. The ability system is needed for meaningful player progression and combat variety beyond basic attacks.

## Goals

- Define ability templates with effects
- Implement ability execution pipeline
- Add ability network protocol
- Create ability hotbar UI
- Support ability upgrades/upgrade paths

## User Stories

### AB-001: Ability Definitions
**Description:** As a combat designer, I want abilities defined in data so they can be balanced.

**Acceptance Criteria:**
- [ ] AbilityDefinition struct with ID, name, cooldown, cost
- [ ] AbilityEffect struct for damage/healing/buff
- [ ] Database of starting abilities per class
- [ ] Ability ID protocol registration

### AB-002: Ability Execution
**Description:** As a player, I want to activate abilities.

**Acceptance Criteria:**
- [ ] Hotkey (1-6) or UI click triggers ability
- [ ] Client sends AbilityInputPacket
- [ ] Server validates and executes ability
- [ ] Effect applied to target

### AB-003: Ability UI
**Description:** As a player, I want to see my abilities and their cooldowns.

**Acceptance Criteria:**
- [ ] 6-slot hotbar at bottom of screen
- [ ] Ability icons in slots
- [ ] Cooldown overlay on cooldown
- [ ] Keybind display (1-6)

### AB-004: Ability Networking
**Description:** As a server, I need ability state network-synchronized.

**Acceptance Criteria:**
- [ ] Client receives ability list on login
- [ ] Cooldown sync in entity snapshot
- [ ] Ability cast events broadcast to observers
- [ ] Cast failure handled gracefully

## Functional Requirements

- FR-1: AbilityDefinition struct with effects, cooldown, resources
- FR-2: AbilityInputPacket with ability ID, target
- FR-3: AbilitySystem::execute ability validation and effects
- FR-4: Create AbilityHotbar UI scene
- FR-5: Protocol ability list sync
- FR-6: Cast event to CombatEventHandler

## Non-Goals

- No complex ability trees (unlock progression)
- No passive abilities initially
- No ability animations (use attack animation)
- No combo abilities

## Design Considerations

### Ability Definition Structure
```cpp
enum class AbilityTarget { Self, Enemy, Ally, Ground };
enum class AbilityEffectType { Damage, Heal, Shield, Buff, Debuff };

struct AbilityDefinition {
    uint32_t abilityId;
    std::string name;
    AbilityTarget target;
    AbilityEffectType effectType;
    int16_t baseValue;
    uint16_t cooldownMs;
    uint8_t resourceCost;
    float range;
};

struct AbilityEffect {
    EntityID caster;
    EntityID target;
    uint32_t abilityId;
    int16_t value;
    uint32_t timestamp;
};
```

### Ability UI Implementation
- HorizontalBoxContainer with 6 TextureRect slots
- CooldownTimer for each slot
- KeyLabel overlay showing 1-6

### Starting Abilities for MVP
1. Slash (basic melee, instant)
2. Heavy Strike (delayed damage)
3. Shield Block (temporary defense)
4. Heal (self-heal, cooldown)

## Technical Considerations

### Network Protocol
- Server->Client: AbilityListPacket (abilities owned)
- Client->Server: AbilityInputPacket (activate)
- Server->Client: AbilityResultPacket (success/fail)
- Server->Observer: CombatEvent with ability ID

### Validation Pipeline
1. Check ability ID exists
2. Check cooldown expired
3. Check resources sufficient
4. Check target in range
5. Execute ability effect

## Success Metrics

- Player can cast abilities with hotkeys
- Cooldowns display correctly in UI
- Server-side ability validation works
- Combat event shows ability source

## Open Questions

- Should abilities require targeting (ground/range)?
- How do we handle ability animation variation?
- Should there be ability combo system?