# PRD-022: Combat State Machine Node-Based Template Visual Polish & Finalization

**Version:** 1.0
**Status:** 🟡 Not Started — Needed for issue #6 closure
**Owner:** COMBAT_AGENT
**Priority:** MEDIUM (P3 — shareability / polish)
**Dependencies:** PRD-008 (CombatStateMachine FSM)

---

## 0 — Executive Summary

PRD-008 defined the CombatStateMachine node-based FSM template. This PRD polishes it: assign proper icon resource, create debug overlay hints, produce usage guide, and finalize node template for designer adoption. Addresses the remaining visual/interaction gap from COMPATIBILITY_ANALYSIS: "No node-based FSM template in Godot" — the prototype exists but needs completion for handout.

(This document intentionally brief — PRD-008 covers core system.)

---

## 1 — Problem Statement

The `CombatStateMachine.tscn` packed scene lacks:
- Consistent icon in scene file (makes it hard to identify)
- Debug drawing on states to preview transitions
- Node tooltips/help for designers
- Reference guide with screenshots and wiring steps

Without polish, the node-based FSM remains an internal prototype and fails to demonstrate the intended visual workflow that satisfies the "node-FSM" requirement.

---

## 2 — Acceptance Criteria

- PackedScene icon.png exists (16×16 and 32×32) at `src/client/scenes/godot-icons/CombatStateMachine.icon`
- Each GraphNode inside the template displays a colour-coded label matching GCD mechanism
- `docs/COMBAT-FSM-USAGE.md` exists with annotated screenshots
- CI resource-check fails if missing icon.svg or usage guide missing
- Folder `grip-content` optional but not required

### By Checklist

- [ ] Icon assigned to CombatStateMachine.tscn
- [ ] GraphNode tooltips completed
- [ ] State debug overlay (tint) implemented
- [ ] `COMBAT-FSM-USAGE.md` drafted
- [ ] CI check `icon_and_usage.py` added

---

## 3 — Deliverables

1. `src/client/scenes/godot-icons/CombatStateMachine.svg` (or .png)
2. `src/client/scenes/CombatStateMachine.tscn` updated with icon and GraphNode hints
3. `docs/COMBAT-FSM-USAGE.md` — step-by-step guide
4. `.github/workflows/ci.yml` step (new) to validate presence of icon + doc
5. `scripts/test_combatfsm_res.py` validation script

**Owner:** COMBAT_AGENT

---

## 4 — Open Questions

- Should each state GraphNode get a distinct icon? → Likely too heavy; label only.
- Should GraphModifers plugins add an on-hover annotation panel? → optional
