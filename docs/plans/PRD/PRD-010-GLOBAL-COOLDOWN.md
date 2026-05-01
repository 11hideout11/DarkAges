# PRD-010: Global Cooldown System

**Version:** 1.0  
**Status:** 🔴 Not Started  
**Owner:** SERVER_AGENT  
**Priority:** CRITICAL  
**Dependencies:** PRD-003 (Combat System)

---

## 1. Overview

### 1.1 Purpose
Implement a server-authoritative global cooldown (GCD) system that prevents ability spam and enforces combat pacing. The GCD ensures a minimum 1.2s delay between ability uses, synchronized between server and client.

### 1.2 Scope
- Server-side cooldown tracking per player
- Client-side cooldown prediction for responsive UI
- Network synchronization of cooldown state
- Reconnection state recovery

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| GCD-001 | Server maintains GCD timer per player (1.2s duration) | P0 | Timestamp-based tracking |
| GCD-002 | Server rejects ability use during GCD | P0 | Server authority |
| GCD-003 | Client predicts GCD for responsive UI | P0 | Visual feedback |
| GCD-004 | GCD state sync on reconnection | P1 | Snapshot recovery |
| GCD-005 | GCD visualization on client | P2 | Cooldown UI indicator |
| GCD-006 | Individual ability cooldowns | P0 | Per-ability cooldowns |

### 2.2 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| GCD Check Latency | <1ms | <2ms |
| Network Overhead | <100 bytes/player | <200 bytes |

---

## 3. Current Gap

**Gap:** No formal cooldown system exists. Search for "cooldown", "GlobalCooldown", "CanAttack" returns zero results in codebase.

**Location:** src/server/include/combat/ (empty)

---

## 4. Implementation Strategy

### 4.1 Server Architecture

```
PlayerComponent:
  - lastAbilityTime: Timestamp (ms)
  - cooldownMap: unordered_map<AbilityID, Timestamp>
  
CombatInputHandler:
  - ValidateGCD(entity, abilityID) -> bool
  - ApplyCooldown(entity, abilityID, duration) -> void
  - GetRemainingCooldown(entity, abilityID) -> Duration
```

### 4.2 Client Architecture

```
AbilityBar:
  - localCooldownTimers: Dictionary<AbilityID, Timer>
  - PredictCooldown(abilityID, duration): void
  - RollbackCooldown(abilityID): void (on server reject)
  - UpdateVisualization(): void
```

### 4.3 Network Protocol

- Client → Server: ability_use (abilityID, current_time)
- Server → Client: ability_use_response (success, remaining_cooldown)
- Server → Client: cooldown_sync (cooldown_map snapshot)

---

## 5. Deliverables

### 5.1 Server Files
- `src/server/include/combat/CooldownComponent.hpp`
- `src/server/include/combat/CooldownSystem.hpp`
- `src/server/src/combat/CooldownComponent.cpp`
- `src/server/src/combat/CooldownSystem.cpp`

### 5.2 Client Files
- `src/client/src/combat/CooldownUI.cs` (new)
- `src/client/src/combat/AbilityBar.cs` (modifications)

### 5.3 Protocol Files
- `src/shared/proto/combat.proto` (updates)

### 5.4 Test Files
- `src/server/tests/TestCooldownSystem.cpp` (new)

---

## 6. Testing

| Test | Location | Criteria |
|------|----------|--------|
| GCD rejection | TestCooldownSystem | Server rejects during 1.2s window |
| Client prediction | Integration | UI updates immediately |
| Reconnection sync | Integration | Cooldown state restored |
| Performance | Benchmark | <1ms check latency |

---

## 7. Acceptance Criteria

- [ ] Server rejects ability use during 1.2s GCD window
- [ ] Client shows cooldown indicator (visual feedback)
- [ ] Server rejects within 1ms tick budget
- [ ] Individual ability cooldowns work (e.g., 10s for ultimate)
- [ ] No test regressions (baseline: 2,097 cases, 12,644 assertions)

---

*Last Updated: 2026-05-01*