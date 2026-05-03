# PRD-106: Abilities Talents System

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** Proposed  
**Priority:** High  
**Prerequisite:** Basic melee attacks exist

---

## 1. Problem Statement

Only basic melee attacks are implemented. There is no abilities framework, talent trees, or skill variety. Players have no character progression options beyond basic attacks.

### Current State
- ✅ Basic melee attacks work
- ⏸️ Abilities framework needed
- ⏸️ Talent trees not implemented

### Impact
- No skill variety
- No progression depth
- No class differentiation

---

## 2. Goals

### Primary Goals
1. Create abilities framework
2. Implement 3 talent trees (10 points each)
3. Add 20+ abilities
4. Server-side validation
5. Ability cooldowns

### Success Criteria
- [ ] Abilities framework exists
- [ ] 20 abilities work
- [ ] Talent trees function
- [ ] Server validates all ability use
- [ ] 2 new abilities per sprint

---

## 3. Technical Specification

### Ability Types

| Type | Activation | Cost | Examples |
|------|------------|------|----------|
| ACTIVE_INSTANT | Instant | Mana | Bash, Heal |
| ACTIVE_CHANNEL | Channeled | Mana/channel | Channel Fireball |
| ACTIVE_TARGET | Target | Mana | Lightning Bolt |
| PASSIVE | Always on | None | Damage Aura |
| TOGGLE | Toggle on/off | Mana/sec | Shield |

### Ability Schema

```csharp
public struct AbilityData {
    public uint32_t Id;
    public string Name;
    public string Description;
    public AbilityType Type;
    public uint32_t CooldownMs;
    public int ManaCost;
    public int Range;
    public TargetType Target;
    public string Icon;
    public AnimationId Animation;
}

// Ability Component
public class AbilityComponent : Component {
    private const int MAX_ABILITIES = 30;
    private AbilityData[] unlockedAbilities;
    private Dictionary<uint32_t, long> cooldowns;  // timestamp
    
    public bool CanUseAbility(uint32_t abilityId);
    public bool UseAbility(uint32_t abilityId, Vector3 target);
    public void StartCooldown(uint32_t abilityId);
    public float GetCooldownRemaining(uint32_t abilityId);
}
```

### Talent Trees

```
Talent Trees:
┌────────────────────────────────────────────────────┐
│ Warrior (Str)          │ Mage (Int)          │
│ [Strike]W[Wound]      │ [Fireball]─[Burn]   │
│ [Bash]──[Stun]        │ [Icebolt]─[Freeze]  │
│ [Sunder]─[Bleed]     │ [Telekin]─[Push]   │
│                       │                     │
│ Rogue (Agi)           │ Hybrid             │
│ [Backstab]─[Crit]    │ [Smite]─[Holy]     │
│ [Stealth]─[Vanish]    │ [Heal]─[Shield]    │
│ [Poison]─[Death]     │ [Buff]─[Debuff]    │
└────────────────────────────────────────────────────┘
```

### Talent Points

```csharp
public class TalentTree {
    public string TreeName;  // "warrior", "mage", "rogue"
    public int MaxPoints = 10;
    public int CurrentPoints { get; set; }
    public List<TalentNode> Nodes;
}

public struct TalentNode {
    public uint32_t Id;
    public string Name;
    public int Rank;  // 0-3
    public int RequiredLevel;
    public int PrerequisiteId;
    public List<AbilityId> Unlocks;
    public string Description;
}
```

### Integration

| Component | Integration | Notes |
|-----------|------------|-------|
| AbilityComponent | Entity component | Add to Player |
| CombatSystem | Apply ability effects | Validate server-side |
| UIAbilities | Hotbar display | Show cooldowns |
| SaveSystem | Persist talents | Save/load |

---

## 4. Implementation Plan

### Week 1: Framework

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Define ability schema | Data structures |
| 3-4 | Create AbilityComponent | Component works |
| 5-6 | Implement 5 basic abilities | Test framework |
| 7 | Server validation | Security |

### Week 2: Abilities

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Implement 10 more abilities | 15 total |
| 10-11 | Add cooldowns/mana | Resources work |
| 12-13 | Add to UI | Hotbar works |
| 14 | Test all abilities | Integration |

### Week 3: Talents

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Create talent trees | 3 trees |
| 18-19 | Unlock system | Point allocation |
| 20-21 | Save/load | Persistence |
| 22-24 | Final polish | Tests |

### Dependencies
- Player.tscn (exists)
- CombatStateMachineController.cs (after PRD-018)

---

## 5. Testing Requirements

### Unit Tests
- Ability instantite
- Cooldown management
- Talent point validation

### Integration Tests
- 20 abilities work
- Talent trees unlock abilities

### Test Metrics
- Abilities: 20+
- Test coverage: >80%

---

## 6. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 4 weeks |
| LOC | ~500 |
| Skills | C#, Game Design, Godot |

---

## 7. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Balance issues | High | Medium | Tuning pass |
| Complexity | High | Medium | Start simple |

---

## 8. Open Questions

1. **Q: Reset talents?**
   - A: Gold cost for reset

2. **Q: Shared trees?**
   - A: No - one tree primary for MVP

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Define ability schema