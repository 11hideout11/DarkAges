# PRD-010: Global Cooldown System

**Version:** 1.0  
**Status:** 🟡 Planned  
**Owner:** SERVER_AGENT  
**Priority:** CRITICAL  
**Dependencies:** PRD-003 (Combat System)

---

## 1. Overview

### 1.1 Purpose
Implement a server-authoritative global cooldown (GCD) system that prevents ability spam and enforces combat pacing. The GCD ensures a minimum 1.2s delay between ability uses, synchronized between server and client.

### 1.2 Scope
- Server-side cooldown tracking per player entity
- Client-side prediction and visual feedback
- Network synchronization and reconnection recovery
- Integration with existing combat system

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| GCD-001 | Server maintains GCD timer per player (1.2s duration) | P0 | Timestamp-based tracking |
| GCD-002 | Server rejects ability use during GCD | P0 | CombatInputHandler validation |
| GCD-003 | Client predicts GCD for responsive UI | P0 | Local timer for UI feedback |
| GCD-004 | GCD state sync on reconnection | P1 | Snapshot delta sync |
| GCD-005 | GCD visualization on client | P2 | Cooldown indicator |

### 2.2 Non-Functional Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| GCD Check Latency | <1ms | <2ms |
| Network Overhead | <100 bytes/player | <200 bytes |
| Memory Overhead | <1KB/player | <2KB |

---

## 3. Current Gap

**Gap:** No formal cooldown system exists. Search for "cooldown", "GlobalCooldown", "CanAttack" returns zero results in codebase.

**Location:** src/server/include/combat/ (new files needed)

---

## 4. Architecture

### 4.1 Server Architecture

```
PlayerComponent (enhanced):
  - lastAbilityTime: Timestamp (ms)
  - cooldownMap: unordered_map<AbilityID, Timestamp>
  
CombatInputHandler (modified):
  - ValidateGCD(entity, abilityID) -> bool
  - ApplyGCD(entity, abilityID) -> void
  - GetRemainingCooldown(entity, abilityID) -> Duration
```

### 4.2 Client Architecture

```
AbilityBar (modified):
  - localGCDTimer: Timer
  - PredictGCD(abilityID): void
  - RollbackGCD(abilityID): void
  
CooldownIndicator (new):
  - Visual cooldown overlay
  - Animated progress indicator
```

### 4.3 Network Protocol

```
CooldownSync (proto):
  - player_id: uint64
  - ability_id: uint32
  - cooldown_end_ms: uint64
  - is_global: bool
```

---

## 5. Implementation Phases

### Phase 1: Server-Side GCD
- CooldownComponent with timestamp tracking
- CooldownSystem with validation logic
- Integration with CombatInputHandler

### Phase 2: Client Integration
- AbilityBar modifications for local GCD
- CooldownIndicator UI component
- Prediction and rollback logic

### Phase 3: Network Sync
- Snapshot delta sync for GCD state
- Reconnection recovery
- Lag compensation

---

## 6. Deliverables

### Server Files
- `src/server/include/combat/CooldownComponent.hpp`
- `src/server/include/combat/CooldownSystem.hpp`
- `src/server/src/combat/CooldownComponent.cpp`
- `src/server/src/combat/CooldownSystem.cpp`

### Client Files
- `src/client/src/combat/CooldownUI.cs` (new)
- `src/client/src/combat/AbilityBar.cs` (modifications)

### Protocol
- `src/shared/proto/combat.proto` (updates)

### Tests
- `src/server/tests/TestCooldownSystem.cpp` (new)

---

## 7. Testing

| Test | Location | Criteria |
|------|----------|----------|
| GCD rejection | Unit | Server rejects within 1ms |
| GCD prediction | Integration | Client predicts correctly |
| GCD sync | Network | State syncs within 50ms |
| Performance | Benchmark | <1ms tick |

---

## 8. Acceptance Criteria

- [ ] Server rejects ability use during 1.2s GCD window
- [ ] Client shows GCD indicator (visual feedback)
- [ ] Server rejects within 1ms tick budget
- [ ] Network sync for reconnection works
- [ ] No test regressions (baseline: 2,097 cases, 12,644 assertions)
- [ ] Build passes: cmake --build build -j$(nproc)

---

*Last Updated: 2026-05-01*