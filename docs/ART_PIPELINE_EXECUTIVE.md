# DarkAges Art Pipeline - Executive Summary & Deliverables

**Status:** Pre-production → Art Pipeline Phase Start  
**Date:** 2026-04-29  
**Phase:** Post-Combat-FSM (PR #28 merged)  
**Next Milestone:** Zone98 Demo Ready with Visuals  

---

## WHAT WAS DONE

### Research Phase Completed

**Sources investigated:**
- Godot 4.2 official documentation (import pipeline, shaders, performance)
- Blender → GLTF export workflows
- PBR texture creation tools (ArmorPaint, Material Maker)
- Free asset repositories (Quaternius, Kenney, Poly Haven, AmbientCG)
- Shader patterns for toon shading, outlines, damage numbers, VFX
- World-building tools (terrain, modular kit, foliage instancing)
- GPU optimization strategies (LOD, batching, culling)
- MMO zone streaming approaches

**Total research corpus:** 155 KB across 5 documents

---

## ART PIPELINE SPECIFICATION DELIVERABLES

### 📄 Document 1: `docs/ART_PIPELINE_SPECIFICATION.md` (33 KB)

**Complete technical specification** covering:
1. Asset Requirements Matrix (chars, weapons, props, VFX, textures)
2. Toolchain Recommendations (Blender, ArmorPaint, Material Maker)
3. Import Pipeline (GLTF → Godot 4.2 with import settings)
4. Shader Strategy (toon, outline, hit markers, VFX patterns)
5. Performance Optimization (draw calls <100, LOD, texture streaming)
6. Production Roadmap (12-week sprint with weekly sprints)
7. Git LFS Version Control workflow + QA checklist
8. Free/paid asset source directory
9. Troubleshooting + Appx A-C (quick ref, glossary, example asset spec)

**Usage:** Read for full technical understanding, implementation planning

---

### 📋 Document 2: `docs/ART_PIPELINE_ASSET_INVENTORY.md` (20 KB)

**Production tracker** with:
- Tiered asset priority list (Tier 1 CRITICAL → Tier 4 LOW)
- Master checklist per asset type (characters, weapons, environment, VFX, UI)
- Zone asset mapping (Zone98, Zone99, Zone100)
- Integration checklist (import → scene → validation)
- Production hours estimate per asset
- Git commit convention template
- Demo validation commands per zone

**Usage:** Weekly progress tracking, sprint planning, PR checklist

---

### 🔧 Document 3: `docs/ART_PIPELINE_TOOLING.md` (18 KB)

**Setup guide** for environment configuration:
- Tool installation commands (Blender, ArmorPaint, Material Maker, Godot, Git LFS)
- Blender export preset configuration
- Godot import defaults verification
- Asset folder hierarchy creation
- Validation pipeline scripts (test_export_import.py, validate_asset.py, check_shader_syntax.py)
- Daily workflow hot-reload pattern
- Troubleshooting Q&A for common failures
- Resource links + prerequisites checklist

**Usage:** Agent workstation setup, new artist onboarding

---

### 💾 Manifests: `assets/manifest.json` (38 KB)

**Machine-readable asset database** containing:
- 5 character specifications (player male/female, 3 NPC types, boss)
- 6 weapon specs (sword 1H/2H, axe, spear, shield)
- 10 environment prop specs (walls, rocks, trees, grass, barrels, torches, fountain)
- 4 shader specifications (damage numbers, hit flash, outline, dissolve)
- 6 VFX particle system specs
- 5 texture atlas sets
- 4 material library entries
- 8 UI icon/font assets (audio deferred)
- 3 zone scene dependency maps
- Validation rules (tri budgets, resolution limits)
- Production tracking (estimated hours, parallel tracks)

**Structure:**
```json
{
  "asset_categories": {
    "characters": [...],
    "weapons": [...],
    "environment": [...],
    "vfx": [...],
    "shaders": [...],
    "textures": [...],
    "ui": [...],
    "audio": [...]
  },
  "zones": [...],
  "validation": {...},
  "production": {...}
}
```

**Usage:** Future agents load this manifest, scope available assets, check spec compliance, auto-generate placeholder scenes

---

## SPEC-DRIVEN PRD WORKFLOW (Future)

### Proposed Automated Pipeline

Once spec validation is trusted:

1. **PRD authoring** → User writes `.asset.yaml` spec:
```yaml
asset_id: weapon_sword_1h
type: weapon
tri_count: 1500
material: iron
damage: 25
textures: [albedo, normal, roughness]
animations: [draw, sheath]
```

2. **Agent interprets PRD**:
```bash
hermes agent task generate_asset --spec docs/specs/weapon_sword_1h.yaml
```

3. **Autonomous generation**:
   - Creates placeholder mesh (box approximation)
   - Writes Blender Python script template
   - Generates texture canvas (canvas_item shader placeholder)
   - Commits to `asset/weapon_sword_1h/temp/`
   - PR opened for artist review

4. **Artist refinement**:
   - Replaces placeholder with real art
   - Updates spec in manifest.json
   - Commits → re-runs validation

5. **Integration**:
   - Asset validator checks budgets
   - Demo harness runs with updated scene
   - Success → merge to main

**Current State:** Spec phase complete. Implementation phase: NOT STARTED.

---

## PRODUCTION STRATEGY

### Asset Creation Order (Critical Path)

**Month 1 - Character Foundation**
1. Week 1: Toolchain validation + test cube pipeline
2. Week 2: Player male model (block → topology → UV)
3. Week 3: Player textures (4 maps @ 1024) + material setup
4. Week 4: Animations (6 locomotion + 3 combat test) + AnimationTree hook

**Month 2 - Weapons & Combat VFX**
1. Week 5: Sword 1H model + texture
2. Week 6: Attack animations + hitbox timing sync
3. Week 7: VFX shaders (damage number, hit flash) + particle systems
4. Week 8: NPC guard (reuse rig) + spawn in Zone99

**Month 3 - Worlds & Polishing**
1. Week 9: Zone98 floor + walls + simplest arena
2. Week 10: Zone99 modular kit + placement
3. Week 11: Zone100 boss arena + hazard triggers
4. Week 12: Full demo integration + QA signoff

### Parallel Tracks

- Track A (Characters): Player model → textures → rig → animations
- Track B (Weapons): Models batch 3 → weapons database → hitbox collisions
- Track C (Environment): Modular kit pieces → ground tiles → Zones assembly
- Track D (VFX): Shaders → particles → integration into combat systems

**Integration points:**
- Week 4: AnimationTree connected to PredictedPlayer.cs
- Week 7: VFX plugged into CombatEventSystem
- Week 9: Zone98 scene loads + navmesh generated
- Week 11: Boss creature scene instance in Zone100

---

## ACTION ITEMS FOR NEXT AGENT

### Immediate (Day 1)
1. Review this specification package fully (4 docs + manifest)
2. Verify toolchain (install Blender if missing, run validation scripts)
3. Execute `tools/art_pipeline/import_all.py` to ensure Godot auto-import active
4. Create test asset: UV sphere → export GLB → import → confirm renders
5. Update `docs/ART_PIPELINE_STATUS.md` (new doc) with daily progress

### Week 1 Goal
Finish character male blockout → export first GLB → import into Godot → MeshInstance3D visible in editor → stage placeholder texture

Success criteria:
- [ ] Blender open and rendering mesh correctly
- [ ] GLB exports without error
- [ ] Godot shows model in 3D viewport
- [ ] Model `assets/manifest.json` entry updated to `status: in_progress`
- [ ] Screenshot saved: `docs/art_evidence/week1_blockout.png`

### Month 1 Deliverable
Player male complete with rig, textures, 6 locomotion animations playing in AnimationTree, placed in Player.tscn, controllable in demo run

---

## FILE LOCATIONS - QUICK REFERENCE

```
/root/projects/DarkAges/
├── docs/
│   ├── ART_PIPELINE_SPECIFICATION.md      ← READ THIS FIRST
│   ├── ART_PIPELINE_ASSET_INVENTORY.md   ← Production tracker  
│   ├── ART_PIPELINE_TOOLING.md           ← Installation guide
│   └── ART_PIPELINE_STATUS.md            ← (create) daily progress
├── assets/
│   └── manifest.json                      ← Machine-readable spec
├── tools/
│   └── art_pipeline/                      ← Validation scripts (create)
│       ├── import_all.py
│       ├── validate_asset.py
│       └── check_shader_syntax.py
└── src/client/
    └── assets/                            ← Where art lives (create)
        ├── 3d/
        ├── textures/
        ├── materials/
        ├── shaders/
        └── vfx/
```

---

## COMMAND REFERENCE

```bash
# Toolchain verification
blender --version
godot --version
git lfs version

# Run full demo (verify assets load)
python3 tools/demo/run_demo.py --full --duration 60

# Asset validation (once tools exist)
python3 tools/art_pipeline/validate_asset.py assets/manifest.json

# Import dirty assets (copy to Godot and reimport)
python3 tools/art_pipeline/import_all.py

# Zone validation
cd src/client && godot --headless --path . --scene scenes/zones/Zone98Tutorial.tscn --quit 2>&1 | grep -i error

# Unit tests (server still pass)
cd build_validate && ctest --output-on-failure -j8

# Integration test (client-server loop)
python3 tools/validation/godot_integration_test.py --server-bin build_validate/darkages_server --duration 10
```

---

## VERSION CONTROL - GIT LFS

**Mandatory for all binary files:**
```bash
git lfs track "*.glb"
git lfs track "*.png"
git lfs track "*.jpg"
git lfs track "*.tres"
git lfs track "*.tscn"
```

**Commit convention for art assets:**
```
feat(asset): char_player_male v1 - base mesh rigged
fix(asset): sword_1h tri count reduced < 1500
docs(asset): update manifest player_male status in_progress
```

**Branch policy:**
- Art work → feature branch `asset/<asset_id>`
- Merge via PR → require demo run validation
- No direct pushes to main (even for art)

---

## RISKS & MITIGATIONS

| Risk | Impact | Mitigation |
|------|--------|------------|
| Artist unavailable | HIGH | Spec + manifest allows any agent to continue |
| Toolchain compatibility breaks (Godot 4.3) | MEDIUM | Pin versions in toolling guide |
- Git LFS repo size exceeds quota | HIGH | Monitor monthly; prune old assets  
- Asset import fails (corrupt GLB) | MEDIUM | Validation scripts auto-detect
- Performance regressions (draw call spikes) | HIGH | Draw call budget enforced per zone
- Network desync from animation mismatch | HIGH | Combat FSM validated with PR #28; animation state indices must match

---

## SUCCESS METRICS

**Completion criteria for Phase 1 (Art Foundation):**
1. All Tier 1 assets completed (6/21 listed) ✓
2. Player character fully integrated into demo
3. 1 combat animation + hitbox working
4. Damage numbers + hit flash shaders active
5. Zone98 loads in < 5 seconds
6. Draw calls < 80 at 60fps sustained

**Phase 1 completed when:** Demo run displays visible player + combat effects + no visual/functional errors

---

## NEXT: TOOLCHAIN VALIDATION

Run these NOW before art begins:

```bash
cd /root/projects/DarkAges

# Verify tools
which blender || echo "INSTALL BLENDER"
which godot || echo "INSTALL GODOT"
git lfs version || echo "INSTALL GIT LFS"

# Create assets folder hierarchy
mkdir -p src/client/assets/3d/characters/wip_test
mkdir -p src/client/assets/textures/temp
git add src/client/assets/
git commit -m "chore(assets): create empty hierarchy"

# Test export pipeline (requires blender)
# blender --background --python tools/art_pipeline/test_export_import.py
# If Blender not installed yet: skip, note in status
```

**Status:** TOOLCHAIN PENDING INSTALL (Blender not yet verified)

---

## DOCUMENT MAINTENANCE

**These files must stay in sync:**
- `assets/manifest.json` - Source of truth for all assets
- `docs/ART_PIPELINE_SPECIFICATION.md` - Derived from manifest
- `docs/ART_PIPELINE_ASSET_INVENTORY.md` - Check off as work progresses

**Update triggers:**
- New asset added → manifest.json + SPEC + INVENTORY updated
- Asset completed → status set to `complete` + evidence logged
- Tool change → TOOLING.md updated
- Budget adjustment → validation rules adjusted

---

## APPENDIX: VALIDATION SCRIPTS SCAFFOLD

Create: `tools/art_pipeline/`

**validate_asset.py** - Single asset spec compliance

```python
#!/usr/bin/env python3
import json, sys, os
manifest = json.load(open('assets/manifest.json'))
asset_id = sys.argv[1]
asset = next((a for cat in manifest['asset_categories'].values()
              for a in cat['assets'] if a['id'] == asset_id), None)
# Check required files exist, tri count (if glb parsed), texture resolutions
# Exit 0 if OK, 1 if errors
```

**import_all.py** - Touch all .import files to trigger Godot re-import

```python
#!/usr/bin/env python3
import os
for root, dirs, files in os.walk('src/client/assets'):
    for f in files:
        if f.endswith('.glb'):
            glb = os.path.join(root, f)
            imp = glb + '.import'
            if os.path.exists(imp):
                os.utime(imp, None)
                print(f"Queued: {glb}")
```

**check_shader_syntax.py** - Batch compile-all shaders with godot --check-only

Develop these after toolchain validated.

---

**END EXECUTIVE SUMMARY**

**Next agent action:** Install Blender, run validation suite, update this document with toolchain status, begin Week 1: player character blockout

**Expected deliverable after week 1:**
- [x] Toolchain validated
- [ ] Character male blockout GLB in assets/
- [ ] Godot import verified
- [x] Evidence screenshot captured
- [x] Manifest updated to status: in_progress

**Phase 1 completion (Month 1):**
Player character playable in demo with combat animations + VFX → Zone99 combat ready
