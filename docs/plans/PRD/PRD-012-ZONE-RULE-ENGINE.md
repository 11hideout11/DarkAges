# PRD-012: Zone Rule Engine

**Version:** 1.0  
**Status:** 🟡 Planned  
**Owner:** SERVER_AGENT  
**Priority:** HIGH  
**Dependencies:** PRD-009 (Demo Zones), PRD-004 (Sharding)

---

## 1. Overview

### 1.1 Purpose
Implement a zone rule engine that applies zone-specific rules (PvP, PvE, Tutorial, Raid) and enables the multi-zone demo system defined in PRD-009.

### 1.2 Scope
- Zone rule data model and enforcement
- Rule evaluation hooks in combat
- Zone transition logic
- Demo zone configurations

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| ZRE-001 | Zone rules: PvP enabled/disabled | P0 | Global PvP toggle |
| ZRE-002 | Zone rules: Tutorial (safe area) | P0 | No PvP, training dummies |
| ZRE-003 | Zone rules: Arena (team-based) | P1 | Team assignment |
| ZRE-004 | Zone rules: Raid (boss mechanics) | P1 | Boss phase system |
| ZRE-005 | Zone rule validation hook | P0 | Combat evaluation |
| ZRE-006 | Zone transition rules | P1 | Level requirements |
| ZRE-007 | Zone spawn configuration | P1 | Wave-based spawning |

### 2.2 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| Rule Evaluation | <0.1ms/player | <0.5ms |
| Zone Transition | <10ms | <50ms |
| Memory per Zone | <1KB | <5KB |

---

## 3. Current Gap

**Gap:** Single zone type with hardcoded behavior. No zone rule engine exists. Required for PRD-009 multi-zone demo.

**Location:** src/server/include/zones/ (new files needed)

---

## 4. Architecture

### 4.1 Zone Rule Data Model

```
ZoneDefinition (enhanced):
  - base: existing ZoneDefinition
  - rules: ZoneRuleSet
  - spawnConfig: SpawnConfig
  - portalConfig: PortalConfig
  
ZoneRuleSet:
  - type: enum (PvE, PvP, Tutorial, Arena, Raid)
  - pvpEnabled: bool
  - friendlyFire: bool
  - respawnTimer: Duration
  - minLevel: uint8
  - maxLevel: uint8
  - allowedAbilities: vector<AbilityID>
  - spawnWaves: vector<SpawnWave>
  - bossPhaseConfig: BossPhaseConfig
  - teamConfig: TeamConfig

SpawnWave:
  - waveNumber: uint8
  - enemies: vector<SpawnEntry>
  - spawnDelay: Duration
  - activationCondition: Condition

BossPhaseConfig:
  - phases: vector<BossPhase>
  - transitionRules: vector<PhaseTransition>
  
TeamConfig:
  - teamCount: uint8
  - teamSize: uint8
  - scoreTarget: uint32
```

### 4.2 Zone Rule Engine

```
ZoneRuleEngine:
  - EvaluateDamage(damage, attacker, target, zone) -> modifiedDamage
  - EvaluateDeath(entity, zone) -> RespawnResult
  - EvaluateInput(entity, input, zone) -> bool
  - GetZoneRules(zoneID) -> ZoneRuleSet*
  - EvaluateSpawn(spawner, wave, zone) -> SpawnDecision
  
ZoneTransitionValidator:
  - CanTransition(player, sourceZone, targetZone) -> bool
  - GetTransitionCost(player, targetZone) -> Cost
  - GetPortalDestination(player, portal) -> ZoneID
  
ZoneSpawnManager:
  - TriggerWave(zone, waveNumber): void
  - CheckWaveCompletion(zone) -> bool
  - GetActiveEnemies(zone) -> vector<Entity>
```

### 4.3 Integration Points

```
CombatSystem (modified):
  - PreApplyDamage(damage, attacker, target): modifiedDamage
  - PreEvaluateDeath(entity): RespawnResult
  
PlayerSystem (modified):
  - CanEnterZone(player, zone): bool
  - OnZoneTransition(player, from, to): void
  
NetworkProtocol (enhanced):
  - ZoneChange: zone_id, rules snapshot
  - PvPStateChange: enabled/disabled
  - ScoreUpdate: team, score
```

---

## 5. Implementation Phases

### Phase 1: Zone Rule Data Model
- ZoneRuleSet struct
- ZoneDefinition enhancement
- Zone configuration loading
- Tutorial rules

### Phase 2: Rule Evaluation Hooks
- Combat system integration
- Damage modification
- Death/respawn handling
- PvP toggle enforcement

### Phase 3: Zone Transition Logic
- Portal system integration
- Level requirement checking
- Transition animations
- Client synchronization

### Phase 4: Demo Zone Configurations
- Tutorial zone (zone 1) config
- Arena zone (zone 2) config
- Boss zone (zone 3) config

### Phase 5: Spawn System
- Wave-based spawning
- Boss phase transitions
- Arena team management

---

## 6. Deliverables

### Server Files
- `src/server/include/zones/ZoneRuleSet.hpp`
- `src/server/include/zones/ZoneRuleEngine.hpp`
- `src/server/include/zones/SpawnConfig.hpp`
- `src/server/src/zones/ZoneRuleSet.cpp`
- `src/server/src/zones/ZoneRuleEngine.cpp`
- `src/server/src/zones/SpawnConfig.cpp`

### Zone Configurations
- `src/server/zones/configs/tutorial.json` (zone 1)
- `src/server/zones/configs/arena.json` (zone 2)
- `src/server/zones/configs/boss.json` (zone 3)

### Integration
- `src/server/src/combat/CombatSystem.cpp` (modifications)
- `src/server/src/systems/PlayerSystem.cpp` (modifications)

### Tests
- `src/server/tests/TestZoneRuleEngine.cpp` (new)

---

## 7. Testing

| Test | Location | Criteria |
|------|----------|----------|
| PvP disabled | Unit | No damage in tutorial |
| PvP enabled | Unit | Full damage in arena |
| Safe area | Integration | No player damage |
| Zone transition | Integration | <10ms |
| Spawn waves | Integration | Correct timing |

---

## 8. Acceptance Criteria

- [ ] PvP disabled zones prevent player damage
- [ ] Tutorial zone is safe area (no PvP)
- [ ] Arena zone enforces team rules
- [ ] Boss zone has phase mechanics
- [ ] Zone transitions work correctly
- [ ] Demo zones (tutorial, arena, boss) configured
- [ ] No test regressions
- [ ] Build passes: cmake --build build -j$(nproc)

---

*Last Updated: 2026-05-01*