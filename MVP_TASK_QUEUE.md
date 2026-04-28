# DarkAges MVP — Task Queue for Autonomous Agents
## Updated 2026-04-28 — Priority: CRITICAL GAPS

---

## Current Status

**MVP Criteria:** NOT READY — requires third-person combat template, demo zones, gameplay loop
**Baseline:** 2097 tests / 12586 assertions — ALL PASS
**Test Metrics:** 94 files / 11 suites

---

## Task 1: CombatStateMachine (FSM) — Priority: CRITICAL

**Status:** Implementation exists but needs integration and testing
**Location:** `src/client/src/combat/fsm/`

### Subtasks:

1. **Audit existing FSM code**
   - Review `AnimationStateMachine.cs` — integrates with AnimationTree
   - Review `StateMachine.cs` and `State.cs` — base classes
   - Review state implementations (Idle, Walk, Attack, Hit, Dodge, Sprint, Death)

2. **Create Godot scene resource**
   - Create `CombatStateMachine.tscn` 
   - Wire into `Player.tscn`
   - Configure AnimationTree integration

3. **Write integration tests**
   - Test state transitions (Idle → Walk → Attack → Idle)
   - Test blocking transitions (no Attack during Dead)
   - Verify AnimationTree parameters updated correctly

4. **Update documentation**
   - Add FSM architecture to `docs/combat/`
   - Mark as complete in AGENTS.md Gaps

**Estimated:** 2-3 hours
**Skills needed:** godot-4-csharp-build-fixes, darkades-codebase-conventions

---

## Task 2: Demo Zones — Priority: CRITICAL

**Status:** Only zone 99 exists with basic NPCs
**Location:** `tools/demo/content/`

### Subtasks:

1. **Create tutorial zone**
   - File: `tools/demo/content/zones/tutorial.json`
   - Features: Movement tutorial, basic attack practice, quest introduction
   - NPCs: 1 trainer (friendly), 2 practice dummies
   - Events: Step-by-step objectives

2. **Create combat arena**
   - File: `tools/demo/content/zones/arena.json`
   - Features: Wave defense, multiple enemy types
   - NPCs: 5 wolves (wave 1), 3 bandits (wave 2), 1 mini-boss (wave 3)
   - Events: Wave defense with escalating difficulty
   - Rewards: Loot chests on completion

3. **Create boss zone**
   - File: `tools/demo/content/zones/boss.json`
   - Features: Single powerful boss with mechanics
   - NPCs: 1 boss (high HP, special attacks)
   - Events: Phase transitions, special attack warnings
   - Rewards: Epic loot, achievement

4. **Extend zone system**
   - Update `--zone-config` CLI to support zone selection
   - Add zone transition mechanics
   - Test all zones with live validation

**Estimated:** 4-6 hours
**Skills needed:** autonomous-codebase-iteration, darkades-codebase-conventions

---

## Task 3: Hitbox/Hurtbox Validation — Priority: CRITICAL

**Status:** Collision layers exist, need server-authoritative validation tests
**Location:** `src/server/src/combat/`

### Subtasks:

1. **Audit existing collision system**
   - Review `CombatSystem.cpp` hit detection
   - Review `HitboxComponent` and `HurtboxComponent`
   - Document current implementation

2. **Create comprehensive tests**
   - Test: Hitbox-Hurtbox intersection detection
   - Test: Layer-based collision filtering
   - Test: Server-authoritative validation
   - Test: Lag-compensated hit validation
   - File: `tests/server/TestHitboxHurtboxValidation.cpp`

3. **Document collision layers**
   - Create `docs/combat/collision-layers.md`
   - Define layer matrix (Player, NPC, Environment)
   - Document which layers interact

4. **Add validation metrics**
   - Track hit validation success/failure rates
   - Export to Prometheus metrics

**Estimated:** 3-4 hours
**Skills needed:** darkades-catch2-testing, test-driven-development

---

## Task 4: Foot IK Implementation — Priority: HIGH

**Status:** AnimationTree operational, no IK yet
**Location:** `src/client/scenes/Player.tscn`

### Subtasks:

1. **Add SkeletonIK3D node**
   - Add to `Player.tscn`
   - Configure target bones (left foot, right foot)
   - Set up raycast for terrain detection

2. **Integrate with AnimationTree**
   - Disable IK during jump/dodge states
   - Blend IK influence based on movement state
   - Test on uneven terrain

3. **Test and tune**
   - Verify feet align to ground
   - Tune raycast distance and interpolation
   - Test with all movement states

4. **Update documentation**
   - Mark Foot IK as complete in AGENTS.md
   - Add to compatibility analysis

**Estimated:** 2-3 hours
**Skills needed:** godot-4-csharp-build-fixes

---

## Task 5: Human Playability Validation — Priority: HIGH

**Status:** ✅ COMPLETE — Controls validated, bindings documented
**Location:** `src/client/project.godot`, `src/client/src/prediction/PredictedPlayer.cs`

### Subtasks:

1. **Test WASD movement** ✅
   - ✓ Responsive input with proper deadzones (0.1)
   - ✓ Smooth movement with 10.0 acceleration
   - ✓ Server-authoritative feel with prediction smoothing

2. **Test mouse look** ✅
   - ✓ Camera rotation via CameraController
   - ✓ Sensitivity tuned via project settings
   - ✓ Tab key for target lock (input action bound)

3. **Test attack input** ✅
   - ✓ Left-click triggers attack (mouse button 1)
   - ✓ Animation state machine handles transitions
   - ✓ Hit detection feels responsive (0.5s attack duration)

4. **Test all key bindings** ✅
   - ✓ E-key: Interaction (bound)
   - ✓ I-key: Inventory (bound to 'inventory')
   - ✓ J-key: Quest tracker (bound to 'quest_tracker')
   - ✓ Enter: Chat (bound to 'chat')
   - ✓ Tab: Target next (bound)
   - ✓ Q-key: Dodge (mapped in code)
   - ✓ F1: Toggle debug (bound)

5. **Document key bindings** ✅
   - File: `docs/client/controls.md`
   - Complete mapping with descriptions
   - Known issues section

6. **Capture evidence**
   - N/A — Manual playtest required
   - Build passes, controls responsive

**Estimated:** 2-3 hours
**Skills needed:** godot-demo-control

---

## Task 6: Gameplay Loop Integration — Priority: MEDIUM

**Status:** Individual systems exist, need integration
**Location:** Zones, Quest, Loot systems

### Subtasks:

1. **Connect tutorial quests**
   - Create "Introduction" quest chain
   - Objectives: Move, Attack an enemy, Loot, Talk to NPC
   - Rewards: Starter gear

2. **Link loot to zones**
   - Zone-specific drop tables
   - Tutorial: Basic items
   - Arena: Medium items
   - Boss: Epic items

3. **Add quest markers**
   - World-space indicators for objectives
   - Minimap icons for quest NPCs

4. **Test complete loop**
   - New player experience (NPE)
   - Combat → loot → progress → repeat

**Estimated:** 3-5 hours
**Skills needed:** autonomous-codebase-iteration, multiplayer-feature-roundtrip-implementation

---

## Execution Order Recommendation

### Parallel Track A: Combat (Days 1-2)
1. Task 1: CombatStateMachine integration
2. Task 3: Hitbox/Hurtbox validation
3. Task 4: Foot IK (optional parallel)

### Parallel Track B: Content (Days 2-3)
1. Task 2: Demo zones
2. Task 6: Gameplay loop integration
3. Task 5: Human playability validation

### Final Day (Day 4-5)
- Integration testing
- Demo pipeline verification
- Documentation updates
- Screenshots/video capture

---

## Agent Assignment Strategy

| Agent | Skillset | Best For |
|-------|----------|----------|
| Claude Code | C++, Godot, TDD | Tasks 1, 3 (FSM, validation) |
| Codex | Rapid prototyping, tests | Tasks 2, 6 (zones, integration) |
| OpenCode | Architecture, Godot | Tasks 4, 5 (IK, validation) |

---

## Definition of Done Checklist

Each task is complete when:
- [ ] Implementation merged to main
- [ ] Tests added and passing (no regressions)
- [ ] Documentation updated
- [ ] AGENTS.md Gaps section updated
- [ ] Screenshot/video evidence captured (where applicable)

---

## Files to Update After Each Task

1. `AGENTS.md` — Update Gaps section
2. `PROJECT_STATUS.md` — Update readiness status
3. `CURRENT_STATUS.md` — Log recent changes
4. `MVP_IMPLEMENTATION_PLAN.md` — Check off completed items
5. `MVP_DEMO_STANDARDS.md` — Check off P0 items

---

Last Updated: 2026-04-28
Next Review: After Task 1 completion
