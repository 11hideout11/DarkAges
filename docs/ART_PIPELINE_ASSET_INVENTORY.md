# DarkAges MMO - Asset Inventory & Production Tracker

**As of:** 2026-05-02  
**Project Phase:** Phase 10-11 (Security & Chaos Testing) + Art Pipeline Active  
**Status:** Documentation Updated

---

## OVERVIEW

This document is the single source of truth for **what assets exist** and **what still needs to be created** for the DarkAges demo zones to be showcase-ready.

**Inventory Status Legend:**
- ✅ COMPLETE - Asset created, imported, integrated, tested
- 🔄 IN PROGRESS - Work started, not finished
- ❌ MISSING - Not started, required
- ⚠️ PLACEHOLDER - Temporary asset, needs replacement
- ❓ UNKNOWN - Needs investigation

---

## ASSET HIERARCHY BY PRIORITY

### TIER 1: CRITICAL (Blockers for Demo)
- [ ] Character player male (rigged, textured, animated)
- [ ] Character player female (optional for Phase 8, can defer)
- [ ] Basic weapon set (sword 1H, sword 2H)
- [ ] Attack animations matching Combat FSM states
- [ ] Attack trail VFX + hit flash
- [ ] Damage number materials + shader
- [ ] Zone98Tutorial environment (ground, walls, basic lighting)

### TIER 2: HIGH (Enhances Demo Quality)
- [ ] NPC Guard (re-uses player rig/animations, different texture)
- [ ] Combat arena zone (Zone99Combat) - modular kit pieces
- [ ] Health bar UI (TextureProgressBar skins)
- [ ] Target lock indicator shader
- [ ] Boss creature design (25K tris, unique shader FX)

### TIER 3: MEDIUM (Polish)
- [ ] Zone100Boss arena (large circular)
- [ ] Additional weapons (axe, spear, shield)
- [ ] Armor sets (light/heavy variants)
- [ ] Environment props (rocks, trees, grass, barrels)
- [ ] Audio: sword swing, hit, death, ambient zone sounds
- [ ] Respawn effect (shader + particles)

### TIER 4: LOW (Nice-to-have)
- [ ] Hair/Face variations
- [ ] Detailed armor with texture variants
- [ ] Weather effects (rain, fog)
- [ ] Day/night cycle (time of day lighting)
- [ ] Custom death animations per weapon type

---

## MASTER ASSET CHECKLIST

### SECTION 1: PLAYER CHARACTERS

#### 1.1 Base Character Mesh - Male [TIER 1]
- [ ] Model created in Blender
- [ ] Topology: Quads only, clean edge flow
- [ ] Tri count: 12,000 - 18,000
- [ ] UV unwrapped: 0-1 space, padding 5px minimum
- [ ] Rigged: Humanoid armature (32 bones minimum)
- [ ] Skinning: ≤4 bone weights per vertex
- [ ] Exported: model_male.glb
- [ ] Godot import: ✓ (check Output log)
- [ ] Preview in scene: Player.tscn → MeshInstance3D
- [ ] Scale verified: 1 unit = 1 meter
- [ ] File location: `assets/characters/player_male/model.glb`
- [ ] Git LFS: Tracked ✓
- [ ] Status: ❌ NOT STARTED

#### 1.2 Base Character Mesh - Female (Optional/Omitted for Phase 8) [TIER 2]
- [ ] Model created
- [ ] Tri count within budget
- [ ] Same rig (share armature)
- [ ] Textures distinct
- [ ] File: `assets/characters/player_female/model.glb`
- [ ] Status: ⚠️ DEFERRED (use male placeholder)

#### 1.3 Character Textures (PBR) [TIER 1]
For player_male:
- [ ] Albedo map: 1024×1024 PNG sRGB
- [ ] Normal map: 1024×1024 PNG linear
- [ ] Roughness map: 1024×1024 PNG linear
- [ ] Metallic map: 1024×1024 PNG linear
- [ ] AO map: 1024×1024 PNG linear (optional)
- [ ] Hair/skin detail atlas (if separate)
- [ ] File locations: 
  - `assets/characters/player_male/textures/albedo.png`
  - `assets/characters/player_male/textures/normal.png`
  - `assets/characters/player_male/textures/roughness.png`
  - `assets/characters/player_male/textures/metallic.png`
- [ ] Godot material: CharacterMaterial.tres created
- [ ] Textures assigned to StandardMaterial3D
- [ ] Preview lit in-editor (sun + ambient) ✓
- [ ] Status: ❌ NOT STARTED

### SECTION 2: ANIMATIONS

#### 2.1 Locomotion Animations [TIER 1]
All animations must:
- [ ] 30 FPS or 60 FPS native
- [ ] Root motion enabled for movement
- [ ] Looped: idle, walk, run
- [ ] Single-shot: jump_start, jump_loop, jump_land
- [ ] Export format: GLTF embedded OR separate .anim files
- [ ] File: `assets/characters/player_male/animations/<name>.glb` or `.anim`
- [ ] Import to Godot → AnimationPlayer tracks visible
- [ ] AnimationTree connections verified

| Animation | Status | Notes |
|-----------|--------|-------|
| idle      | ❌     | 1.5s loop, neutral stance |
| walk      | ❌     | 0.8s loop, 1.5 m/s velocity |
| run       | ❌     | 0.6s loop, 3.0 m/s velocity |
| jump_start | ❌   | Root motion up trajectory |
| jump_loop | ❌    | Hover loop |
| jump_land | ❌    | Absorb impact |
- [ ] Total animations in set: 6
- [ ] Status: ❌ NONE COMPLETE

#### 2.2 Combat Animations [TIER 1]
Important: Must sync with Combat FSM state durations
- [ ] attack_1h_left - ~0.6s wind-up + follow-through
- [ ] attack_1h_right - ~0.6s  
- [ ] attack_2h - ~1.0s (2-handed weapons)
- [ ] block - holdable, 0.3s entry, 0.2s exit
- [ ] dodge - 0.4s sidestep
- [ ] parry - 0.2s deflection (weapon vs weapon)
- [ ] hit_reaction - stagger backward, ~0.5s
- [ ] death_front - ~2s collapse
- [ ] death_back - ~2s collapse alternate
- [ ] respawn - fade in + get up sequence

Test requirement:
- [ ] Animation state triggers damage phase at correct frame
- [ ] Animation syncs with hitbox activation via AnimationNode
- [ ] Godot AnimationStateMachine.cs transitions verified

| Animation | Frame Window for Hitbox | Status |
|-----------|----------------------|--------|
| attack_1h | frames 15-25 (active) | ❌ |
| attack_2h | frames 20-35 | ❌ |
- [ ] Combat Animations: ❌ NONE

#### 2.3 Creature Animations (Boss) [TIER 2]
- [ ] idle - menacing stance
- [ ] walk - heavy footfalls
- [ ] attack_lunge - long wind-up
- [ ] attack_aoe - ground pound
- [ ] enrage - berserk mode
- [ ] death - dramatic collapse
- [ ] Status: ❌ NOT STARTED

### SECTION 3: WEAPONS

#### 3.1 Weapon Models [TIER 1]

| Weapon | Tris | Materials | Animations | Status |
|--------|------|-----------|------------|--------|
- [ ] Sword 1H (steel) | 1500 | 2 (blade, hilt) | draw, sheathe, drop | ❌
- [ ] Sword 1H (bronze) | 1500 | 2 | - | ❌ skin variant  
- [ ] Sword 2H (claymore) | 3000 | 2 | - | ❌
- [ ] Axe 1H | 1800 | 2 | - | ❌
- [ ] Spear | 2200 | 3 (shaft, head, pommel) | - | ❌
- [ ] Shield | 2000 | 1-2 (wood/metal) | block anim | ❌

**Common requirements:**
- [ ] All weapons: bounding boxes tight
- [ ] Collision shapes added to scenes
- [ ] Socket points: hand_r, hand_l (weapon grip)
- [ ] File: assets/weapons/<name>/model.glb
- [ ] Scene: weapons/<name>.tscn with hitbox
- [ ] Status: ❌ 0/6 COMPLETE

#### 3.2 Weapon Textures & Materials [TIER 1]
- [ ] PBR material library: `assets/materials/weapons.tres`
- [ ] Sword 1H material:
  - Albedo: albedo_sword.png
  - Normal: normal_sword.png
  - Roughness: roughness_sword.png
  - Metallic: metallic_sword.png
- [ ] Weapon shader: metalness workflow verified
- [ ] Status: ❌ NOT STARTED

### SECTION 4: ENVIRONMENT ASSETS

#### 4.1 Terrain & Ground [TIER 1]

**Zone98 Tutorial (flat training ground):**
- [ ] Ground tile: 20m × 20m flat plane
  - Mesh: 4×4 quads, simple UV scaling
  - Material: GroundMat.tres (dirt texture)
  - Texture: dirt_ground_1k.png (albedo + roughness)
  - Collision: StaticBody3D
- [ ] File: assets/environment/ground/training_floor.glb
- [ ] File: assets/environment/ground/training_floor.tscn
- [ ] Status: ❌ NOT STARTED

**Zone99 Combat (arena platform):**
- [ ] Arena floor: circular 30m diameter
- [ ] Raised platform edges (walls)
- [ ] Stairs/ramps ( gentle grade )
- [ ] File: assets/environment/arena/
- [ ] Status: ❌ NOT STARTED

**Zone100 Boss (large circular pit):**
- [ ] Pit geometry (hollow cylinder)
- [ ] Hazard triggers (death volume)
- [ ] File: assets/environment/boss_arena/
- [ ] Status: ❌ NOT STARTED

#### 4.2 Modular Kit Pieces [TIER 2]

Ruined architecture kit:
- [ ] Wall segment 4m × 3m (straight)
- [ ] Wall corner piece (90°)
- [ ] Wall segment with archway
- [ ] Column (broken)
- [ ] Broken wall debris
- [ ] Fountain basin
- [ ] Statue base (headless)
- [ ] Stairs (3-step, 5-step)

**Requirement:** Each piece:
- Tri count: 500-1500
- LOD1 created (50% tris)
- Collision simplified (convex or box)
- Textured: consistent style (stone ruins)
- UV tiling: 2×2 repeat for walls/columns

| Piece | Tris | Status |
|-------|------|--------|
- [ ] wall_straight_4m | 1200 | ❌
- [ ] wall_corner_outer | 900 | ❌  
- [ ] wall_corner_inner | 800 | ❌
- [ ] wall_arch | 1500 | ❌
- [ ] column_broken | 600 | ❌
- [ ] fountain_basin | 2000 | ❌
- [ ] statue_base | 500 | ❌
- [ ] stairs_3 | 400 | ❌
- [ ] stairs_5 | 650 | ❌
- [ ] debris_pile | 300 | ❌

#### 4.3 Environment Props [TIER 2]
- [ ] Barrel (wooden)
  - File: assets/environment/props/barrel.glb
  - Tris: 200
- [ ] Crate (wooden)
  - Tris: 150  
  - Variants: plain, branded
- [ ] Torch (wall-mounted)
  - Tris: 50
  - Includes particle VFX (fire)
- [ ] Banner (hanging)
  - Tris: 100
  - Cloth animation via shader
- [ ] Well structure
  - Tris: 800
  - Contains water plane

**Vegetation:**
- [ ] Grass tuft (six variants)
  - Tris: 50 per tuft
  - Instanced via MultiMesh
- [ ] Tree type A (pine)
  - Tris: 2000 (incl. LODs)
- [ ] Tree type B (deciduous)
  - Tris: 2500
- [ ] Rock formation (large)
  - Tris: 800
  - Texture: moss_cover + normal

**All environment assets:**
- [ ] LOD1 created (50% tris)
- [ ] Collision shape added to scene
- [ ] Material assigned (terrain_mat.tres or variants)

#### 4.4 Terrain Textures [TIER 1]
Base tile textures (1024×1024):
- [ ] Dirt/Ground (albedo + normal + roughness)
- [ ] Stone floor (smooth, worn)
- [ ] Grass/dirt blend (for terrain)
- [ ] Cobblestone (paved areas)
- [ ] Lava/water (for boss arena)

Atlas packing:
- [ ] Warning: max texture size 2048×2048 for VRAM budget
- [ ] Prefer tiled textures over atlas for terrain
- [ ] Detail textures: 256×256 for secondary detail layer

### SECTION 5: VISUAL EFFECTS (VFX)

#### 5.1 Shaders [TIER 1]

**Core shaders directory:** `src/client/shaders/`

| Shader | Purpose | Status | File |
|--------|---------|--------|------|
- [ ] Toon shader | Stylized character lighting | ❌ | `toon.gdshader`
- [ ] Outline shader | Enemy/mob highlights | ❌ | `outline.gdshader`  
- [ ] Damage number float | Floating text bob+fade | ❌ | `damage_number.gdshader`
- [ ] Hit flash tint | Screen flash on hit | ❌ | `hit_flash.gdshader`
- [ ] Crit explosion | Burst particles | ❌ | `crit_explosion.gdshader`
- [ ] Dissolve | Death/respawn fade | ❌ | `dissolve.gdshader`
- [ ] Glow (fresnel) | Halo/aura effects | ❌ | `glow_fresnel.gdshader`
- [ ] Respawn fade-in | Reappear shimmer | ❌ | `respawn.gdshader`

**Shader implementation checklist per shader:**
- [ ] Vertex shader: inputs correct (VERTEX, NORMAL, UV)
- [ ] Fragment shader: outputs ALBEDO, NORMAL (if needed)
- [ ] Uniforms: parameters exposed for material overrides
- [ ] Godot 4.2 syntax validated (compile ✓)
- [ ] Test scene: VFXTest.tscn renders correctly
- [ ] Performance: < 0.5ms per instance

#### 5.2 Particle Systems [TIER 1]

**Particle types:**
- [ ] Damage text spawner - emits text particles upward
- [ ] Sword swing trail - ribbon mesh + particle sparks
- [ ] Hit impact spark - debris burst
- [ ] Critical hit explosion - radial burst + flash
- [ ] Buff/debuff indicator - floating icon above character
- [ ] Death dissolution - particle ash cloud
- [ ] Respawn beam - descending light pillar

**Particle implementation:**
- [ ] Use GPUParticles3D node
- [ ] ProcessMaterial configured (lifetime, emission shape)
- [ ] Texture: particle_atlas.png (sprite sheet with animations)
- [ ] One-shot vs looping: per effect type
- [ ] Instancing enabled for performance
- [ ] Max particles per system: < 100 (tight budget)
- [ ] File: assets/vfx/<effect>/scene.gd

**Particle atlas:** 1024×1024 sheet:
- [ ] spark_8frame
- [ ] explosion_16frame
- [ ] dust_4frame
- [ ] sparkle_6frame
- [ ] Status: ❌ ATLAS NOT CREATED

#### 5.3 Misc Effects [TIER 2-3]
- [ ] Outline post-process (screen-space edge detection)
- [ ] Motion blur (camera-based, subtle)
- [ ] Hit stop (frame freeze on big hit)
- [ ] Screen shake (camera jitter)
- [ ] Footstep dust clouds

### SECTION 6: USER INTERFACE (UI)

#### 6.1 Icons & Sprites [TIER 1]
All icons 64×64 or 128×128 PNG:
- [ ] Health bar frame (left+right caps)
- [ ] Health bar fill (128×16 horizontal gradient)
- [ ] Mana bar fill (blue gradient)
- [ ] Stamina bar fill (green gradient)
- [ ] Ability icons (8 slots) - sword, shield, special abilities
- [ ] Target frame (portrait bg)
- [ ] Debuff icon (default skull)
- [ ] Buff icon (hand/glow)
- [ ] Chat window panel
- [ ] Quest tracker frame
- [ ] Inventory bag icon
- [ ] Mini-map (optional)
- [ ] Mouse cursor (pointer, attack, interact)

**Atlas pack:** `ui/icons_atlas.png` + `ui/icons_atlas.tres`
- [ ] Atlas generated with Godot's AtlasTexture
- [ ] Trim transparent borders
- [ ] Mipmaps enabled
- [ ] Status: ❌ NOT STARTED

#### 6.2 HUD Screens [TIER 1]
- [ ] HealthBarSystem visual theme (Font, colors)
  - Font: `assets/ui/fonts/` (proportional, 16px/24px/32px)
  - Colors: health_gradient.tres
- [ ] Damage number font (impact-style)
- [ ] Screen-edge indicators (ability cooldowns)
- [ ] Combo counter (combo hit display)
- [ ] XP bar (if progression included)
- [ ] Status: ❌ NOT STARTED

#### 6.3 Fonts [TIER 2]
- [ ] DynamicFont resources for Latin/CJK
- [ ] Size variants: 12px (chat), 16px (HUD), 24px (numbers), 48px (titles)
- [ ] Optional: bitmap fonts for performance (BMFont)
- [ ] File: assets/ui/fonts/

### SECTION 7: AUDIO (Future Phase)

- [ ] Sword swing SFX (woosh)
- [ ] Sword hit SFX (clash, flesh)
- [ ] Bow/arrow SFX (if ranged)
- [ ] Footstep grass/stone
- [ ] Ambient zone sound (wind, birds, distant NPC)
- [ ] Combat music (triggered on aggro)
- [ ] Boss music (Phase 2.5)
- [ ] Voice: grunt (hit), death, emote
- [ ] UI click confirm/cancel
- [ ] Status: ⚠️ LATER SPRINT

Audio format: `.ogg` or `.wav` → Godot imports as AudioStream

---

## ZONE ASSET MAPPING

### Zone 98 - Tutorial

| Asset Type | Count | Required | Status |
|-------------|-------|----------|--------|
| Ground tiles | 4 | ✅ | ❌ |
- Wall segments | 6 | ✅ | ❌
- Tutorial props | 5 (dummies) | ✅ | ❌
- Lights | 4 spot + 2 ambient | ✅ | ❌
- NavMesh | 1 | ✅ | ❌
- Total draw calls | < 100 | goal | unknown

**Placeholder status:** Floor only (no geometry), zone loads but empty

### Zone 99 - Combat Arena

| Asset Type | Count | Required | Status |
|-------------|-------|----------|--------|
- Arena floor | 1 large | ✅ | ❌  
- Platform edges | 8 segments | ✅ | ❌
- Cover walls | 10 | ✅ | ❌
- Spectator seating | 3 rows | ❌ polish | ❌
- Lights | 8 spotlights | ✅ | ❌
- NavMesh | 1 | ✅ | ❌
- Total draw calls | < 150 | goal | unknown

### Zone 100 - Boss Arena

| Asset Type | Count | Required | Status |
|-------------|-------|----------|--------|
- Arena pit (circular) | 1 | ✅ | ❌
- Pillars | 4 | ✅ | ❌
- Hazard triggers | 3 | ✅ | ❌
- Boss spawn point | 1 | ✅ | ❌
- Atmospheric fog | 1 volumetric | TIER 2 | ❌
- Lights | 6 dramatic | ✅ | ❌
- NavMesh | 1 | ✅ | ❌

---

## IMPORT & INTEGRATION LOG

### Models Imported
| File | Godot Resource | Tri Count | Mat Count | Import Date | Verified |
|------|----------------|-----------|------------|-------------|----------|
- (none) | | | | | |

### Textures Imported
| File | Size | Format | VRAM (comp) | Assigned To | Import Date |
|------|------|--------|-------------|-------------|-------------|
- (none) | | | | | |

### Shaders Compiled
| Shader | Uniforms | Compile Status | Date | Test Scene |
|--------|----------|----------------|------|------------|
- (none) | | | | |

### Animations Imported
| Animation | Duration | Frames | Bone Count | Source | Date |
|-----------|----------|---------|------------|--------|------|
- (none) | | | | | |

---

## INTEGRATION CHECKLIST

For each asset type, integration involves:
1. **Import:** .glb or texture imported into Godot Asset DB
2. **Scene placement:** Dragged into corresponding .tscn scene  
3. **Material assignment:** If mesh, assign material
4. **Script connections:** If functional (weapon hitbox), attach script
5. **Network sync:** If networked entity, verify spawns
6. **Demo run:** Executes in `run_demo.py --full`
7. **Evidence capture:** Screenshot + log evidence
8. **Documentation:** Asset record updated here

---

## VERSION CONTROL (Git LFS)

**Binary tracking enforced:**
```bash
git lfs track "*.glb"
git lfs track "*.png"  
git lfs track "*.jpg"
git lfs track "*.tres"
git lfs track "*.tscn"
```

**Commit convention:**
```
feat(asset): player male model v1 - rigged, textured

- 15K tris, 32 bones, humanoid rig
- 4 PBR textures @ 1024
- Integrated into Player.tscn test slot
- AnimationTree bindings verified

Asset: char_player_male
```

**Branch policy:**
- Asset work on feature branches: `asset/char-player-male`
- Merge to `main` only after demo validation
- PR requires: screenshot evidence + demo log check

---

## DEMO VALIDATION POINTS

After asset integration, each zone must pass:

**Zone98:**
```bash
python3 tools/demo/run_demo.py --smoke
# Expected: player character visible, terrain renders
# Evidence: screenshot showing character geometry
```

**Zone99:**
```bash
python3 tools/demo/run_demo.py --server-only --duration 30 --npcs 5
# Expected: NPCs spawn, attack animations visible
# Evidence: log shows CombatEvent subtype hits
```

**Zone100:**
```bash
python3 tools/demo/run_demo.py --full --duration 60
# Expected: boss entity spawns, VFX active
# Evidence: screenshot showing boss + damage numbers
```

---

## DELIVERABLES SUMMARY

**For each milestone, update this tally:**

| Category | Total Required | Complete | Remaining | % Complete |
|----------|---------------|----------|-----------|------------|
| Character models | 5 | 0 | 5 | 0% |
| Weapon models | 6 | 0 | 6 | 0% |
- Environment props | 20+ | 0 | 20+ | 0%  
- Textures (sets) | 30+ | 0 | 30+ | 0%
- Animations (clips) | 30+ | 0 | 30+ | 0%
- Shaders | 8 | 0 | 8 | 0%
- Particle systems | 7+ | 0 | 7+ | 0%
- UI icons | 20+ | 0 | 20+ | 0%
- Zone scenes (complete) | 3 | 0 | 3 | 0%

**Current project state: PRE-PRODUCTION ART PHASE - 0% asset complete**

---

## NEXT STEPS (Immediate Actions)

**Day 1-3:** Set up toolchain
- [ ] Install Blender 4.x (or verify existing)
- [ ] Install ArmorPaint (optional but recommended)
- [ ] Install Material Maker (optional)
- [ ] Configure Blender → Godot export preset
- [ ] Test export: `test_cube.glb` → Godot import
- [ ] Validate shader compiler: create `test_shader.gdshader`

**Week 1-2:** Player character
- [ ] Block male character (boxy)
- [ ] Add basic rig (32 bones)
- [ ] UV unwrap (single atlas)
- [ ] Test import → MeshInstance3D
- [ ] Iterate topology → finalize
- [ ] Status: BLOCKOUT PHASE

**Week 3-4:** Character polish + animations
- [ ] Final topology (quads clean)
- [ ] Skin weight painting
- [ ] Texture painting (roughness/metallic pass)
- [ ] Export GLB + materials
- [ ] Import to Godot → StandardMaterial3D
- [ ] Connect to AnimationTree
- [ ] Idle/Walk/Run imported and playing
- [ ] Status: INTEGRATION PHASE

**Deliverable after Week 4:**
- [ ] ART_PIPELINE_FOUNDATION_Complete.md - milestone report
- [ ] test_player_scene.tscn - verifiable scene
- [ ] demo_character_run.json - evidence artifact

---

## APPENDIX: RESOURCE LINKS

### Blender Resources
- Blender 4.x Download: https://www.blender.org/download/
- Auto-Rig Pro: https://blendermarket.com/products/auto-rig-pro
- BlenderToGodot4Pipeline Addon: https://michaeljared.itch.io/blender-to-godot-4-pipeline-addon

### Texture Tools  
- ArmorPaint: https://armorpaint.org/
- Material Maker: https://materialmaker.org/
- AmbientCG (PBR textures): https://ambientcg.com/
- Poly Haven: https://polyhaven.com/

### Godot Resources
- Godot Asset Library (in-editor): Browse Online tab
- Visual Shader examples: https://github.com/godotengine/godot-demo-projects
- 4.2 Shader Language: https://docs.godotengine.org/en/stable/tutorials/shaders/

### Free 3D Models
- Quaternius (Godot ready): https://quaternius.com/
- Kenney.nl: https://kenney.nl/assets
- Itch.io Low Poly Free: https://itch.io/game-assets/free/tag-3d/tag-low-poly

### Paid Models (if needed)
- Unity Asset Store → FBX → Blender → GLB
- Sketchfab: filter game ready + low poly

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-29  
**Next Review:** After Week 1 prototype complete

**Maintainer:** Hermes Agent (autonomous)  
**Reviewers:** Iam (user), Subagent Codex/Claude during art sprints
