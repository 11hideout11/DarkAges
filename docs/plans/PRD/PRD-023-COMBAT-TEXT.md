# PRD-023: Combat Floating Text Integration — Damage Numbers & Healing Indicators

**Version:** 1.0
**Status:** 🔄 In Progress — Components exist, integration needed
**Owner:** UI_AGENT
**Priority:** MEDIUM (P3 — Player Feedback Quality)
**Dependencies:** PRD-008 (CombatStateMachine FSM), PRD-010 (Hitbox validation)
**Gap Source:** COMPATIBILITY_ANALYSIS.md — "Floating Combat Text: DamageNumber.cs exists but not integrated"

---

## Implementation Status (2026-05-01)

### ✅ Completed - Components Exist
- [x] CombatTextSystem.cs exists (UI system with pooling)
- [x] DamageNumber.cs exists

### 📋 Pending - Integration
- [ ] Connect to AttackFeedbackSystem or CombatSystem
- [ ] Trigger damage display on hit registration
- [ ] Display heal numbers on healing
- [ ] Critical hit styling

---

## 1. Overview

### 1.1 Purpose
Integrate the existing **combat text system** (`CombatTextSystem.cs`, `DamageNumber.cs`) into the combat pipeline so that:
- Damage numbers appear when entity takes damage
- Healing numbers appear when entity is healed
- Critical hits display distinct styling (larger, different color, "CRIT" label)
- Numbers fade out smoothly, track world position

### 1.2 Current State

**Files that exist (unused):**
```
src/client/src/combat/DamageNumber.cs          — DamageNumber node/component (floating text)
src/client/src/ui/CombatTextSystem.cs          — CombatTextSystem manager (spawns numbers)
src/client/scripts/DamageNumber.gd (maybe)     — possible legacy version
```

**What's missing:**
- No calls to `CombatTextSystem.Instance().ShowDamage()` in combat code
- No `DamageNumber` nodes attached to `Player.tscn` or `HUD`
- No combat event hooks in `ServerCombatManager` or client-side `PredictedPlayer`

**Result:** No damage numbers appear during combat (player gets hit but receives no visual feedback).

---

## 2. Requirements

### 2.1 Functional Requirements
ID      | Requirement                           | Priority | Details
--------|---------------------------------------|----------|--------
TEXT-001 | Spawn damage number on hit            | P0       | Call CombatTextSystem when damage dealt
TEXT-002 | Color-coded by damage type            | P0       | Red physical, blue magic, green healing, yellow crit
TEXT-003 | Position follow target               | P0       | Number floats above damaged entity's head
TEXT-004 | Fade out animation                   | P0       | 0.8s fade + upward drift
TEXT-005 | Critical hit indicator               | P1       | Larger font, "CRIT" prefix, golden color
TEXT-006 | Multi-hit queuing                   | P1       | Rapid hits stack vertically
TEXT-007 | Server-authoritative damage source   | P0       | Server sends damage events, client displays

### 2.2 Non-Functional
- Per-hit spawn cost: <0.1ms
- Peak simultaneous numbers: <50 (cleanup old ones)
- Memory: <1MB for pooled DamageNumber nodes
- No network overhead: server sends minimal event (damage amount + type + target ID)

---

## 3. Integration Strategy

### 3.1 Server → Client Damage Event

**Current server→client damage flow:**
```
ServerCombatManager (server) applies damage
  → sends EntityHealthChanged RPC to client (if entity controlled by client)
  → Client applies health delta locally (PredictedPlayer.TakeDamage())
  → No visual feedback event
```

**Needed: Add `CombatTextEvent` to RPC or use existing:**

Option A: Extend `SharedStats` delta to include `displayDamage` flag
Option B: New RPC: `DisplayCombatText(target_id, amount, type, is_crit)`
Option C: Client infers from health delta (heuristic)

**Recommended: Option A** — piggyback on existing `UpdateSharedStats` RPC.

```protobuf
message EntityHealthDelta {
    EntityID entity_id = 1;
    int32 old_health = 2;
    int32 new_health = 3;
    int32 display_damage = 4;   // positive = healing, negative = damage
    DamageType type = 5;        // PHYSICAL/MAGIC/TRUE
    bool is_crit = 6;           // critical hit flag
}
```

Then in `PredictedPlayer.TakeDamage()`:
```csharp
public void TakeDamage(EntityHealthDelta delta)
{
    currentHealth = delta.new_health;
    // NEW: trigger combat text
    if (CombatTextSystem.Instance != null)
    {
        CombatTextSystem.Instance.ShowDamage(
            amount: delta.display_damage,
            type: delta.type,
            isCrit: delta.is_crit,
            worldPosition: this.GlobalPosition + Vector3.Up * 2.0f
        );
    }
}
```

---

## 4. CombatTextSystem API (Existing)

**Verify existing API** before integration. The file likely defines:

```csharp
public class CombatTextSystem : Node
{
    public static CombatTextSystem Instance { get; private set; }

    [Export] PackedScene damageNumberScene;  // DamageNumber.tscn
    [Export] float defaultLifetime = 0.8f;
    [Export] float riseSpeed = 2.0f;

    public void ShowDamage(int amount, DamageType type, bool isCrit, Vector3 worldPos)
    {
        var number = damageNumberScene.Instantiate<DamageNumber>();
        number.Initialize(amount, type, isCrit);
        number.GlobalPosition = worldPos;
        AddChild(number);
        number.AnimateAndQueueFree();
    }
}
```

**If API differs**, adapt integration accordingly. But PRD assumes typical structure.

---

## 5. Animation & Visuals

**DamageNumber scene (`DamageNumber.tscn`):**
```
DamageNumber (Node3D)
├── Label3D (text = "-50")
├── AnimationPlayer (fade_out + rise)
└── CPUParticles2D (impact spark; optional)
```

**Styling:**
- Font size: 32pt normal, 48pt crit
- Colors: Red (0xff4444), Blue (0x4488ff), Green (0x44ff44), Gold (0xffd700)
- Outline: 2px black for readability
- Background: None (transparent)
- Duration: 0.8s (fade alpha 1→0)
- Y offset: start at head height +2.0m, end at +3.5m

**Stacking:**
If 3 hits within 0.5s, offset each successive number:
- Hit 1: y = 2.0
- Hit 2: y = 2.0 + 0.5
- Hit 3: y = 2.0 + 1.0

---

## 6. File Changes

| File | Action | Lines |
|------|--------|-------|
| `src/client/src/combat/DamageNumber.cs` | Review/update styling if needed | +20 (optional) |
| `src/client/src/ui/CombatTextSystem.cs` | Ensure public API `ShowDamage()` exists | +0 (likely exists) |
| `src/client/scenes/Player.tscn` | Attach CombatTextSystem node (if not already) | +1 node |
| `src/client/src/combat/PredictedPlayer.cs` | ADD: call CombatTextSystem in TakeDamage() | +8 |
| `src/client/src/combat/ServerCombatManager.cs` | ADD: set display_damage in RPC payload | +5 |
| `src/client/src/combat/RemotePlayer.cs` | ADD: display damage from server events | +8 |
| `tests/client/TestCombatText.cs` | NEW | +100 |

---

## 7. Implementation Plan

**Phase 1 — Prep (1h):**
1. Read `DamageNumber.cs` and `CombatTextSystem.cs` to confirm API
2. Create simple integration test: manually call `CombatTextSystem.ShowDamage(-10)` in editor

**Phase 2 — Server RPC Update (1h)**
1. Modify `EntityHealthDelta` protobuf message: add `display_damage`, `type`, `is_crit`
2. Update `ServerCombatManager.SendHealthDelta()` to populate fields
3. Rebuild server (Protocol change → test protocol serialization)

**Phase 3 — Client Integration (2h)**
1. `PredictedPlayer.TakeDamage()` → call CombatTextSystem
2. `RemotePlayer.OnHealthDelta()` → call CombatTextSystem (if player sees enemy take damage)
3. Attach `CombatTextSystem` to `Main.tscn` or player scene tree root

**Phase 4 — Styling & Tuning (1h)**
1. Adjust font sizes, colors, lifetimes in `DamageNumber.cs`
2. Test stacking behavior (rapid hits)
3. Verify crit numbers stand out

**Phase 5 — Testing (1h)**
1. Write `TestCombatText.cs` (verify ShowDamage called correct times)
2. Play combat, visually confirm numbers appear
3. Check for memory leaks (count nodes, ensure cleanup)

**Total:** ~6 hours

---

## 8. Acceptance Criteria

✅ **Visual Feedback**
- Damage numbers appear above hit entity's head (red for physical damage)
- Healing numbers appear (green)
- Critical hits show "CRIT" + golden color + larger font
- Numbers fade and rise over 0.8s, then removed from scene

✅ **Technical**
- Zero test regressions (2129 baseline)
- Combat text system does not consume >0.1ms per hit
- No memory leaks: DamageNumber nodes auto-queue_free()
- Works in multiplayer (all clients see each other's damage numbers)

✅ **Code Quality**
- `CombatTextSystem.Instance` pattern used correctly (singleton, autoload)
- Server RPC payload correctly populates display fields
- Unit tests cover ShowDamage invocation

---

## 9. Related PRDs

- **PRD-010** (Hitbox Validation) — damage events source
- **PRD-008** (Combat FSM) — state machine triggers combat events
- **COMPATIBILITY_ANALYSIS.md** — flagged as incomplete visual coherence item

---

## 10. Performance Budget

| Operation | Target Cost |
|-----------|-------------|
| Spawn DamageNumber node | 0.05ms |
| Update Label3D text | 0.01ms |
| Animate + QueueFree | 0.02ms (coroutine) |
| Per-hit total | <0.1ms |

With 10 simultaneous hits on screen: <1ms total — acceptable.

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to UI_AGENT for immediate visual polish integration
