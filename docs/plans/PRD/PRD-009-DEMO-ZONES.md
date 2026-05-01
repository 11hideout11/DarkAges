# PRD-009: Demo Zones System — Complete 3-Zone Configuration

**Version:** 1.0
**Status:** 🔄 In Progress — Zone configs exist, need validation & pacing
**Owner:** ZONES_AGENT
**Priority:** CRITICAL (P0 — MVP Blocking)
**Dependencies:** PRD-001 (Server Core), PRD-002 (Networking)

---

## 1. Overview

### 1.1 Purpose

Ensure the three core demo zones (tutorial, arena, boss) are fully configured with proper gameplay pacing, encounter design, NPC spawn logic, and zone-specific events that create a curated demo experience showcasing all core DarkAges systems.

### 1.2 Scope

- Validate existing `tutorial.json`, `arena.json`, `boss.json` zone configurations
- Add missing pacing parameters (wave timing, NPC counts, difficulty scaling)
- Ensure zone transitions work (demo mode auto-advances through zones)
- Add zone-specific objectives and events
- Document zone design patterns for future content creators

### 1.3 Out of Scope

- Creating new zone art/assets (uses existing capsule models)
- Persistent zone storage (Redis/Scylla disabled for demo)
- Dynamic zone generation (future feature)

---

## 2. Requirements

### 2.1 Zone Configuration Requirements

| ID | Requirement | Priority | Current Status |
|----|-------------|----------|----------------|
| ZONE-001 | Tutorial zone (zone_id 98) complete config | P0 | ✅ Exists, needs validation |
| ZONE-002 | Combat arena zone (zone_id 99) complete config | P0 | ✅ Exists, needs pacing |
| ZONE-003 | Boss zone (zone_id 100) complete config | P0 | ✅ Exists, needs events |
| ZONE-004 | Zone-to-zone demo flow (auto-advance) | P0 | ⚠️ Needs implementation |
| ZONE-005 | NPC spawn archetype variety per zone | P0 | ✅ Partial, needs expansion |
| ZONE-006 | Zone-specific objectives/events | P1 | ⚠️ Minimal, needs enrichment |
| ZONE-007 | Encounter pacing (wave timing, difficulty curves) | P0 | ⚠️ Needs tuning |

### 2.2 Functional Requirements

- **Tutorial (zone 98)**: Teach movement, basic combat, interaction, UI
- **Arena (zone 99)**: Wave defense — escalating combat encounters
- **Boss (zone 100)**: Single boss fight with mechanics (phases, adds)
- **Demo progression**: Auto-advance: tutorial → arena → boss → summary
- **NPC variety**: At least 3 archetypes per zone (melee, ranged, support)
- **Objective tracking**: Zone objectives display in UI; completion gates progression

### 2.3 Non-Functional Requirements

- Zone configs load in <100ms
- JSON schema validated at demo startup
- No hardcoded values — all config-driven
- Backwards compatible with existing zone server

---

## 3. Current Gap Analysis

**Gap:** While zone JSON files exist (`tools/demo/content/zones/*.json`), they are **not yet validated** for proper demo pacing and encounter design per the updated MVP criteria which requires "zones with proper gameplay pacing and encounter design" and "zone-specific events and objectives."

**Evidence:**
- Files present: `tutorial.json` (3.1KB), `arena.json` (5.4KB), `boss.json` (7.3KB)
- But missing:
  - Explicit wave timing configurations
  - Objective definitions with completion criteria
  - Zone transition/advance logic in demo orchestrator
  - Validation of NPC spawn rates, level scaling
  - Documentation of zone design patterns

**Impact:** Demo may run but lacks curated experience; zones might not flow logically or provide satisfying progression. MVP requires "curated demo experience" which needs intentional pacing.

---

## 4. Current Zone State (as of 2026-05-01)

### 4.1 Tutorial Zone (zone_id: 98)

**Present:**
- Spawn points: player start, NPC trainer, practice dummy, item chest
- NPC presets: training_dummy (3x), rabbit (5x passive)
- Basic map bounds, safe zone flag

**Missing:**
- Quest objectives (talk to Aldric, hit dummy 5x, collect gear)
- Scripted tutorial events (triggered on proximity/completion)
- Validation gates (must complete before advancing)
- Objective display in UI
- NPC dialogue/instruction text

### 4.2 Arena Zone (zone_id: 99)

**Present:**
- Spawn points: player entrance, arena center, 4 wave spawn points (NW/NE/SW/SE)
- Wave configuration: 5 waves, increasing difficulty
- NPC archetypes: goblin_melee, goblin_ranged, goblin_shaman
- Reward drops configured

**Missing:**
- Wave timing (how long between waves? 30s? 60s?)
- Wave completion detection (all NPCs dead?)
- Wave advance trigger (explicit countdown or instant)
- Difficulty scaling validation (HP/damage per wave)
- Arena-specific UI (wave counter, timer)

### 4.3 Boss Zone (zone_id: 100)

**Present:**
- Boss entity: `boss_goblin_king` with 500 HP, special abilities
- Adds spawn points (4 adds per phase)
- 3 phases with health thresholds (75%, 50%, 25%)
- Loot table: legendary_items, zone_exit_key

**Missing:**
- Phase transition effects (visual/audio cues)
- Boss arena boundaries (prevent kiting)
- Phase-specific mechanics (avoid AoE, kill adds first)
- Boss intro/timeline (cinematic spawn)
- Victory condition detection

### 4.4 Demo Orchestrator Gaps

The `tools/demo/demo` launcher currently:
- Runs a single zone (default zone 99) or uses `--zone-config`
- Does NOT auto-advance through zones sequentially
- Does NOT gate progression on objective completion
- Does NOT display zone-specific UI feedback

**Need:** Demo mode should auto-progress: tutorial → arena → boss → report

---

## 5. Implementation Plan

### Phase 1: Zone Pacing & Objectives Schema

**Extend zone JSON schema** with:

```json
{
  "zone_id": 98,
  "pacing": {
    "duration_seconds": 120,          // Max time player should spend
    "advance_condition": "objectives_complete",  // "timer", "boss_dead", etc.
    "objective_display": true
  },
  "objectives": [
    {
      "id": "talk_to_aldric",
      "description": "Speak with Instructor Aldric",
      "type": "interact",
      "target_npc": "Instructor Aldric",
      "required": true,
      "hidden": false
    },
    {
      "id": "kill_5_dummies",
      "description": "Defeat 5 training dummies",
      "type": "kill",
      "target_archetype": "training_dummy",
      "count": 5,
      "required": true
    }
  ],
  "events": [
    {
      "trigger": "objective_complete:talk_to_aldric",
      "action": "spawn_npcs",
      "npcs": ["Training Dummies"],
      "delay_seconds": 2
    }
  ]
}
```

**Files to update:**
- `tools/demo/content/zones/tutorial.json` — add `pacing`, `objectives`, `events`
- `tools/demo/content/zones/arena.json` — add wave timer, wave-complete detection
- `tools/demo/content/zones/boss.json` — add phase transition events, win condition

### Phase 2: Zone Objective System (Server)

**Create `src/server/src/zones/ZoneObjectiveSystem.cpp`:**
- Tracks objective progress per player
- Emits events on objective completion
- Sends objective updates to client (via existing snapshot system)
- Gates zone advance when `advance_condition` met

**Component:** `ZoneObjectiveComponent` (attached to player in zone)

**Tests:** `TestZoneObjectives.cpp` — validate completion logic

### Phase 3: Zone Progression in Demo Orchestrator

**Update `tools/demo/run_demo.py`:**
- Instead of single zone, run sequence: `[tutorial, arena, boss]`
- After each zone completes, wait 5s, then load next zone
- Capture screenshot at each milestone
- Aggregate results into single demo report

**CLI flags:** `--demo-sequence` (default: `tutorial,arena,boss`)

### Phase 4: Zone UI Integration

**Update Godot client UI** (`src/client/scenes/GameUI.tscn`):
- Add "Zone Objectives" panel (top-left)
- Display active objectives with checkboxes/progress bars
- Show wave counter in arena
- Show boss phase indicator
- Display "Zone Complete! Loading next..." transition overlay

**Script:** `ZoneObjectivesUI.cs` — listens to snapshot updates

### Phase 5: Pacing Validation

**Create validation script** `tools/demo/validate_zone_pacing.py`:
- Checks each zone config has:
  - At least 3 objectives in tutorial
  - Wave timer defined in arena
  - Boss phase events present
- Fails demo if config invalid

---

## 6. Deliverables

| Item | Path | Type |
|------|------|------|
| Updated Tutorial Config | `tools/demo/content/zones/tutorial.json` | JSON |
| Updated Arena Config | `tools/demo/content/zones/arena.json` | JSON |
| Updated Boss Config | `tools/demo/content/zones/boss.json` | JSON |
| Objective System (server) | `src/server/src/zones/ZoneObjectiveSystem.cpp` | C++ |
| Objective Component | `src/server/include/zones/ZoneObjectiveComponent.hpp` | C++ |
| Objective Tests | `src/server/tests/TestZoneObjectives.cpp` | C++ |
| Demo Orchestrator Update | `tools/demo/run_demo.py` (mod) | Python |
| UI Update | `src/client/scenes/GameUI.tscn` + `ZoneObjectivesUI.cs` | Godot + C# |
| Validation Script | `tools/demo/validate_zone_pacing.py` | Python |
| Documentation | `docs/zone-configuration-guide.md` | Markdown |

---

## 7. Acceptance Criteria

**Zone Configs:**
- [ ] `tutorial.json` has ≥3 objectives, events list, pacing block
- [ ] `arena.json` has `wave_config` with timers and completion rules
- [ ] `boss.json` has `phase_events` and victory condition

**Server:**
- [ ] ZoneObjectiveSystem tracks objectives per player
- [ ] Objectives broadcast in snapshots
- [ ] Tests pass: `TestZoneObjectives.cpp` (new)
- [ ] No regression: baseline 2129 test cases

**Client:**
- [ ] UI panel shows zone objectives during demo
- [ ] Wave counter visible in arena zone
- [ ] Boss phase indicator visible in boss zone
- [ ] Smooth transition between zones (5s black screen + countdown)

**Demo Orchestrator:**
- [ ] `./tools/demo/demo --full-sequence` runs: tutorial → arena → boss
- [ ] Report includes per-zone metrics (time, kills, objectives)
- [ ] Screenshots captured at zone transitions

**Build:**
- [ ] `cmake --build build_validate -j$(nproc)` — passes
- [ ] `ctest --output-on-failure -j1` — all tests pass

---

## 8. Test Plan

### Unit Tests

- `TestZoneObjectives.cpp`: Objective tracking, completion logic, persistence
- `TestZoneConfigSchema.cpp`: JSON schema validation for all 3 zones

### Integration Tests

- `test_zones` (existing) — ensure zone setup/teardown still works
- Demo pipeline integration: run full sequence, verify all zones load

### Manual QA

- Play tutorial: complete all objectives, verify zone advances
- Play arena: survive 5 waves, verify wave transitions
- Play boss: reduce to 25% HP, verify phase 3 triggers

---

## 9. Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Zone configs become bloated | Use JSON schema validation; keep optional fields |
| Objective system adds server overhead | Benchmark: <1ms per zone tick (well within 16.67ms budget) |
| Client UI overwhelms screen | Use compact, collapsible panel; hide after inactivity |
| Demo sequence too long ( >3min) | Adjust zone durations; use `--quick` sequence variant |

---

## 10. Related Documents

- PRD-001: Server Core
- PRD-005: Client Architecture
- `tools/demo/MVP_DEMO_STANDARDS.md` — Gap analysis source
- `docs/plans/PRP/PRP-XXX-ZONE-DESIGN.md` (to be created as follow-up)

---

**Last Updated:** 2026-05-01
**PRD Author:** Autonomous Agent (gap analysis)
