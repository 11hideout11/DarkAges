# PRD-011: Skill/Ability System

**Version:** 1.0  
**Status:** 🔴 Not Started  
**Owner:** SERVER_AGENT  
**Priority:** CRITICAL  
**Dependencies:** PRD-003 (Combat System), PRD-010 (GCD)

---

## 1. Overview

### 1.1 Purpose
Implement a data-driven skill/ability system that enables diverse combat abilities beyond basic melee/ranged attacks. Supports active abilities, passive abilities, buffs, and debuffs with RPC handshake validation.

### 1.2 Scope
- Ability database with JSON definitions
- Active abilities (with casting time, channeling, interrupts)
- Passive abilities (auto-applied)
- Buff/debuff system with stacking rules
- Client ability bar integration

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| SKL-001 | Data-driven ability loading from JSON | P0 | ability_database.json |
| SKL-002 | Ability types: Active, Passive, Buff, Debuff | P0 | Type enumeration |
| SKL-003 | RPC handshake (request → validation → response) | P0 | Server validation |
| SKL-004 | Ability casting (channel time, interrupt) | P1 | Cast bar on client |
| SKL-005 | Ability effects (damage, heal, buff) | P0 | Effect application |
| SKL-006 | Ability slots (hotbar) management | P1 | 10 slots per player |
| SKL-007 | Combo system (attack chain) | P1 | Attack_1 → Attack_2 |
| SKL-008 | Passive buff application | P0 | Auto-on equip |

### 2.2 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| Ability Lookup | <0.1ms | <0.5ms |
| Effect Application | <0.5ms | <1ms |
| Database Load Time | <100ms | <500ms |

---

## 3. Current Gap

**Gap:** Only basic melee/ranged combat implemented. No data-driven ability system exists. Referenced in PRD-003 but not specified.

**Location:** src/server/include/combat/ (empty)

---

## 4. Implementation Strategy

### 4.1 Ability Database

```
AbilityDatabase (singleton):
  - abilities: unordered_map<AbilityID, AbilityDefinition>
  - LoadFromJson(file_path): void
  
AbilityDefinition:
  - id: AbilityID
  - name: string
  - type: enum (Active, Passive, Buff, Debuff)
  - cooldown_ms: uint16
  - cast_time_ms: uint16
  - channel_time_ms: uint16
  - effects: vector<AbilityEffect>
  - requirements: AbilityRequirements
  - cost: AbilityCost
```

### 4.2 Ability Component (per entity)

```
AbilityComponent (per entity):
  - equippedAbilities: array<AbilityID, 10>
  - activeBuffs: vector<BuffState>
  - castState: CastState
  
CastState:
  - abilityID: AbilityID
  - startTime: Timestamp
  - channelElapsed: Duration
  - interrupted: bool
```

### 4.3 Combat System Integration

```
CombatSystem:
  - ProcessAbilityRequest(entity, abilityID) -> AbilityResult
  - ValidateAbility(entity, abilityID) -> ValidationResult
  - ApplyAbilityEffects(source, target, ability) -> void
  - ApplyBuff(entity, buff) -> void
  - RemoveBuff(entity, buffID) -> void
```

### 4.4 RPC Flow

```
1. Client: ability_use_request (abilityID, target)
2. Server: ValidateAbility() check
   - GCD valid?
   - Cooldown ready?
   - Requirements met? (cost, range, etc.)
3. Server: If channeled → start channel
   - Else: Apply effects immediately
4. Server: ability_use_response (success/fail, effects)
5. Client: On success → play animation, show feedback
   - On fail → show error, rollback prediction
```

---

## 5. Deliverables

### 5.1 Server Files
- `src/server/include/combat/AbilityComponent.hpp`
- `src/server/include/combat/AbilityDatabase.hpp`
- `src/server/include/combat/AbilitySystem.hpp`
- `src/server/src/combat/AbilityComponent.cpp`
- `src/server/src/combat/AbilityDatabase.cpp`
- `src/server/src/combat/AbilitySystem.cpp`

### 5.2 Data Files
- `src/shared/proto/ability.proto` (new)
- `data/abilities/abilities.json` (new)

### 5.3 Client Files
- `src/client/src/combat/AbilityBar.cs` (modifications)
- `src/client/src/combat/CastBar.cs` (new)
- `src/client/src/combat/BuffDisplay.cs` (new)

### 5.4 Test Files
- `src/server/tests/TestAbilitySystem.cpp` (new)

---

## 6. Testing

| Test | Location | Criteria |
|------|----------|--------|
| Ability loading | TestAbilityDatabase | All abilities parse correctly |
| GCD enforcement | TestAbilitySystem | Active abilities respect GCD |
| Passive application | TestAbilitySystem | Passives apply on equip |
| Buff stacking | TestAbilitySystem | Buffs stack per rules |
| Combo chain | Integration | Attack sequence works |
| Channel interrupt | Integration | Channel breaks on damage |

---

## 7. Acceptance Criteria

- [ ] Data-driven abilities load from JSON/proto
- [ ] Active abilities respect GCD and individual cooldowns
- [ ] Passive abilities apply automatically on equip
- [ ] Buffs/Debuffs stack correctly per stacking rules
- [ ] Client ability bar displays all 10 slots
- [ ] Cast bar shows channel progress
- [ ] No test regressions (baseline: 2,097 cases)

---

*Last Updated: 2026-05-01*