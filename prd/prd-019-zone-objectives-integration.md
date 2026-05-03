# PRD-019: Zone Objectives System Integration

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** Proposed  
**Priority:** High  
**Prerequisite:** ZoneObjectiveComponent.hpp and ZoneObjectiveSystem.cpp exist

---

## 1. Problem Statement

Zone objectives have been implemented on the server side:
- ✅ ZoneObjectiveComponent.hpp - Server tracking component
- ✅ ZoneObjectiveSystem.cpp - Objective tracking logic
- ✅ TestZoneObjectives.cpp - Unit tests
- ✅ Zone configs enriched with objectives

However, the system is **not integrated** with the server tick loop for continuous updates.

### Current State
- ✅ ZoneObjectiveComponent exists
- ✅ ZoneObjectiveSystem logic exists
- ✅ Unit tests pass
- ⚠️ Not wired to tick loop

### Impact
- Objectives don't update during gameplay
- Players cannot see progress
- Zone completion not detected
- No reward distribution

---

## 2. Goals

### Primary Goals
1. Integrate ZoneObjectiveSystem with server tick loop
2. Track player progress in real-time
3. Detect zone completion
4. Implement reward distribution

### Success Criteria
- [ ] Objectives update per tick
- [ ] Player progress tracked
- [ ] Zone completion detected
- [ ] Rewards distributed

---

## 3. Technical Specification

### Architecture

```
Server Tick Loop:
┌─────────────────────────────────────────────────────────┐
│                    ZoneSystem                         │
│  ┌──────────────────────────────────────────────┐    │
│  │ ZoneObjectiveSystem::Update(delta_time)      │    │
│  │  - For each zone:                         │    │
│  │    - For each objective:                 │    │
│  │      - Check progress                   │    │
│  │      - Update completion status          │    │
│  │      - Handle completion                 │    │
│  └──────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

### ZoneObjectiveSystem Interface

```cpp
// Header: include/zones/ZoneObjectiveSystem.hpp
namespace DarkAges::zones {

struct ObjectiveProgress {
    EntityID player_id;
    uint32_t objective_id;
    float progress;      // 0.0 - 1.0
    bool completed;
    uint64_t timestamp;
};

class ZoneObjectiveSystem {
public:
    // Initialize with zone configs
    void Initialize(const std::vector<ZoneConfig>& zones);
    
    // Per-tick update
    void Update(float delta_time);
    
    // Player interactions
    void OnEnemyKilled(EntityID player, EntityID enemy);
    void OnItemCollected(EntityID player, ItemID item);
    void OnLocationReached(EntityID player, Vector3 position);
    
    // Query
    std::vector<ObjectiveProgress> GetPlayerProgress(EntityID player);
    bool IsZoneComplete(ZoneID zone);
    
    // Events
    std::function<void(EntityID, uint32_t, const std::vector<ItemReward>&)> OnObjectiveComplete;
    
private:
    std::unordered_map<ZoneID, ZoneObjectives> zones_;
};

} // namespace DarkAges::zones
```

### Integration Points

| Component | Integration | File |
|-----------|------------|------|
| ServerTickSystem | Call ZoneObjectiveSystem::Update | ServerTickSystem.cpp |
| EnemyDeathSystem | Notify ZoneObjectiveSystem | EnemyDeathSystem.cpp |
| ItemCollection | Notify ZoneObjectiveSystem | ItemSystem.cpp |
| SnapshotSystem | Include objective state | SnapshotSystem.cpp |

### Objective Types

| Type | Update Check | Trigger |
|------|-------------|---------|
| KillXEnemies | counter++ | OnEnemyKilled |
| CollectXItems | counter++ | OnItemCollected |
| ReachLocation | distance() < threshold | OnLocationReached |
| SurviveXSeconds | timer += dt | Update loop |
| DealXDamage | accumulator | OnDamageDealt |

---

## 4. Implementation Plan

### Week 1: Core Integration

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Wire ZoneObjectiveSystem to tick | System updates |
| 3-4 | Implement KillXEnemies tracking | Kill tracking works |
| 5-6 | Implement CollectXItems tracking | Item tracking works |
| 7 | Implement Location tracking | Location tracking |

### Week 2: Completion & Rewards

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Detect zone completion | Completion triggers |
| 10 | Implement reward distribution | Rewards work |
| 11-12 | Add to snapshot system | State synced |
| 14 | Test full flow | Integration passes |

### Dependencies
- ZoneObjectiveComponent.hpp (exists)
- ZoneObjectiveSystem.cpp (exists)
- TestZoneObjectives.cpp (exists, tests pass)

---

## 5. Testing Requirements

### Unit Tests
- All existing TestZoneObjectives pass
- Objective type tests

### Integration Tests
- Full objective flow
- Zone completion detection
- Reward distribution

### Test Metrics
- Test coverage: >80%
- Tick overhead: <0.5ms per zone

---

## 6. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2 weeks |
| LOC | ~300 |
| Skills | C++, EnTT ECS, Game design |

---

## 7. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Performance | Medium | High | Profile per tick |
| Edge cases | Medium | Low | Thorough tests |

---

## 8. Open Questions

1. **Q: Save objective progress?**
   - A: Yes, to database on completion

2. **Q: Party objective sharing?**
   - A: Deferred - solo for MVP

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Begin tick loop integration