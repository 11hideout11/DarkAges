# PRD-012: Zone Rule Engine

**Version:** 1.0  
**Status:** 🔴 Not Started  
**Owner:** SERVER_AGENT  
**Priority:** HIGH  
**Dependencies:** PRD-009 (Demo Zones), PRD-004 (Sharding)

---

## 1. Overview

### 1.1 Purpose
Implement a zone rule engine that applies zone-specific rules (PvP, PvE, Tutorial, Raid) and enables the multi-zone demo system defined in PRD-009. Each zone has configurable rules that govern combat, spawning, and player interactions.

### 1.2 Scope
- Zone rule data model (ZoneRuleSet)
- Rule evaluation hooks in combat system
- Zone transition validation
- Demo zone configurations (Tutorial, Arena, Boss)

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| ZRE-001 | Zone rules: PvP enabled/disabled | P0 | PvP zone flag |
| ZRE-002 | Zone rules: Tutorial (safe area) | P0 | No PvP, no enemy NPCs |
| ZRE-003 | Zone rules: Arena (team-based) | P1 | Team colors, scoring |
| ZRE-004 | Zone rules: Raid (boss mechanics) | P1 | Phase transitions |
| ZRE-005 | Zone rule validation hook | P0 | Combat system integration |
| ZRE-006 | Zone transition rules | P1 | Level requirements |
| ZRE-007 | Spawn wave configuration | P1 | Timed NPC spawns |
| ZRE-008 | Zone-specific NPC loadouts | P0 | Different abilities per zone |

### 2.2 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| Rule Evaluation | <0.1ms/player | <0.5ms |
| Zone Transition | <10ms | <50ms |
| Configuration Load | <50ms | <200ms |

---

## 3. Current Gap

**Gap:** Single zone type with hardcoded behavior. No zone rule engine exists. Required for PRD-009 multi-zone demo.

**Location:** src/server/zones/ (single implementation)

---

## 4. Implementation Strategy

### 4.1 Zone Rule Data Model

```
ZoneDefinition:
  - base: existing ZoneDefinition
  - rules: ZoneRuleSet
  - spawnConfig: SpawnConfig
  
ZoneRuleSet:
  - zoneType: enum (Tutorial, PvE, PvP, Arena, Raid)
  - pvpEnabled: bool
  - friendlyFire: bool
  - respawnTimer: Duration
  - minLevel: uint8
  - maxPlayers: uint8
  - allowedAbilities: vector<AbilityID>
  - restrictedAbilities: vector<AbilityID>
  - weather: WeatherType
  - timeOfDay: TimeOfDay
  
SpawnConfig:
  - spawnWaves: vector<SpawnWave>
  - spawnTriggers: vector<SpawnTrigger>
  
SpawnWave:
  - waveId: uint8
  - npcs: vector<NpcSpawn>
  - delay_ms: uint32
  - repeatCount: uint8
  
NpcSpawn:
  - npcId: uint32
  - count: uint8
  - formation: FormationType
```

### 4.2 Zone Rule Engine

```
ZoneRuleEngine:
  - EvaluateDamage(damage, attacker, target, zone) -> modifiedDamage
  - EvaluateDeath(entity, zone) -> RespawnResult
  - EvaluateInput(entity, input, zone) -> bool
  - GetZoneRules(zoneID) -> ZoneRuleSet
  - LoadZoneConfig(zoneID) -> ZoneDefinition
  
ZoneTransitionValidator:
  - CanTransition(player, targetZone) -> bool
  - GetTransitionCost(player, targetZone) -> Cost
  - FindNearestPortal(player) -> Portal
```

### 4.3 Combat System Integration

Each combat input passes through zone rule validation:

```
CombatInputHandler::ProcessAbilityUse():
  1. Get player zone
  2. Get zone rules via ZoneRuleEngine
  3. Check PVP-enabled for PvP abilities
  4. Check ability restrictions
  5. Proceed if allowed, reject if not
```

---

## 5. Deliverables

### 5.1 Server Files
- `src/server/include/zones/ZoneRuleSet.hpp`
- `src/server/include/zones/ZoneRuleEngine.hpp`
- `src/server/src/zones/ZoneRuleSet.cpp`
- `src/server/src/zones/ZoneRuleEngine.cpp`

### 5.2 Configuration Files
- `data/zones/configs/tutorial.json` (zone 1)
- `data/zones/configs/arena.json` (zone 2)
- `data/zones/configs/boss.json` (zone 3)

### 5.3 Test Files
- `src/server/tests/TestZoneRuleEngine.cpp` (new)

---

## 6. Testing

| Test | Location | Criteria |
|------|----------|--------|
| PvP disable | TestZoneRuleEngine | No damage in safe zones |
| Tutorial zone | Integration | No enemies, no PvP |
| Arena rules | Integration | Team damage disabled |
| Boss phases | Integration | Phase transitions work |
| Zone transition | Integration | Portal teleports correctly |

---

## 7. Acceptance Criteria

- [ ] PvP disabled zones prevent player damage
- [ ] Tutorial zone is safe area (no PvP, no enemy NPCs)
- [ ] Arena zone enforces team rules
- [ ] Boss zone has phase mechanics
- [ ] Zone transitions work correctly
- [ ] Spawn waves activate on timer
- [ ] No test regressions

---

*Last Updated: 2026-05-01*