# DarkAges Implementation Plan - PRD Progression

**Last Updated:** 2026-05-03  
**Review Focus:** Resume development from end-of-session summary
**Top-Priority Action:** Fix boss zone NPC definitions (Gap #1)

---

# 1. OBJECTIVE

Fix the MVP readiness gap for Demo Zones - specifically the boss zone that has 0 NPCs due to missing `npc_presets` in boss.json. This is the top-priority actionable item that is not blocked by external dependencies.

---

# 2. CONTEXT SUMMARY

## End-of-Session Summary (AGENTS.md, 2026-05-03)

### Completed Tasks:
- Fixed 47 C# build errors in Godot 4.2 client
- All tests passing: 1305 cases, 7254 assertions, 100%
- Demo pipeline: 5/5 checks pass
- 22 PRDs completed during execution

### Active Gaps (Internal - Feasible Now):
1. **Boss zone NPC definitions** - `boss.json` missing `npc_presets` array (top priority)
2. **Zone objectives** - zone JSON configs lack `zone_objectives` array

### External Blockers:
- GNS runtime - WebRTC auth token required
- Production DB - Docker daemon required

### MVP Readiness Status:
| Requirement | Status | Notes |
|---|---|---|
| P0-1: Combat Multiplayer Template | ✅ COMPLETE | FSM, hitbox/hurtbox, AnimationTree, IK, lock-on |
| P0-2: Demo Zones | ⚠️ PARTIAL | 3 zones exist; boss zone has 0 NPCs |
| P0-3: Gameplay | ✅ COMPLETE | Human-playable, visual feedback, demo mode |

---

# 3. APPROACH OVERVIEW

Add `npc_presets` array to boss.json matching the pattern used in tutorial.json and arena.json. This will enable the boss entity (Gruk The Unstoppable) and minions to spawn when the zone loads.

The fix follows the existing pattern:
- archetype: maps to NPC type definition
- count: number to spawn
- spawn_at: spawn point reference
- level, combat_type, behavior: boss-specific settings

---

# 4. IMPLEMENTATION STEPS

## Step 4.1: Add npc_presets to boss.json
**Goal:** Enable boss entity spawning
**Method:** Add npc_presets array after spawn_points, matching tutorial.json pattern

Reference: `tools/demo/content/zones/boss.json`

Tasks:
- [x] Identify missing npc_presets array in boss.json
- [ ] Add npc_presets with boss (ogre_chieftain) + minions (wolf, bandit)
- [ ] Verify JSON syntax is valid

## Step 4.2: Validate Demo Pipeline
**Goal:** Verify boss zone now spawns NPCs
**Method:** Run demo pipeline checks

Reference: `tools/demo/demo --quick`

Tasks:
- [ ] Verify boss.json parses correctly
- [ ] Confirm npc_presets are detected on zone load

---

# 5. TESTING AND VALIDATION

## Validation Criteria
- boss.json parses as valid JSON
- npc_presets array present with 3 entries (boss + 2 minion types)
- Demo pipeline confirms boss entity loads (future check when build available)

## Success Indicators
- boss.json has npc_presets array
- Archetypes reference valid NPC types (ogre_chieftain, wolf, bandit)
- Spawn points correctly referenced
- MVP P0-2 Demo Zones: boss zone now has NPCs instead of 0
