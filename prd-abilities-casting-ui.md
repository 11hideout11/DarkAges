# PRD: Abilities UI & Casting System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Category:** Gameplay - Player Progression

---

## 1. Problem Statement

The server has ability data (data/abilities.json) and AbilitySystem.hpp, but there's no client-side UI to display abilities or trigger casting. Players cannot cast spells or use skills.

### Current State
- ⚠️ Ability definitions exist in data/
- ⚠️ Server AbilitySystem implemented
- ⚠️ No client ability UI
- ⚠️ No casting input handling

### Impact
- Players cannot use abilities
- Combat limited to auto-attacks
- No skill expression
- Missing core combat loop

---

## 2. Goals

### Primary Goals
1. Create ability icons and artwork
2. Build ability hotbar integration
3. Implement cast input handling
4. Add cast feedback UI (casting bar, cooldown)

### Success Criteria
- [ ] Abilities display in hotbar
- [ ] Click or key press triggers cast
- [ ] Cast bar shows channeling
- [ ] Cooldown shows remaining time
- [ ] Target validation works

---

## 3. Technical Specification

### Ability Data (Existing)

```json
{
  "abilities": [
    {
      "id": 1,
      "name": "Fireball",
      "type": "damage",
      "castTime": 1500,
      "cooldown": 3000,
      "manaCost": 15,
      "range": 15,
      "effectValue": 45,
      "icon": "res://assets/abilities/fireball.png"
    }
  ]
}
```

### Client Components

```csharp
// AbilitySlot.cs (Hotbar slot)
public partial class AbilitySlot : TextureRect {
    [Export] private int slotIndex;
    [Export] private Texture2D icon;
    [Export] private TextureProgressBar cooldownOverlay;
    [Export] private Label hotkeyLabel;
    
    public void SetAbility(AbilityData ability);
    public void SetCooldown(float remaining, float total);
    public void Clear();
    public event Action<int> OnSlotActivated;
}

// AbilityCastingUI.cs
public partial class AbilityCastingUI : Control {
    [Export] private ProgressBar castBar;
    [Export] private Label abilityName;
    [Export] private Label castTime;
    
    public void ShowCasting(AbilityData ability, float castTime);
    public void UpdateProgress(float progress);
    public void Hide();
}

// CastTargeting.cs
public partial class CastTargeting : Node3D {
    public enum TargetingMode { None, Self, Enemy, Ally, Ground }
    
    private TargetingMode mode;
    private float range;
    private Area3D targetArea;
    
    public void BeginTargeting(AbilityData ability);
    public void CancelTargeting();
    public Entity GetTarget();
}
```

### Input Handling

```csharp
// InputManager.cs additions
private void HandleAbilityInput(int slotIndex) {
    var ability = hotbar.GetAbility(slotIndex);
    if (ability == null) return;
    
    if (ability.targetingMode == TargetingMode.Self) {
        CastAbility(ability, player.Entity);
    } else {
        BeginTargeting(ability);
    }
}

private void HandleMouseClick() {
    if (targetingActive) {
        var target = GetTargetAtMouse();
        if (ValidateTarget(target)) {
            CastAbility(currentAbility, target);
        }
    }
}
```

### Integration Points

| Component | Integration | File |
|-----------|------------|------|
| Hotbar | Display ability icons | Hotbar.cs |
| CombatSystem | Send cast request to server | NetworkManager.cs |
| PlayerController | Target raycasting | PlayerController.cs |
| GameState | Sync ability cooldowns | GameState.cs |

---

## 4. User Stories

### US-001: Ability Display
**Description:** As a player, I want to see my abilities on the hotbar so that I know what I can cast.

**Acceptance Criteria:**
- [ ] Each hotbar slot shows ability icon
- [ ] Empty slots show placeholder
- [ ] Tooltip shows ability details on hover

### US-002: Cast Trigger
**Description:** As a player, I want to cast an ability by clicking its hotbar slot so that I can use skills.

**Acceptance Criteria:**
- [ ] Click hotbar slot triggers cast
- [ ] Press corresponding number key triggers
- [ ] Cannot cast during global cooldown
- [ ] Cannot cast without sufficient mana

### US-003: Self-Target Cast
**Description:** As a player, I want to cast self-targeting abilities instantly so that buffs work quickly.

**Acceptance Criteria:**
- [ ] Buff abilities cast immediately on key press
- [ ] No targeting reticle shown
- [ ] Cast success feedback displayed

### US-004: Targeted Cast
**Description:** As a player, I want to target enemy abilities so that I can hit specific enemies.

**Acceptance Criteria:**
- [ ] Targeting reticle appears on mouse
- [ ] Valid targets highlight
- [ ] Invalid targets show error
- [ ] Click executes cast

### US-005: Cast Feedback
**Description:** As a player, I want to see casting feedback so that I know my action is processing.

**Acceptance Criteria:**
- [ ] Cast bar shows channeling progress
- [ ] Cooldown overlay appears after cast
- [ ] Error messages for failed casts

---

## 5. Functional Requirements

- FR-1: Load ability icons from assets/abilities/
- FR-2: Display 10 abilities in hotbar
- FR-3: Number keys 1-0 trigger corresponding slots
- FR-4: Mouse click triggers slot under cursor
- FR-5: Self-target abilities cast instantly
- FR-6: Target abilities show reticle until target selected
- FR-7: Show cast bar for abilities >0ms cast time
- FR-8: Show cooldown overlay after ability use
- FR-9: Block input during global cooldown (200ms)
- FR-10: Display "Not enough mana" on insufficient mana

---

## 6. Non-Goals

- No ability points allocation
- No talent trees
- No ability customization
- No macros

---

## 7. Implementation Plan

### Week 1: Visual Assets

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-3 | Create placeholder ability icons | 22 PNGs |
| 4-5 | Create targeting reticle | Sprite works |
| 6-7 | Create cast bar UI | UI works |

### Week 2: Integration

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-10 | Wire ability loading | Abilities in hotbar |
| 11-12 | Implement input handling | Input works |
| 13-14 | Add targeting logic | Targeting works |

### Week 3: Feedback

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Cast bar integration | Bar works |
| 18-19 | Cooldown overlay | Overlay works |
| 20-21 | Test and polish | Full flow works |

---

## 8. Testing Requirements

### Unit Tests
- Ability slot state changes
- Cooldown calculations
- Targeting validation

### Integration Tests
- Press hotkey → cast executes
- Click target → cast fires
- Channel complete → effect applies

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2-3 weeks |
| LOC | ~400 (C#) |
| Skills | C#, Godot, art (placeholder OK) |

---

## 10. Open Questions

1. **Q: How many abilities to implement first?**
   - A: 10 for initial (5 damage, 3 buff, 2 heal)

2. **Q: Auto-attack separate from abilities?**
   - A: Yes, auto-attack on left-click, abilities on hotbar

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Create ability icon placeholders