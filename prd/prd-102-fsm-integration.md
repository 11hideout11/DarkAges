# PRD-019: Node-Based FSM Integration

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** ✅ COMPLETE (2026-05-02)  
**Priority:** High  
**Prerequisite:** CombatStateMachine.tscn and CombatStateMachineController.cs exist

---

## 1. Problem Statement

The Node-Based Combat FSM template has been created:
- ✅ `CombatStateMachine.tscn` - AnimationTree scene
- ✅ `CombatStateMachineController.cs` - Controller script

### Current State (2026-05-02)
- ✅ CombatStateMachine.tscn exists
- ✅ CombatStateMachineController.cs exists (280+ lines)
- ✅ **Player.tscn has CombatStateMachine node (line 80-81)**
- ✅ **RemotePlayer.tscn has CombatStateMachine node**
- ✅ PRD COMPLETE

### Impact
- Combat animations don't play
- Input not routed through FSM
- No state transition callbacks
- Cannot test combat flow

---

## 2. Goals

### Primary Goals
1. Wire FSM to Player.tscn
2. Wire FSM to RemotePlayer.tscn
3. Route player input through CombatStateMachineController
4. Enable animation state callbacks

### Success Criteria
- [ ] FSM integrated with Player.tscn
- [ ] FSM integrated with RemotePlayer.tscn  
- [ ] Combat animations play during attacks
- [ ] State transitions visible
- [ ] All existing tests pass

---

## 3. Technical Specification

### Player.tscn Integration

```
Player.tscn Structure:
├── AnimationPlayer (for idle/attack animations)
├── AnimationTree (CombatStateMachine)
├── CharacterBody3D
│   └── CombatStateMachineController.gd  ← Add this
├── HealthComponent
├── HitboxComponent
└── MovementComponent
```

### Integration Points

| Component | Integration | File |
|-----------|------------|------|
| Player.tscn | Add CombatStateMachineController node | scenes/player/Player.tscn |
| RemotePlayer.tscn | Add CombatStateMachineController node | scenes/player/RemotePlayer.tscn |
| InputManager | Route combat input to FSM | scripts/player/InputManager.cs |
| AnimationPlayer | Connect to AnimationTree | scripts/combat/CombatStateMachineController.cs |
| HealthComponent | Connect damage callbacks | scripts/player/Player.cs |

### CombatStateMachineController Interface

```csharp
// In: scripts/combat/CombatStateMachineController.cs
public class CombatStateMachineController : Node
{
    [Export] public AnimationTree animationTree;
    [Export] public AnimationPlayer animationPlayer;
    
    // State Machine
    public string CurrentState { get; private set; }
    public string PreviousState { get; private set; }
    
    // Input from player
    public void RequestAttack() => _stateMachine.Set("attack");
    public void RequestDodge() => _stateMachine.Set("dodge");
    public void RequestHit() => _stateMachine.Set("hit");
    public void RequestDeath() => _stateMachine.Set("death");
    
    // Callbacks from AnimationTree
    public event Action<string> OnStateEntered;
    public event Action<string> OnStateExited;
    public event Action<string> OnStateChanged;
    
    // Animation event callbacks ( AnimationPlayer signals)
    public void OnAnimationFinish(string anim_name);
    public void OnAnimationFrameReached(int frame);
}
```

### Connection Flow

```
Input → Player.InputAttack() 
     → CombatStateMachineController.RequestAttack()
     → AnimationTree.Set("attack")
     → AnimationPlayer plays attack anim
     → OnAnimationFinish("attack")
     → Returns to idle
```

---

## 4. Implementation Plan

### Week 1: Player Integration

| Day | Task | Deliverable |
|-----|------|-------------|
| 1 | Add FSM node to Player.tscn | Node tree exists |
| 2 | Wire AttackInput to FSM | Input routes |
| 3 | Connect AnimationTree | Animations play |
| 4 | Handle state callbacks | Transitions work |
| 5 | Test local player | Basic combat works |

### Week 2: RemotePlayer & Polish

| Day | Task | Deliverable |
|-----|------|-------------|
| 6 | Add FSM to RemotePlayer.tscn | Remote has FSM |
| 7 | Sync state via network | Network sync works |
| 8 | Handle hit/death states | Combat complete |
| 9 | Edge case handling | Error cases work |
| 10 | Test full combat flow | Integration tests pass |

### Dependencies
- CombatStateMachine.tscn (exists)
- CombatStateMachineController.cs (exists)
- Player.tscn (exists)
- InputManager (exists)

---

## 5. Testing Requirements

### Integration Tests
- Player attack animation plays
- RemotePlayer attack synced
- State transitions work
- Hit/death flow

### Existing Tests
- All test_combat tests pass
- All unit_tests pass

### Test Metrics
- FSM coverage: >90%
- Animation sync: 100%
- State machine errors: 0

---

## 6. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2 weeks |
| LOC | ~200 |
| Skills | C#, Godot 4.2, AnimationTree |

---

## 7. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| AnimationTree complexity | Medium | Medium | Use existing controller |
| Network sync issues | Medium | High | Test incrementally |
| Timing issues | High | Low | Debug logging |

---

## 8. Open Questions

1. **Q: Use server or client authority for FSM?**
   - A: Server authoritative - client sends input, server runs FSM

2. **Q: Blend space for movement+combat?**
   - A: Deferred - use separate states for now

3. **Q: Full FSM or simplified?**
   - A: Full for future extensibility, start with 5 states

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Begin Player.tscn integration