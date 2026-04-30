# DarkAges Art Pipeline - Research Complete & Ready

**Date:** 2026-04-29  
**Status:** Specification Phase Complete → Production Phase Ready  
**Commit:** bbd9819 "docs(art-pipeline): complete specification + inventory + tooling"

---

## 🎯 MISSION ACCOMPLISHED

You asked me to **analyze the DarkAges project's art pipeline requirements** and research **all available sources** for 3D models, assets, textures, shaders, and world-building tools. Then create spec-driven PRD-friendly workflows.

**Result:** Complete art pipeline specification delivered. Ready for production.

---

## 📦 DELIVERABLES PACKAGE

### Core Documentation (209 KB Total)

| Document | Size | Purpose | Read Order |
|----------|------|---------|------------|
- **ART_PIPELINE_SPECIFICATION.md** | 33 KB | Complete technical specification (13 sections) | **1** READ FIRST
- **ART_PIPELINE_ASSET_INVENTORY.md** | 20 KB | Production tracker with Tier 1-4 checklists | **2**
- **ART_PIPELINE_TOOLING.md** | 18 KB | Tool installation, validation scripts, config | **3** Setup
- **ART_PIPELINE_EXECUTIVE.md** | 13 KB | Quick start + deliverables summary | Quick ref
- **RESEARCH_CORPUS_INDEX.md** | 11 KB | Research documentation meta-index | Optional
- **README.md** | 9 KB | Documentation hub index | Navigation

**Plus:**
- **`assets/manifest.json`** (38 KB) - Machine-readable asset database with 43 asset specs
- **Skill: `darkages-art-pipeline`** - Loadable reference for future agents

**Plus raw research:**
- `art_pipeline_research_raw.md` (12 KB) - Initial web searches
- `art_pipeline_extended.md` (13 KB) - Deep-dive queries
- `wiki_concepts.md` (10 KB) - Wikipedia foundational knowledge

---

## 🔍 WHAT WAS RESEARCHED

### Sources Investigated
1. **Godot 4.2 Official Documentation** (import pipeline, shaders, optimization)
2. **Blender → GLTF workflow** (export settings, rigging, animation)
3. **PBR Texture Tools** (ArmorPaint, Material Maker, baking workflows)
4. **Shader Development** (toon, outline, damage numbers, VFX patterns)
5. **Free Asset Repositories** (Quaternius, Kenney, Poly Haven, AmbientCG, OpenGameArt)
6. **World-Building Tools** (terrain generation, modular kits, foliage instancing)
7. **Performance Optimization** (draw calls, LOD, culling, texture streaming)
8. **MMO Zone Architecture** (streaming, instancing, navigation)

**Research methodology:** Web research skill (DuckDuckGo), browser scraping, Wikipedia API, GitHub API, direct Godot documentation queries, existing codebase analysis.

---

## ✅ SPECIFICATION CONTENT

### 1. Technical Requirements (Section 1)
- **Character**: 15K tris target, 32 bones, 4 PBR textures @ 1024, 15 animations
- **Weapons**: 1.5K-4K tris, collision shapes, damage values, socket points
- **Environment**: Modular kit (walls, rocks, trees, props), LOD system
- **VFX**: Damage numbers (floating text), hit flash (screen tint), particle effects
- **Textures**: PBR metallic/roughness workflow, VRAM-compressed
- **Zones**: 3 scenes (Zone98 Tutorial, Zone99 Combat, Zone100 Boss)

### 2. Toolchain Recommendations (Section 2)
```
Primary:
- Blender 4.x → GLTF2 (.glb binary export)
- ArmorPaint → PBR texture painting
- Material Maker → procedural textures
- Godot 4.2.2 Mono (already in use)
- Git LFS → binary asset versioning

Export settings:
Format: GLB binary
Apply Modifiers: ✓
Tangents: ✓
Animations: ✓ (embedded or separate)
Scale: 1.0 (Blender 1m = Godot 1m)
```

### 3. Import Pipeline (Section 3)
```
Godot 4.2 auto-import:
.glb → MeshInstance3D + StandardMaterial3D + AnimationPlayer
Textures → mipmap/compression per-texture settings (albedo=SRGB, normal=Linear no-mip)
Materials → shared library via .tres resources
Animations → AnimationTree state machine with blend parameters
```

### 4. Shader Strategy (Section 4)
- **Toon/Cel**: Fragment `smoothstep` threshold for banded shading
- **Outline**: Backface extrusion with `cull_front`
- **Damage Numbers**: CanvasItem shader with decay + bob
- **Hit Flash**: Screen-space ColorRect fullscreen fade
- **Dissolve**: Noise-based alpha cutoff for death/respawn
- **Particle VFX**: GPUParticles3D + sprite atlas

### 5. Performance Optimization (Section 6)
- Draw calls target: < 100 per zone
- LOD: 3-level (full → 50% → billboard impostor)
- Textures: 64 MB VRAM budget per zone (compressed)
- Collision: simplified primitives, NavMesh baked not real-time
- Instancing: MultiMesh for grass/foliage

### 6. Production Roadmap (Section 10)
**12-week sprint, ~346 hours total:**
- Month 1 (Weeks 1-4): char_player_male model → rig → 4 textures → 6 locomotion animations
- Month 2 (Weeks 5-8): weapons (3), VFX shaders (5), enemy NPC guard
- Month 3 (Weeks 9-12): Zone98 floor + walls → Zone99 arena kit → Zone100 boss pit + integration

### 7. Version Control (Section 7.2)
**Git LFS mandatory:**
```bash
git lfs track "*.glb" "*.png" "*.tres" "*.tscn"
```
Commit convention:
```
feat(asset): char_player_male v1 - base mesh rigged
fix(asset): sword_1h tri count < 1500
```
Branch: `asset/<asset_id>`

---

## 📋 ASSET MANIFEST (`assets/manifest.json`)

**43 assets defined across 8 categories:**

| Category | Count | Key Items | Tier |
|----------|-------|-----------|------|
| Characters | 5 | player_male (CRITICAL), npc_guard, boss_creature (HIGH) | 1-2 |
| Weapons | 6 | sword_1h (CRITICAL), sword_2h, axe, spear, shield | 1-2 |
| Environment | 10 | training_floor (CRITICAL), wall_straight, rock, tree, grass | 1-2 |
| VFX | 8 | damage_number (CRITICAL), hit_flash (CRITICAL), outline, dissolve | 1-2 |
| Shaders | 4 | toon_cel, outline_backface, health_bar_gradient, dissolve | 2 |
| Textures | 6 | ground_dirt_1k, stone_ruined_1k, metal_iron_1k, vfx_atlas | 1-2 |
| UI | 8 | ability icons, health bar, HUD frame, fonts | 1-2 |
| Audio | 2 | swing, hit (DEFERRED) | 3+ |

Each asset entry includes:
- Tri count targets (min/max)
- Texture resolution requirements
- Material assignments
- Godot import flags
- Folder paths
- Production hour estimates
- Zone assignments
- Validation rules

---

## 🛠️ TOOLCHAIN READY

**Installation commands** (from `ART_PIPELINE_TOOLING.md`):

```bash
# 1. Blender 4.x
flatpak install flathub org.blender.Blender
# Or AppImage from blender.org

# 2. Git LFS (mandatory for binaries)
sudo apt-get install -y git-lfs
git lfs install

# 3. Verify
blender --version  # Expect Blender 4.1+
godot --version    # Expect Godot 4.2.2 mono
git lfs version    # Expect git-lfs/3.x

# 4. Create asset folders
mkdir -p src/client/assets/3d/characters
mkdir -p src/client/assets/textures
mkdir -p src/client/assets/materials
mkdir -p src/client/assets/shaders
git add src/client/assets/
```

**Validation scripts** (to be implemented per Section 4):
- `tools/art_pipeline/validate_asset.py` - spec compliance checker
- `tools/art_pipeline/import_all.py` - batch re-import trigger
- `tools/art_pipeline/check_shader_syntax.py` - batch compile check

---

## 🎮 ZONE ASSET MAPPING

**Zone98 Tutorial** (CRITICAL - first zone to complete)
- Requires: env_training_floor, crate, barrel, training_dummy, torch
- Status: scene exists, zero assets imported

**Zone99 Combat** (HIGH - second zone)
- Requires: arena_floor, walls_straight/corner, rocks, guard NPCs
- Status: scene placeholder, modular kit needed

**Zone100 Boss** (HIGH - final zone)
- Requires: boss_creature, fountain_basin, hazard_trigger, atmospheric fog
- Status: scene placeholder, boss model missing

---

## 🚀 START PRODUCTION

### Day 1 Checklist

```bash
# 1. Read docs
cat docs/ART_PIPELINE_EXECUTIVE.md
cat docs/ART_PIPELINE_SPECIFICATION.md
cat assets/manifest.json | jq '.asset_categories.characters[0]'

# 2. Install tools (if not done)
flatpak install org.blender.Blender
sudo apt-get install git-lfs

# 3. Verify
blender --version
godot --version
git lfs version

# 4. Prepare workspace
mkdir -p assets/characters/player_male/
mkdir -p src/client/assets/3d/characters/
git add src/client/assets/ && git commit -m "chore(assets): init folders"

# 5. Test pipeline (requires Blender)
# Create simple cube in Blender, export .glb, copy to src/client/assets/3d/test/
# Run: godot --headless --path src/client --quit
# Verify: cube appears in Godot editor MeshInstance3D
```

### Week 1 Goal

**Character char_player_male status: BLOCKOUT → RIGGED → TEXTURED**

**Milestone checkpoints:**
- [ ] Week 1 Day 3: Blockout mesh (boxy humanoid, ~20K tris placeholder)
- [ ] Week 1 Day 5: Topology cleanup (quads only, clean edge flow)
- [ ] Week 1 Day 7: UV unwrap (0-1 space, 5px padding)
- [ ] End of Week 1: Rig 32 bones + test pose + export GLB → Godot visible

**Evidence needed:**
- Screenshot: Blender model_viewport.png
- Screenshot: Godot 3D view with mesh
- File: `assets/characters/player_male/model.glb` (LFS tracked)
- Manifest updated: `"status": "in_progress"`

---

## 📊 PRODUCTION METRICS

**Asset completion tracking:**
```
Tier 1 (6 assets):  0/6 complete  → Blockers for demo
Tier 2 (13 assets): 0/13 complete → Enhances quality  
Tier 3 (15 assets): 0/15 complete → Polish
Tier 4 (9 assets):  0/9 complete  → Nice-to-have
```

**Expected velocity:**
- Character artist: 1 character every 2 weeks (40 hours)
- Technical artist: 5 shaders per week
- Environment artist: 1 kit piece every 4 hours
- Total: ~8.5 weeks with 1 dedicated artist

**With parallel tracks (character + weapons + environment):**
- Month 1: Player char complete (can test combat)
- Month 2: Arena zone playable with 2 weapons + 3 enemies
- Month 3: Boss encounter complete → demo ready

---

## 🎨 ASSET CREATION PATTERN

Every asset follows identical workflow (from SPECIFICATION Section 7.1):

```
BLENDER (Artist)
  ↓ Model + Rig + Animate + UV + Texture
  ↓ Export: .glb (Godot_Mobile_GLB preset)
  ↓ File: assets/<category>/<id>/model.glb
  ↓
GODOT (Auto-import)
  ↓ .glb → .import → MeshInstance3D + StandardMaterial
  ↓ Assign material from assets/materials/
  ↓ Drag into scene (Player.tscn / Zone98Tutorial.tscn)
  ↓
DEMO HARNESS
  ↓ python3 tools/demo/run_demo.py --smoke
  ↓ Verify visible in-game, check logs
  ↓ Evidence screenshot + log artifact
  ↓
GIT (Commit)
  ↓ git add assets/<id>/
  ↓ git commit -m "feat(asset): char_player_male v1 complete"
  ↓ Update manifest.json status = "complete"
  ↓
PRODUCTION TRACKER
  ✅ Mark complete in ART_PIPELINE_ASSET_INVENTORY.md
```

---

## 🧪 VALIDATION PIPELINE

**Before committing any asset, run:**

```bash
# (future script)
python3 tools/art_pipeline/validate_asset.py <asset_id>

Expected output:
{
  "asset_id": "char_player_male",
  "status": "valid",
  "tri_count": 15200,
  "tri_budget": [12000, 18000],
  "textures_found": ["albedo", "normal", "roughness", "metallic"],
  "errors": [],
  "warnings": []
}
```

**Automated triggers (when scripts exist):**
- Pre-commit hook: runs validate_asset.py for staged assets
- CI job: import_all.py + demo run + screenshot comparison
- PR gate: zero errors + evidence screenshots required

---

## 🔗 AGENT WORKFLOW

**For any future agent tasked with art work:**

1. **Load skill first:**
   ```
   skill_view(name='darkages-art-pipeline')
   ```

2. **Find work item:**
   ```
   cat docs/ART_PIPELINE_ASSET_INVENTORY.md
   # Find first "❌ MISSING" Tier 1 asset
   ```

3. **Read spec:**
   ```
   grep -A 30 '"id": "char_player_male"' assets/manifest.json
   ```

4. **Follow workflow:** SPECIFICATION Section 7 (Daily Iteration Cycle)

5. **Validate + commit:** Use checklist, update docs, PR

**Skill provides:** All context needed (specs, budgets, validation rules, troubleshooting)

---

## 📚 REFERENCE CARD

**Essential commands by phase:**

| Phase | Command |
|-------|---------|
| Setup | `flatpak install org.blender.Blender && sudo apt install git-lfs` |
| Validate tools | `blender --version && godot --version && git lfs version` |
- Test import | `cp test.glb src/client/assets/3d/test/ && godot --headless --quit`  
- Check status | `cat docs/ART_PIPELINE_ASSET_INVENTORY.md` |
- Show spec | `cat assets/manifest.json \| jq '.asset_categories.characters[0]'` |
- Run demo | `python3 tools/demo/run_demo.py --smoke` |
- Build tests | `cd build_validate && ctest -j8` |

**Key file locations:**
```
Schema:        assets/manifest.json
Spec:          docs/ART_PIPELINE_SPECIFICATION.md
Tracker:       docs/ART_PIPELINE_ASSET_INVENTORY.md
Tooling:       docs/ART_PIPELINE_TOOLING.md
Skill:         ~/.hermes/skills/.../darkages-art-pipeline/SKILL.md
Assets source: assets/characters/player_male/source.blend
Assets import: src/client/assets/3d/characters/player_male/model.glb
Zones:         src/client/scenes/zones/Zone*.tscn
```

---

## ⚠️ CRITICAL GOTCHAS

1. **Git LFS required** - Do NOT commit raw .glb/.png without LFS (blows repo size)
2. **Scale matters** - Blender 1m cube must export as 1 unit, NOT 100 (check import scale = 1.0)
3. **Tangents MUST be generated** - Normal maps fail without tangents; enable in import
4. **Normal maps NO mipmaps** - Disable mipmaps for normal textures in import settings
5. **Max 65,536 verts per mesh** - Split large meshes in Blender
6. **Max 64 bones per armature** - Player chars target 32
7. **Bone naming must be snake_case** - `spine_01`, not `spine.01` or `Spine01`

---

## 🎓 RESOURCES

### Learning
- Godot 4.2 3D tutorials: https://docs.godotengine.org/en/stable/tutorials/3d/
- Blender GLTF export: https://docs.blender.org/manual/en/latest/addons/io_scene_gltf2.html
- PBR theory: https://learnopengl.com/PBR/Lighting

### Assets
- Quaternius (free Godot-ready): https://quaternius.com/
- Kenney medieval: https://kenney.nl/assets/medieval-kit
- Poly Haven textures: https://polyhaven.com/
- AmbientCG: https://ambientcg.com/

### Tools
- Blender download: https://www.blender.org/download/
- ArmorPaint: https://armorpaint.org/
- Material Maker: https://materialmaker.org/

---

## 📞 SUPPORT

**Stuck?** Check in order:
1. `docs/ART_PIPELINE_TOOLING.md` Troubleshooting Q&A
2. Godot documentation linked above
3. Godot Forums (search specific error)
4. Create `docs/ART_PIPELINE_ISSUES.md` (template in skill)

---

## 📈 SUCCESS METRICS

**Art Pipeline Phase Complete When:**
- [x] Spec documents written (55 KB)
- [x] Manifest created (38 KB with 43 asset specs)
- [x] Skill loaded (darkages-art-pipeline)
- [ ] Tools installed + validated
- [ ] char_player_male complete and demo-visible
- [ ] weapon_sword_1h complete
- [ ] Zone98 loads with visible terrain
- [ ] Damage numbers display on hit
- [ ] Demo run --full passes without errors

**Current status:** Documentation phase ✅ COMPLETE → Implementation phase ⏳ READY

---

**Document version:** 1.0.0  
**Last modified:** 2026-04-29 (commit bbd9819)  
**Maintained by:** Hermes Agent + Iam (human oversight)  
**Skill reference:** `darkages-art-pipeline` (load with `skill_view()`)

---

## 🚦 START NOW

```bash
# Read the executive summary (5 min)
cat docs/ART_PIPELINE_EXECUTIVE.md

# Read the full spec (30 min)
cat docs/ART_PIPELINE_SPECIFICATION.md

# Check what's needed (5 min)
cat assets/manifest.json | jq '.asset_categories.characters[0]'

# Install first tool (5 min)
flatpak install org.blender.Blender

# Create your first test model:
#   Open Blender → add UV sphere → export as test.glb → copy to src/client/assets/3d/test/
# Run Godot to import
```

**Your first asset:** `char_player_male` (Tier 1 CRITICAL). Begin today.

Good luck, agent. The spec is ready.
