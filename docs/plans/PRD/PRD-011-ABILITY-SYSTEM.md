# PRD-011: Skill/Ability System

**Version:** 1.0  
**Status:** 🟡 Planned  
**Owner:** SERVER_AGENT  
**Priority:** CRITICAL  
**Dependencies:** PRD-003 (Combat System), PRD-010 (GCD)

---

## 1. Overview

### 1.1 Purpose
Implement a data-driven skill/ability system that supports multiple ability types (active, passive, buff, debuff), ability RPC handshake, and integration with the Global Cooldown System.

### 1.2 Scope
- Ability database with JSON/proto definitions
- Ability component per entity
- Casting system with channel time and interrupt
- Effects system (damage, heal, buffs)
- Client ability bar integration

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| SKL-001 | Data-driven ability definitions (JSON/proto) | P0 | AbilityDatabase singleton |
| SKL-002 | Ability types: Active, Passive, Buff, Debuff | P0 | Type-specific behavior |
| SKL-003 | RPC handshake (request → validation → response) | P0 | Client-server handshake |
| SKL-004 | Ability casting (channel time, interrupt) | P1 | Cast bar integration |
| SKL-005 | Ability effects (damage, heal, buff) | P0 | Effect application system |
| SKL-006 | Ability slots (hotbar) management | P1 | 10 slot hotbar |

### 2.2 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| Ability Lookup | <0.1ms | <0.5ms |
| Effect Application | <0.5ms | <1ms |
| Memory (abilities) | <10KB | <50KB |

---

## 3. Current Gap

**Gap:** Only basic melee/ranged combat implemented. No data-driven ability system exists. Referenced in PRD-003 but not specified.

**Location:** src/server/include/combat/ (new files needed)

---

## 4. Architecture

### 4.1 Server Architecture

```
AbilityDatabase (singleton):
  - abilities: unordered_map<AbilityID, AbilityDefinition>
  - LoadFromProto(): void
  - GetAbility(id):AbilityDefinition*
  
AbilityDefinition:
  - id: AbilityID
  - name: string
  - type: enum (Active, Passive, Buff, Debuff)
  - cooldown_ms: uint16
  - cast_time_ms: uint16
  - effects: vector<AbilityEffect>
  - requirements: AbilityRequirements
  - target_type: enum (Self, Enemy, Ally, Area)

AbilityComponent (per entity):
  - equippedAbilities: array<AbilityID, 10>
  - activeBuffs: vector<BuffState>
  - castingState: CastingState
  
AbilitySystem:
  - ProcessAbilityRequest(entity, abilityID) -> AbilityResult
  - ApplyAbilityEffects(entity, target, ability) -> void
  - UpdatePassiveAbilities(entity) -> void
```

### 4.2 Client Architecture

```
AbilityBar:
  - slots: array<AbilitySlot, 10>
  - UpdateSlot(abilityID, state): void
  - OnSlotActivated(slot): void
  
CastBar:
  - ShowCastProgress(ability, duration): void
  - InterruptCast(): void
  
AbilityIcon:
  - cooldown overlay
  - hotkey binding display
```

### 4.3 Network Protocol

```
C2S_AbilityUse:
  - ability_id: uint32
  - target_entity: uint64
  - position: Vector3
  
S2C_AbilityResult:
  - ability_id: uint32
  - result: enum (Success, Failed, Interrupted)
  - effects: vector<Effect>
  
S2C_AbilitySync:
  - equipped_abilities: array<uint32, 10>
  - active_buffs: vector<BuffState>
```

---

## 5. Implementation Phases

### Phase 1: Ability Database
- AbilityDefinition struct
- AbilityDatabase singleton
- JSON/proto loading
- Initial ability definitions

### Phase 2: Ability Component
- AbilityComponent per entity
- Equipped abilities management
- Passive ability handling
- Buff/debuff state tracking

### Phase 3: Casting System
- Cast bar integration
- Channel time handling
- Interrupt handling
- Cast cancellation

### Phase 4: Effects System
- Damage effects
- Heal effects
- Buff application
- Debuff application

### Phase 5: Client Ability Bar
- AbilityBar modifications
- Icon display
- Cooldown overlay
- Cast bar integration

---

## 6. Deliverables

### Server Files
- `src/server/include/combat/AbilityComponent.hpp`
- `src/server/include/combat/AbilityDatabase.hpp`
- `src/server/include/combat/AbilitySystem.hpp`
- `src/server/include/combat/AbilityDefinition.hpp`
- `src/server/src/combat/AbilityComponent.cpp`
- `src/server/src/combat/AbilityDatabase.cpp`
- `src/server/src/combat/AbilitySystem.cpp`
- `src/server/src/combat/AbilityDefinition.cpp`

### Protocol
- `src/shared/proto/ability.proto` (new)

### Data
- `data/abilities/abilities.json` (new)
- `data/abilities/melee.json` (Slash, Strike)
- `data/abilities/ranged.json` (Shot, Arcane Bolt)
- `data/abilities/buffs.json` (Might, Haste)

### Client Files
- `src/client/src/combat/AbilityBar.cs` (modifications)
- `src/client/src/combat/CastBar.cs` (new)
- `src/client/src/combat/AbilityIcon.cs` (new)

### Tests
- `src/server/tests/TestAbilitySystem.cpp` (new)

---

## 7. Testing

| Test | Location | Criteria |
|------|----------|----------|
| Ability lookup | Unit | <0.1ms |
| Effect application | Unit | <0.5ms |
| Passive activation | Integration | Auto-apply on equip |
| Buff stacking | Unit | Max 3 stacks |
| Casting interrupt | Integration | Client cancel works |

---

## 8. Acceptance Criteria

- [ ] Data-driven abilities load from JSON/proto
- [ ] Active abilities respect GCD
- [ ] Passive abilities apply automatically on equip
- [ ] Buffs/Debuffs stack correctly (max 3)
- [ ] Casting system works with interrupt
- [ ] Client ability bar displays abilities
- [ ] No test regressions (baseline: 2,097 cases)
- [ ] Build passes: cmake --build build -j$(nproc)

---

*Last Updated: 2026-05-01*