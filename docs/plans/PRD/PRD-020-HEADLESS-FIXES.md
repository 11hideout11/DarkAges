# PRD-020: Godot Client Headless Artifacts — Test/CI Stability

**Version:** 1.0
**Status:** 🔄 Not Started — Headless test flakiness due to `_Ready() add_child()` in non-scene context
**Owner:** TESTING_AGENT
**Priority:** MEDIUM (P3 — CI Reliability)
**Dependencies:** None (affects test harness only)
**Issue:** #8 from PROJECT_ISSUES_TRACKER.md

---

## 1. Overview

### 1.1 Problem
When running Godot client in headless mode (`--headless` or `.headless=true` in RICH), the following errors occur:
```
ERROR: _ready: Node already has a parent, can't add to another.
ERROR: Condition "node->is_inside_tree()" is true.
ERROR: Trying to add child 'Player' to scene root but it already has parent
```

**Root cause:** `Player._Ready()` (or `RemotePlayer._Ready()`) calls `AddChild(someNode)` unconditionally. In headless CI/test harness, the scene tree structure is different — nodes may already be parented or the root is read-only.

### 1.2 Impact
- CI flakiness (tests fail intermittently on Linux runners)
- Demo validator crashes on WSL2 (headless environment)
- Prevents full headless automation of client-side validation

### 1.3 Scope
`src/client/` files that manipulate scene tree in `_Ready()`:
- `Player.cs` — potential `AddChild` calls
- `RemotePlayer.cs` — potential `AddChild` calls
- `Main.cs` — scene loading/unloading logic
- `CameraController.cs` — node attachment

**Also affected:** Any client script that constructs nodes dynamically during `_Ready()`.

---

## 2. Requirements

### 2.1 Functional Requirements
ID    | Requirement                         | Priority | Details
------|-------------------------------------|----------|--------
HEAD-001 | Safe node parenting in headless  | P0       | `AddChild()` guarded by `IsInsideTree()` check
HEAD-002 | No crashes in `--headless` mode    | P0       | Client starts and runs diagnostics without error
HEAD-003 | CI test stability                 | P0       | 100% pass rate for client-instrumentation tests
HEAD-004 | Demo validator robustness         | P1       | `demo_validator.py` never crashes on node ops
HEAD-005 | Scene tree leak prevention        | P1       | Nodes properly orphaned between demo runs

### 2.2 Non-Functional
- Zero API change for normal (non-headless) mode
- No performance overhead (guard checks <0.1µs)
- No changes to network protocol or server

---

## 3. Current Error Analysis

**Typical error trace:**
```
ERROR: _ready: Node already has a parent, can't add to another.
   at: add_child (line 123 in Player.cs)
   in: Player._Ready()
   caused by: hud = new CanvasLayer(); AddChild(hud);
```

**Why this happens:**
In headless mode, `Main` scene may auto-add `Player` to a test harness root node. Then `Player._Ready()` executes and tries to add `hud` node — but if `hud` already has a parent (from previous test), `AddChild` fails.

**Common scenarios:**
1. **Test reuse:** Same Player instance reused across test cases without clearing children
2. **Demo validator:** Loads Player.tscn multiple times in same process
3. **CI**: Runs many client-instrumentation tests in one process (atomically)

---

## 4. Technical Solution

### 4.1 Guard Pattern for AddChild/RemoveChild

**Before (fragile):**
```csharp
public override void _Ready()
{
    hud = new CanvasLayer();
    AddChild(hud);  // ❌ Fails if hud already parented
}
```

**After (robust):**
```csharp
public override void _Ready()
{
    hud = new CanvasLayer();
    // Safe add: only if not already in tree
    if (hud.GetParent() == null)
        AddChild(hud);
    // OR: Remove from previous parent first
    if (hud.GetParent() != null)
        hud.GetParent().RemoveChild(hud);
    AddChild(hud);
}
```

**Helper extension method** (centralized solution):
```csharp
// src/client/src/util/NodeExtensions.cs
public static class NodeExtensions
{
    public static void SafeAddChild(this Node node, Node child)
    {
        if (child.GetParent() != null)
            child.GetParent().RemoveChild(child);
        node.AddChild(child);
    }
}

// Usage:
public override void _Ready()
{
    hud = new CanvasLayer();
    this.SafeAddChild(hud);  // ✓ Always safe
}
```

### 4.2 Scene Tree Reset Between Demo Runs

**Problem:** Demo validator loads Player, runs demo, then loads next zone — leftover nodes persist.

**Solution:** `DemoOrchestrator` (in `tools/demo/demo_orchestrator.py`) calls:
```python
# After each demo run:
player_node.queue_free()  # Free entire player subtree
# Wait for idle_frame to ensure GC
```

**Or better:** Recreate entire player scene from scratch each time:
```python
player = load("res://scenes/Player.tscn").Instantiate()
get_tree().Root.AddChild(player)
```

### 4.3 Headless Detection

Add `OS.IsEditor()` + `OS.IsStdOutVerbose()` + headless flag check:
```csharp
bool isHeadless = OS.IsStdOutVerbose() || DisplayServer.GetName() == "headless";
if (isHeadless)
{
    // Skip adding visual-only nodes (HUD, debug overlays)
}
```

But better: always use `SafeAddChild` — same code path for all modes.

---

## 5. Files to Modify

| File | Issue | Fix |
|------|-------|-----|
| `src/client/src/combat/fsm/Player.cs` | HUD add in _Ready() | Wrap with SafeAddChild |
| `src/client/src/RemotePlayer.cs` | Debug label add in _Ready() | Guard with `GetParent()==null` |
| `src/client/src/Main.cs` | Scene swapping logic | Ensure `RemoveChild` before reusing |
| `src/client/src/CameraController.cs` | Camera node attach | Use SafeAddChild |
| `src/client/src/util/NodeExtensions.cs` | NEW | Helper extension method |
| `tests/client/TestHeadlessStability.cs` | NEW | Load/unload cycles |
| `tools/demo/demo_orchestrator.py` | Mod | Ensure player.free() between zones |

---

## 6. Testing Strategy

### 6.1 Automated Test: Headless Stability

**New test file:** `tests/client/TestHeadlessStability.cs`

```csharp
[Test]
public void Headless_LoadUnloadPlayer100Times_NoError()
{
    // Run 100 load/unload cycles in headless
    for (int i = 0; i < 100; i++)
    {
        var player = (Player)GD.Load<PackedScene>("res://scenes/Player.tscn").Instantiate();
        GetTree().Root.AddChild(player);
        player.Ready += () => { /* wait for _Ready complete */ };
        player.QueueFree();
        GetTree().Root.RemoveChild(player);
    }
    // If we reach here, no exceptions thrown ✓
}
```

**Expected:** Zero errors, 100 iterations complete in <5 seconds.

### 6.2 CI Job Addition

`.github/workflows/ci-client.yml` — add matrix job:
```yaml
client-headless-test:
  runs-on: ubuntu-latest
  strategy:
    matrix:
      iterations: [10, 50, 100]
  steps:
    - run: dotnet test tests/client/TestHeadlessStability.cs --iterations=${{ matrix.iterations }}
```

### 6.3 Demo Validator Impact

`tools/demo/demo_validator.py` — previously crashed on zone 2 load due to node reuse.
**Fix needed:** After `validated_count += 1`, call:
```python
if player_node:
    player_node.queue_free()
    player_node = None
```

Add test: Run full 3-zone sequence 10× in a loop → 0 crashes.

---

## 7. Acceptance Criteria

✅ **Functional Stability**
- Client loads in `--headless` mode without any `ERROR: _ready: Node already has a parent` messages
- Demo validator runs full 3-zone sequence (tutorial→arena→boss) 10× in a row with 0 crashes
- Headless load/unload cycle 100× in automated test passes

✅ **Code Quality**
- `NodeExtensions.SafeAddChild()` exists and used in all dynamic node attach sites
- No direct `AddChild()` calls on freshly constructed nodes without guard
- Code review: C# analyzer passes (no warnings)

✅ **Performance**
- Guard checks: <0.1µs overhead per add (negligible)
- No memory leaks: nodes properly freed between runs (valgrind clean on headless test)

✅ **CI Stability**
- `client-headless-test` job added and passing
- Demo validator job (if exists) passes consistently

---

## 8. Implementation Checklist

**Phase 1 — Audit (1h):**
- [ ] Grep for `AddChild(new` and `AddChild(` in `src/client/src/`
- [ ] Identify all `_Ready()` methods that dynamically add children
- [ ] List files to modify (see table in Section 5)

**Phase 2 — Helper (30m):**
- [ ] Write `NodeExtensions.SafeAddChild()`
- [ ] Write unit test for SafeAddChild (removes old parent correctly)

**Phase 3 — Fix Fragile Sites (2h):**
- [ ] Modify Player.cs: Replace `AddChild(hud)` → `SafeAddChild(hud)`
- [ ] Modify RemotePlayer.cs: Add guard before add_child for debug overlay
- [ ] Modify Main.cs: Ensure scene transitions clean up old nodes
- [ ] Modify CameraController.cs: Guard camera attachment
- [ ] Modify any other scripts that call `AddChild` in `_Ready()` or `_EnterTree`

**Phase 4 — Test (1h):**
- [ ] Write `TestHeadlessStability.cs` (100× load/unload)
- [ ] Run locally: `dotnet test TestHeadlessStability.cs`
- [ ] Verify passes

**Phase 5 — CI Integration (1h):**
- [ ] Add headless test job to `.github/workflows/ci-client.yml`
- [ ] Fix demo_validator.py: ensure `queue_free()` after each demo
- [ ] Run CI and confirm 100% pass

**Phase 6 — Validate Demo Pipeline (30m):**
- [ ] Run full demo 10×: `tools/demo/run_demo.py --full 10`
- [ ] Verify 0 crashes, 0 node errors in logs

**Total:** ~6 hours agent time

---

## 9. Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| SafeAddChild removes parent unintentionally | Breaks intentional single-parent | Only call SafeAddChild on newly-constructed nodes (never on scene-editor nodes) |
| Headless mode differs from windowed (Godot bug) | Errors persist | Test both modes locally; add OS-specific guards |
| Demo validator uses different loading code | Flakiness remains | Update demo_validator.py to use SafeAddChild logic too |

---

## 10. Success Validation

Run test suite:
```bash
# Headless stress test
dotnet test tests/client/TestHeadlessStability.cs --filter "Iterations=1000"

# Demo validator 10-zone loop
python tools/demo/demo_validator.py --zones tutorial,arena,boss --loops 10

# CI verification
gh workflow run ci-client.yml  # watch for 100% over 10 runs
```

Expected output:
```
TestHeadlessStability: PASS (1000 iterations, 0 failures)
demo_validator: 30/30 zones validated, 0 crashes
CI: All checks passed
```

---

## 11. Related PRDs

- **PRD-009** (Demo Zones) — this fixes the validator reliability for zone runs
- **PRD-020** (This file) — infrastructure fix enabling reliable demo runs

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to TESTING_AGENT for immediate CI stability fix
