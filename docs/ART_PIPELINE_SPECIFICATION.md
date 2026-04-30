# DarkAges MMO - Complete Art Pipeline Specification

**Generated:** 2026-04-29  
**Engine:** Godot 4.2.2 (Forward+ renderer)  
**Client:** C# Mono  
**Server:** C++20 (EnTT ECS)  
**Target:** Third-person MMO with combat FSM, zones, multiplayer

---

## EXECUTIVE SUMMARY

The DarkAges project currently has **zero visual assets** - only architectural scaffolding.
The codebase contains sophisticated systems (Combat FSM, AnimationStateMachine, FootIK, 
TargetLock, etc.) but no 3D models, textures, shaders, or environments.

This specification defines the complete art pipeline to fill the demo zones:
- Zone98Tutorial (combat tutorial)  
- Zone99Combat (arena)
- Zone100Boss (boss fight)

---

## 1. ART ASSET REQUIREMENTS MATRIX

### 1.1 Characters
| Asset | Type | Format | Resolution | Count |
|-------|------|--------|------------|-------|
| Player Male Humanoid | Rigged mesh | GLTF2 | ~15K tris | 1 |
| Player Female Humanoid | Rigged mesh | GLTF2 | ~15K tris | 1 |
| NPC Guard | Rigged mesh | GLTF2 | ~12K tris | 3 variants |
| NPC Villager | Rigged mesh | GLTF2 | ~10K tris | 5 variants |
| Boss Creature | Rigged mesh | GLTF2 | ~25K tris | 1 |

**Rigging requirements:**
- Humanoid bone structure matching Mixamo/Unity Humanoid
- Spine, limbs, fingers, head, jaw
- 30+ bones minimum
- Root motion compatible

### 1.2 Weapons & Props
| Asset | Type | Format | Tris |
|-------|------|--------|------|
| Sword 1H | Static mesh | GLTF2 | 1.5K |
| Sword 2H | Static mesh | GLTF2 | 3K |
| Axe 1H | Static mesh | GLTF2 | 1.8K |
| Spear | Static mesh | GLTF2 | 2.2K |
| Shield | Static mesh | GLTF2 | 2K |
| Armor Set (Light) | Static mesh | GLTF2 | 8K |
| Armor Set (Heavy) | Static mesh | GLTF2 | 12K |

### 1.3 Environment Assets
| Asset | Type | Format |
|-------|------|--------|
| Ground tiles (3 variants) | Mesh + texture | GLTF2 |
| Rock formations (10) | Mesh | GLTF2 |
| Trees (5 types) | Mesh + LOD | GLTF2 |
| Grass/Foliage | Instanced mesh | GLTF2 |
| Ruined walls/columns | Mesh | GLTF2 |
| Props (barrels, crates, torches) | Mesh | GLTF2 |

### 1.4 Visual Effects (VFX)
| Asset | Type | Implementation |
|-------|------|---------------|
| Damage number materials | Shader/GDShader | Floating text  
| Hit marker | Shader | Screen-space outline/flash
| Health bar gradient | Texture | Atlas frame  
| Attack trail | Particle + shader | Mesh trail  
| Crit indicator | Particle | Explosion burst  
| Respawn effect | Shader + particles | Fade in glow  
| Buff/debuff icons | 2D texture | UI sprite atlas  
| Spell casting halo | Shader | Ring projectile  

### 1.5 Textures
**PBR workflow (metal/roughness):**
- Albedo (sRGB) - 1024x1024 or 2048 max
- Normal (linear) - same resolution  
- Roughness (linear) - 2048
- Metallic (linear) - 2048
- AO (linear) - 2048
- Height (optional) - 1024

**Texture Atlas packing** for UI and small props.

---

## 2. RECOMMENDED TOOLCHAIN

### 2.1 3D Modeling - Godot 4.2 Compatible Pipeline

#### Primary: Blender 4.x (Free, Open Source)
- **Format:** Export → GLTF2 (binary .glb preferred)
- **Scale:** 1 Blender unit = 1 Godot meter (no conversion needed)
- **Forward:** +Y (Blender default) → Godot handles automatically via GLTF
- **Up axis:** Y (matches Godot)
- **Transform:** Apply all transforms before export
- **Vertex colors:** Supported
- **UV maps:** Max 4 UV sets (UV1 for lightmap, UV0 for diffuse)
- **Bone naming:** Snake_case (e.g., spine_01, hand_r)

**Blender→Godot Export Settings (GLTF2):**
- Format: .glb (binary) - single file, smaller
- Include: Selected Objects / Limit to Scene hierarchy as needed
- Apply Modifiers: ✓
- UVs: ✓
- Normals: ✓
- Tangents: ✓ (required for normal maps)
- Vertex Colors: ✓ (if used)
- Materials: Export
- Images: Embed (for single-file convenience) OR Copy to --export-dir
- Animation: Include (if animations baked to armature)

**Recommended Blender Add-ons for Godot:**
- BlenderToGodot4Pipeline (Michael Jared) - automated setup, naming conventions
- Godot Asset Browser - browse Godot assets from Blender
- Auto-Rig Pro - biped rigging, weight painting, retargeting
- Mesh Analyzer - check triangle count, edge flow

#### Secondary: Maya/3ds Max (Paid - License Already Available?)
If licensed:
- FBX 2020 (binary) → Godot supports FBX well
- Up axis: Y
- Scale: 1 unit = 1 cm → set scale factor 100 in import
- Triangulate mesh on export

### 2.2 Texture Creation & Baking

#### Primary Tools:

**ArmorPaint** (Free, Open Source)
- PBR texture painting directly on 3D mesh
- Supports: Color, Roughness, Metallic, Normal, Height, AO
- Layered workflow like Photoshop
- Real-time PBR viewport
- Export to PNG/TGA

**Material Maker** (Free)
- Procedural material generation
- Node-based shader-like texture creation
- Can bake from high-poly → low-poly
- Export PBR maps

**Blender (Cycles/Eevee)**
- Bake from high-poly sculpt → low-poly game mesh
- AO bake, normal map (33px/texel), curvature, height
- Texture atlasing via UV stitching

#### Secondary: Photoshop/GIMP
- Texture atlasing, touchups, UI icons
- PNG/TGA output (no JPEG - artifacts on normals)

**Texture Baking Workflow:**
1. Sculpt high-poly in Blender
2. Retopo low-poly (clean quad topology, <15K tris)
3. UV unwrap low-poly with proper padding (3-5 pixels)
4. Bake: Normal, AO, Curvature, Height (2048 res)
5. Paint color/roughness/metallic in ArmorPaint
6. Export all maps as PNG with sRGB/linear flags

### 2.3 Shader Development

#### Godot 4.2 Shader Language (GLSL ES 3.0 variant)

**Two approaches:**
1. VisualShader Node Editor - Node-based, good for simple materials
2. Text Shaders - Full code control, better for complex effects

**Shader Types:**
- ShaderMaterial - custom shader for mesh
- CanvasItemMaterial - 2D/UI shaders
- ParticleProcessMaterial - particle behavior

**Common Shader Patterns for DarkAges:**

**A. Toon/Cel Shading** (for stylized characters):
```glsl
shader_type spatial;
render_mode unshaded, cull_disabled;

void fragment() {
    float NdotL = dot(NORMAL, LIGHT);
    float toon = smoothstep(0.2, 0.25, NdotL);
    ALBEDO = toon * BASE_COLOR;
}
```

**B. Outlined Characters** (for enemies, boss):
- Method 1: Backface scaling in vertex shader
- Method 2: Post-process outline via CanvasItem with inverted depth

**C. Hit Marker / Flash** (combat feedback):
- Screen-space overlay shader
- Flash red tint, draw crosshair-like indicator
- Alternative: Flash material albedo white then decay

**D. Damage Numbers**:
- Use Label3D with Billboard mode
- Shader for text glow/fade

**E. VFX Shaders:**
- Dissolve (noise-based alpha cutoff)
- Trail (fade over time + ribbon mesh)
- Explosion (vertex displacement + particle)

**Shader Resources to save:**
- /root/projects/DarkAges/shaders/ directory
- Import as .gdshader or .gdshaderinc
- Reference in material overrides

### 2.4 Environment & World Building

#### Terrain:
Godot 4.2 MeshInstance3D with heightmap
- Heightmap resolution: 256x256 → 1M tris (manageable)
- Split into tiles (4x4 or 8x8) for LOD + culling
- Use HeightMapShape3D for collision
- Texture: tiled detail textures + splat map

**Tools:**
- Gaea (paid) / L3DT (paid) / World Machine (paid)
- **Free alternative: Blender Landscape Add-on** (ant landscape, A.N.T.)
- Export heightmap as 16-bit PNG
- Import to Godot via HeightMapShape3D or procedural mesh generator

#### Vegetation & Foliage:
- Grass: MultiMeshInstance3D with random rotation/scale
- Trees: MeshInstance3D + LOD (3 levels)
- Foliage: GPU instances, billboard impostors at distance

**Foliage workflow:**
1. Create mesh in Blender (billboard planes for distant)
2. Place with MultiMesh using random transform
3. Use VisibilityNotifier for culling (LOD based on camera distance)

#### Zone Architecture:
DarkAges zones are .tscn packed scenes:
- Zone98Tutorial.tscn
- Zone99Combat.tscn  
- Zone100Boss.tscn

Each zone contains:
- NavMesh for AI pathfinding
- NavigationRegion3D for static walkable surfaces
- StaticBody3D collision for world geometry
- PointLight/SpotLight for atmosphere
- Area3D trigger volumes for events

**Best practice:**
- Build zone geometry as separate .glb models (modular)
- Pack into scene with PackedScene → instance
- Use SceneUniqueNODE for shared resources to reduce memory

### 2.5 Animation Pipeline

#### Format: GLTF2 embedded animations OR separate .anim tracks

**Animation Types needed:**
1. **Locomotion:** idle, walk, run, jump, fall
2. **Combat:** attack_1H, attack_2H, block, dodge, parry, hit_reaction, death  
3. **Interaction:** talk, pickup, emote
4. **Creature:** creature-specific (boss behaviors)

**AnimationTree in Godot 4.2** (state machine):
```
State Machine (player)
├── Idle
├── Walk
├── Run
├── Jump
├── Attack
├── Block
└── Hurt

BlendSpaces (2D)
├── MovementBlend: (idle←→walk←→run) on speed axis
└── DirectionBlend: forward/back/left/right blend

Transitions:
AnyState → Hurt (on hit)
Attack → Idle (timeout)
```

**FootIK:** Use FootIKController.cs - aligns feet to terrain slope

**Animation blending:**
- Use AnimationNodeBlendTree
- Crossfade time: 0.1s for combat transitions
- 0.3s for locomotion

### 2.6 Asset Organization

```
DarkAges/
├── assets/              # All 3D art assets
│   ├── characters/      # Player, NPCs, enemies
│   │   ├── player_male/
│   │   │   ├── model.glb
│   │   │   ├── textures/
│   │   │   │   ├── albedo.png
│   │   │   │   ├── normal.png
│   │   │   │   ├── roughness.png
│   │   │   │   └── metallic.png
│   │   │   ├── rig/
│   │   │   │   └── armature.glb (for retargeting)
│   │   │   ├── animations/
│   │   │   │   ├── idle.anim
│   │   │   │   ├── walk.anim
│   │   │   │   ├── attack_1h.anim
│   │   │   │   └── ...
│   │   │   └── preview/
│   │   │       └── screenshot.png
│   │   └── npc_guard/...
│   │
│   ├── weapons/
│   │   ├── sword_1h/
│   │   │   ├── model.glb
│   │   │   ├── material.tres
│   │   │   └── textures/
│   │   └── ...
│   │
│   ├── environment/
│   │   ├── terrain/
│   │   │   ├── heightmap_zone98.png
│   │   │   ├── textures/
│   │   │   └── models/
│   │   ├── rocks/
│   │   ├── trees/
│   │   ├── grass/
│   │   └── props/
│   │
│   ├── vfx/                # Particle effects
│   │   ├── damage_number/
│   │   │   ├── shader.gdshader
│   │   │   └── gradient.png
│   │   └── hit_flash/
│   │
│   ├── ui/               # 2D assets (PNG, SVG)
│   │   ├── icons/
│   │   ├── hud/
│   │   └── fonts/
│   │
│   └── shaders/          # Godot shader files
│       ├── toon.gdshader
│       ├── outline.gdshader
│       ├── dissolve.gdshader
│       └── hit_flash.gdshader
│
├── src/client/scenes/
│   ├── Player.tscn       # References assets via ExtResource
│   ├── RemotePlayer.tscn
│   ├── PlayerAnimations.tres
│   ├── PlayerAnimationTree.tres
│   └── zones/
│       ├── Zone98Tutorial.tscn
│       ├── Zone99Combat.tscn
│       └── Zone100Boss.tscn
│
└── docs/
    ├── ART_PIPELINE_SPECIFICATION.md      <- THIS DOC
    ├── ART_PIPELINE_ASSET_INVENTORY.md     # Checklist of required assets
    ├── ART_PIPELINE_WORKFLOW.md            # Step-by-step production
    └── ART_PIPELINE_TOOLING.md             # Tool installation & config
```

---

## 3. IMPORT PIPELINE - FROM SOURCE TO GODOT

### 3.1 Folder Structure

```
src/client/assets/
├── 3d/
│   ├── characters/
│   ├── weapons/
│   ├── environment/
│   └── vfx/
├── textures/
│   ├── characters/
│   ├── ui/
│   └── [cubemaps, gradients]
├── materials/
│   ├── character_mats.tres
│   ├── environment_mats.tres
│   └── vfx_mats.tres
└── shaders/
```

Godot auto-imports on folder refresh.

### 3.2 GLTF Import Settings (Godot 4.2)

When .glb is detected in project:
1. Godot creates .import folder entry
2. Meshes → MeshLibrary or ArrayMesh
3. Materials created as StandardMaterial3D nodes
4. Animations extracted as Animation resources

**Customize via:**
project://internal/editor/ → Import settings → 3D scene

**Override on a per-asset basis:**
- Select .glb in FileSystem dock
- Import tab:
  - Meshes: Generate LODs (checkbox) ✓
  - Meshes: Ensure Tangents (for normal mapping) ✓  
  - Meshes: Create Shadow Meshes (optional) ✓ for static props
  - Animations: Import as AnimationPlayer track in scene

### 3.3 Texture Import Settings

| Texture Type | sRGB | Filter | Mipmaps | Mipmap Bias |
|--------------|------|--------|---------|-------------|
| Albedo       | YES  | Bilinear | YES  | 0 (default) |
| Normal       | NO   | Linear  | NO   | - (dont mip) |
| Roughness    | NO   | Linear  | YES  | 0 |
| Metallic     | NO   | Linear  | YES  | 0 |
| AO           | NO   | Linear  | YES  | 0 |
| Height       | NO   | Linear  | YES  | 0 |

**Compression:**
- VRAM: ETC2 (Android) / ASTC (iOS/Modern) / BC7 (Desktop)
- Lossless: PNG for source, compressed at import
- Max size: 2048×2048 (1024 for small props) → texture atlas for LOTS of small items

### 3.4 Material Asset Strategy

**Option A: Per-object materials** (simple, wasteful)
- Each mesh has its own StandardMaterial3D
- Direct texture references

**Option B: Material Library** (Recommended)
- Single MaterialLibrary.tres resource
- Shared materials: CharacterSkin, MetalWeapon, LeatherArmor
- Use atlases to vary appearance with vertex color or UV offset

**PBR workflow in Godot 4.2:**
- Use StandardMaterial3D
- Channels: albedo_texture, normal_texture, roughness_texture, metallic_texture
- OR single texture with packed channels:
  - RG = roughness + metallic (ORM style)
  - B = AO (optional)
- Godot 4.2 StandardMaterial uses separate channels OR ORM texture

### 3.5 Animation Import Pipeline

1. In Blender: Armature + actions → export GLTF with animations
2. In Godot: .glb imports animations as Animation resources
3. Import Settings: Scene → Import → 3D Animation → Import as AnimationPlayer + Keep Enabled
4. AnimationTree: Create AnimationTree node, set AnimationPlayer as source
5. State Machine: Configure states + blend parameters

**Animation blending parameters:**
- movement_speed (float 0.0 - 1.0) → blends idle→walk→run
- attack_phase (bool true/false) → transitions into attack state
- is_airborne (bool) → jump/fall animation switch
- is_dead (bool) → death/ragdoll

---

## 4. GAME-SPECIFIC ASSET REQUIREMENTS

### 4.1 Character Player Assets

**Base Mesh (Male/Female):**
- Tri count: 12,000 – 18,000 tris (Godot optimized, low-poly)
- Bones: 32 (spine_01-4, head, neck, clavicle_r/l, upperarm_r/l, 
           forearm_r/l, hand_r/l, thigh_r/l, calf_r/l, foot_r/l, toebase_r/l)
- Skinning: ≤4 bone influences per vertex
- Shape keys (blend shapes): for facial expressions if needed

**Texture Resolution:** 1024×1024 (albedo, normal, roughness, metallic)
**Texture Atlas:** Pack face/hair detail into albedo + normal atlas

**Rigging workflow:**
1. Create armature in Blender (humanoid)
2. Parent mesh → Automatic Weights
3. Weight paint (clean, no bone bleeding)
4. Test deformation (arm swing, leg lift)
5. Export .glb + test animation in Godot (AnimationPlayer)

### 4.2 Weapon Models

**Sword 1H (basic):**
- Tri count: 1,500
- Materials: 1 (metal) or 2 (blade + hilt)
- Animations: Drop on ground, sheath, draw
- Socket points: attach_point_hand_r for grip

**Implementation:**
- Weapon scene: Sword1H.tscn → MeshInstance3D + CollisionShape3D (hitbox)
- Attach to player via hand_r bone (using Skin or Skeleton)

### 4.3 Enemy AI Models

**NPC Guard:**
- Tri count: 10,000 (simplified vs player)
- Same rig → can use player animations
- Material variants via color/texture atlas

**Boss Creature:**
- Tri count: 25,000 (higher fidelity)
- Custom rig (quadruped or monstrous biped)
- Scaling factor: 2.2× player height
- Unique shader effects (glowing eyes, fire aura)

### 4.4 Zone Environment Assets

**Zone98 - Tutorial:**
- Flat arena with training dummies
- Clear sightlines, minimal occlusion
- Tutorial signage (billboard text)

**Zone99 - Combat Arena:**
- Elevated platforms for verticality
- Cover objects (barriers)
- Spectator seating

**Zone100 - Boss:**
- Large circular arena (50m radius)
- Environmental hazards (lava pits, traps)
- Boss summoning circles (decal)

**Modular kit:** 
Create kitbash parts: 1x1, 2x1, 2x2, 4x4 wall tiles; floors; arches
Assemble zones from these pieces for variation.

---

## 5. VISUAL EFFECTS (VFX) STRATEGY

### 5.1 Combat Feedback System

**Hit Markers:**
Displayed as UI overlay or on-target effect:
- 2D crosshair flashes briefly
- Number floats upward from target (using Label3D Billboard)
- Color: white (normal), orange (crit), red (heavy)

**Damage Numbers:**
- Label3D with Billboard mode
- Rise + fade shader (tween Y position + modulate alpha)
- Initial font size: 48pt
- Screen space OR world space (world space preferred for MMO)

**Health Bar:**
- TextureProgressBar with custom textures:
  - bar_bg.png (frame)
  - bar_fill.png (horizontal gradient green→red)
- Update via HealthBarSystem.cs

**Attack Trails:**
- TrailRenderer3D node (Godot 4.2)
- Generate ribbon mesh along weapon path
- Shader: fade out along trail distance

**Particle Effects:**
- Use GPUParticles3D (GPU-accelerated)
- Process material sets: color ramp, lifetime, emission shape
- Point light attached for glow

**Shader-based VFX:**
- Dissolve: noise threshold for fade-out
- Glow: emissive + radial gradient fresnel
- Outline: invert depth buffer or backface expansion

---

## 6. PERFORMANCE OPTIMIZATION

### 6.1 Draw Calls & Batching

**Static scene elements:**
- Merge static meshes into single MultiMesh or MeshInstance3D with merged geometry
- Use VisibilityNotifier to cull entire groups
- Batch by material (reduces setTexture calls)

**Target: < 100 draw calls per zone at 60fps**

### 6.2 LOD System

**3-Level LOD:**
1. LOD0 (0-20m): Full mesh, full textures, all shaders
2. LOD1 (20-50m): 50% triangles, 1 texture resolution halved  
3. LOD2 (50+m): Impostor (billboard sprite) OR simple cardboard proxy

**Implementation:**
- MeshInstance3D.lod_max_distance per LOD
- MeshInstance3D.lod_margin for smooth transition
- Automation: Blender's MeshLOD addon or manual decimation

### 6.3 Texture Streaming

**Active set per zone:** 64 MB VRAM target
- 1024×1024 RGBA = 4 MB per texture uncompressed
- With ASTC compression: ~0.5 MB typical

**Solutions:**
- Texture atlasing reduces texture bind calls
- Mipmaps enabled for all textures
- Use anisotropic filtering for terrain close-up

### 6.4 Collision Optimization

- Use simplified collision shapes (primitive boxes, capsules) for dynamic bodies
- StaticBody3D: convex hull or manual trimesh if needed
- NavMesh baked per zone (not real-time)
- Avoid MeshCollision on high-poly meshes

---

## 7. ASSET WORKFLOW PIPELINE

### 7.1 Daily Iteration Cycle

```
Artist workflow:
1. Blender: Model → Rig → Animate → Export .glb
2. Export to: /assets/characters/<name>/model.glb
3. Godot: Auto-import (.glb → .glb.import) - or manual Import → Reimport
4. Assign material: Drag materials from /assets/materials/
5. Test in scene: drag into Player.tscn test slot
6. Deploy: Run demo harness → observe in-game
7. Iterate: return to Blender
```

### 7.2 Version Control

**Git LFS required:**
All binary assets → .gitattributes
```
*.glb filter=lfs diff=lfs merge=lfs -text
*.png filter=lfs diff=lfs merge=lfs -text
*.jpg filter=lfs diff=lfs merge=lfs -text
*.tres filter=lfs diff=lfs merge=lfs -text
*.tscn filter=lfs diff=lfs merge=lfs -text
```

.gitattributes entry at repo root.

**Estimated LFS storage per asset:**
- GLB (character): 3-8 MB
- Texture set (4 maps @ 1024): 8 MB
- Audio: 2 MB per file

**Repo size growth:**
- Initial assets: ~200 files → 1.2 GB LFS
- After 6 months: ~500 files → 3 GB LFS

**Git LFS quota:** Monitor usage, Gitlab/GitHub LFS limits (1GB free storage, 1GB traffic/month on GitHub). Self-hosted LFS on internal infra if needed.

### 7.3 Asset Review & QA

**Checklist before commit:**
- Tri count within budget? (Mesh → Statistics in Blender)
- UV padding? (min 3-5 pixels, no overlap)
- Shading smooth/flat groups correct? (hard edges on armor plates)
- Materials assigned?  
- Export settings validated? (Apply transforms, triangulate)
- Imports clean in Godot? (No errors in Output → Import)
- Scales in-editor correct? (1 Godot unit = 1 meter)

---

## 8. FREE & PAID ASSET SOURCES

### 8.1 FREE ASSETS (CC0 / Royalty-Free)

#### Characters:
**Quaternius** - godot compatible, CC0
- Skeletal characters, weapons, props
- https://quaternius.com/

**Kenney.nl** - Medieval pack
- Low-poly character armor: CC0
- https://kenney.nl/assets/medieval-kit

**OpenGameArt.org** - search low poly + rpg
- Quality varies, check licenses

**Mixamo** (Adobe) - free animations only
- Biped rigging & animations (FBX)
- Character mesh not free, but rig/hierarchy exportable

**Itch.io Game Assets** (free tag)
- https://itch.io/game-assets/free/tag-3d/tag-low-poly
- Filter by Godot / GLTF format

#### Environment:
**Poly Haven** - PBR textures (CC0)
- https://polyhaven.com/
- Textures: 8K HDRIs, 4K PBR maps (albedo, roughness, normal, displacement)
- Free, no attribution required

**AmbientCG** (formerly CC0 Textures)  
- https://ambientcg.com/
- Material library, Texas, downloadable packs
- PBR maps, 2K/4K/8K

**Godot Asset Library** (built-in AssetLib in editor → Online tab)
- Search: terrain, environment, low-poly
- Import directly via Godot

### 8.2 PAID ASSETS (Marketplace)

**Unity Asset Store** → Can be converted to Godot:
- Many assets are .fbx → import to Blender → export GLTF
- Filter: Low Poly, RPG, Fantasy

**Unreal Marketplace** → Same as above
- Look for .fbx packages

**Sketchfab:**
- CC Attribution or commercial licenses
- Filter: Game Ready + Low Poly
- Download: FBX/GLTF

**ArtStation Marketplace:**
- Professional artists, higher price point
- Custom commissions possible

**CGTrader:**
- Wide selection, quality varies

### 8.3 CUSTOM COMMISSIONS

If stock assets insufficient:
- Hire on: Fiverr, Upwork, ArtStation Jobs
- Cost: $50-200 per character (low-poly)  
- Cost: $300-800 for rigged animated character (full RPG set)

---

## 9. CRITICAL TECHNICAL REQUIREMENTS

### 9.1 Vertex Format (Godot 4.2 Limits)

- Max attributes per vertex: 8 (position, normal, tangent, uv0, uv1, color, bones, weights)
- Max bones per vertex: 4
- Max vertex/index buffer: 2^16 (65536) per mesh → need to split large meshes
- Mesh size cache: fully transform ~100K verts acceptable per frame

### 9.2 Animation Constraints

- Max 64 bones per armature (Godot 4.2 limitation)
- Animation tracks: position, rotation (quat), scale
- Root motion: enabled on animations for character movement
- Sampling rate: 30 FPS or 60 FPS (import at native, playback uses interpolation)

### 9.3 Shader Complexity Budget

**Forward+ renderer:**
- 1 main directional light + up to 65535 omni/spot lights in cluster
- Pixel shader instruction count: < 256 ops recommended for mobile compatibility
- Vertex shader: < 128 ops

**MMO with 20+ players on screen:**
- Each player character: 1 draw call (skinned mesh) + 1 shadow cascade
- Simplified material: single albedo+normal texture, no parallax
- Shadow distance: 50m on players, world shadow at 100m

### 9.4 Network Synchronization of Assets

- Reference asset paths by resource name (not file path)  
- Both client and server must have same assets installed
- Server authoritative: validates hitbox collisions, not visual fidelity

---

## 10. PRODUCTION ROADMAP (3-Month Sprint)

### Month 1: Foundation Assets
**Week 1:** Modeling setup + Blender→Godot pipeline validated
- Install tools, configure export preset, create test model
- Validate: test_model.glb imports correctly, textures show

**Week 2:** Character player model (male base mesh)
- Block out, topology clean, UV unwrap
- Rig with Auto-Rig Pro
- Test animation bind-pose in Godot

**Week 3:** Character textures + material test
- Bake test normal map
- Paint albedo/roughness in ArmorPaint
- Create StandardMaterial3D in Godot, test in lit scene

**Week 4:** Basic locomotion animations
- idle, walk, run, jump (Mixamo or hand-keyed)
- Import to Godot, hook into AnimationTree
- Verify PredictedPlayer.cs reads AnimationTree parameters

### Month 2: Weapon & Combat Assets
**Week 5:** Weapon models (sword, axe, spear)
- Low-poly variants (1500-2500 tris)
- Texture variants (steel, bronze, rusty)

**Week 6:** Attack animations + hitbox integration
- Attack_1H, Attack_2H, block, dodge
- Configure CombatStateMachine state transitions
- Test hitbox activation timing matches animation frames

**Week 7:** VFX Shaders + Particle systems
- Damage number shader + floatingLabel system
- Hit flash (screen tint + weapon flash)
- Attack trail particle emitter
- Crit explosion

**Week 8:** Enemy models
- NPC Guard (uses player rig/animations - cheap approach)
- Test in Zone99Combat.tscn

### Month 3: World & Polishing
**Week 9:** Zone98 Tutorial environment
- Ground mesh + rocks
- Basic buildings (collapsed walls, huts)
- Place NavMesh, navigation regions

**Week 10:** Zone99 Combat Arena  
- Arena floor + platforms
- Wall segments, cover objects
- Lighting: point lights + ambient

**Week 11:** Zone100 Boss arena
- Large circular pit
- Hazard triggers (lava, traps)
- Atmospheric fog/volumetric lighting

**Week 12:** Final integration + QA
- All assets in all zones
- Performance profiling (draw calls, fps)
- Final demo run with run_demo.py --full
- Asset inventory audit for documentation

---

## 11. MAINTENANCE & EXTENSION

### 11.1 Adding New Assets

**For a new weapon:**
1. Model in Blender → assets/weapons/dragon_sword/model.glb
2. Export textures → assets/weapons/dragon_sword/textures/
3. Godot auto-import → create DragonSword.tscn scene
4. Add to WeaponDatabase.tres manifest file
5. Test: equip → verify renders, collision, attack animation sync

**For a new character:**
1. Model + rig (humanoid or custom)
2. Attach animations (or reuse generic set)
3. Create MyEnemy.tscn with RemotePlayer script swap
4. Add spawn configuration to zone scene

### 11.2 Material Variants

Use Material Overrides + Texture Atlases instead of duplicate materials.
For armor variants:
- Create base ArmorMat.tres
- Create ArmorMat_Red.tres → override albedo, metallic
- Alternative: vertex color tinting in shader

### 11.3 Asset Versioning

When updating an existing asset:
1. Increment version in filename: model_v2.glb
2. Keep old version until all scenes reference new version
3. Audit: grep -r "model_v1.glb" src/client/
4. Delete old files after migration

---

## 12. TROUBLESHOOTING

### 12.1 Asset Import Fails

**Symptom:** Godot shows Import failed with red icon  
**Cause:** Mesh exceeds vertex limit (>65k verts), UVs overlapping, non-manifold geometry  
**Fix:** Remesh in Blender (Decimate modifier), re-UV, apply transforms  

**Symptom:** Normal maps appear flat/wrong  
**Cause:** Tangents not generated (smooth shading vs flat shading edge split)  
**Fix:** In Blender: Edge Split modifier before export; in Godot: enable Generate Tangents in import settings

**Symptom:** Textures pink/missing  
**Cause:** Texture path invalid after folder move  
**Fix:** Re-assign texture in Material, save material as external resource

**Symptom:** Animation jittery or bone flailing  
**Cause:** Incorrect bone hierarchy (non-standard naming)  
**Fix:** Rename bones to skeleton_bone_XX pattern, ensure proper parenting (Hips → Spine → ... → Head)

### 12.2 Performance Problems

**Low FPS in combat:**
1. Profile: Performance tab in Godot debugger
2. Check: Frame Time → Render too high? → too many draw calls
3. Fix: Mesh combining, enable occlusion culling, reduce lights

**Crashes on zone load:**
- Memory spike from textures: compress to ASTC/ETC2
- Asset leak: not freeing previous zone scene

### 12.3 Material Mismatch (Client-Server)

Both sides must agree on:
- Hitbox shapes/positions (server-side only, not visual)
- Damage values (UI mockup), real damage sent from server
- Animation state indices must match network protocol values

---

## 13. SPECIFICATION TOOLS FOR FUTURE AGENTS

### 13.1 Asset Inventory Database

Create assets/inventory.json to track every asset:
```json
{
  "assets": [
    {
      "id": "char_player_male", 
      "type": "character",
      "path": "assets/characters/player_male/model.glb",
      "tri_count": 14500,
      "bones": 32,
      "textures": ["albedo_1024", "normal_1024", "roughness_1024"],
      "animations": ["idle", "walk", "run", "jump", "attack_1h"],
      "status": "completed",
      "zone_usage": ["Zone98", "Zone99", "Zone100"]
    }
  ]
}
```

### 13.2 Spec-Driven Generation

Future PRD workflows:
- Each asset defined by .asset.yaml manifest (one per asset)
- Spec includes: tri budget, texture res, rig requirements, required animations
- Checklist: ✓ model ✓ rig ✓ textures ✓ animations ✓ import ✓ integration test
- Automated verification: python tools/validate_asset.py assets/characters/player_male/

### 13.3 Automation Scripts

**Asset Packer** - collects all asset files for a build:
```bash
./tools/asset_packer.py --export --format glb --output build/assets/
```

**Spec Compliance Checker:**
```bash
./tools/check_asset_spec.py --all_assets  # validates budgets, required files
```

**Import Batch Validator:**
```bash
./tools/validate_godot_imports.py  # fixes broken imports
```

### 13.4 Documentation Updates

Each asset update updates:
- ART_PIPELINE_ASSET_INVENTORY.md (live checklist)
- docs/assets/ folder automatically re-built from source
- Git commit messages include asset tag: "feat(asset): dragon sword model v1"

---

## APPENDIX A: QUICK REFERENCE

### Godot 4.2 Format Support
| Format | Import OK? | Notes |
|--------|-----------|-------|
| GLB   | Excellent | Binary GLTF, single file, preferred |
| GLTF  | Good | Separate files: .bin + textures |
| FBX   | Good | Binary only, may need unit conversion |
| OBJ   | Limited | No animation, basic materials |
| DAE   | Unstable | Collada occasionally failing  
| Blender .blend | No direct import | Export first

### Recommended Compression
| Platform | Texture Format |
|----------|---------------|
| Windows (DX11/12) | BC7 (best) / BC5 (normals) |
| Linux | BC7 / ETC2 fallback  
| macOS | ASTC (Apple silicon native)
| Android | ASTC (preferred) / ETC2 (fallback)
| iOS | ASTC only

### File Size Targets
- Single character GLB: 3-5 MB (with textures embedded)
- Weapon GLB: < 500 KB
- Environment prop GLB: < 1 MB
- Zone .tscn packed scene: < 30 MB (scene references external assets)

---

## APPENDIX B: GLOSSARY

**GLTF/GLB:** Modern 3D exchange format (Khronos). Binary (GLB) preferred.
**PBR:** Physically Based Rendering - realistic light/material interaction using albedo, normal, roughness, metallic maps
**ORM:** Occlusion/Roughness/Metallic packed into single texture (R=Occlusion, G=Roughness, B=Metallic)
**UV Unwrap:** 2D projection of 3D mesh coordinates for texture mapping
**Normal Map:** Simulates surface detail by perturbing normals per-pixel
**Tri Count:** Number of triangles in mesh; lower = faster rendering
**LOD:** Level of Detail - lower-poly proxies at distance
**Occlusion Culling:** Skip rendering objects not visible from camera
**Instancing:** Draw many copies of same mesh with single draw call
**Shadow Map:** Depth texture from light's perspective, used to compute shadows
**Mipmap:** Pre-generated downscaled texture levels for distant viewing
**Forward+ Rendering:** Godot default 4.2 renderer; cluster-based lighting

---

## APPENDIX C: EXAMPLE ASSET SPEC

### Character: player_male

```yaml
asset_id: player_male
type: character
format: glb
tri_count: 15000  # IDEAL: 12k-18k
bones: 32         # IDEAL: humanoid skeleton
vertex_influences: 4  # Max per-vertex bone weights

pbr:
  albedo:
    path: textures/albedo.png
    size: 1024
    colorspace: sRGB
  normal:
    path: textures/normal.png  
    size: 1024
    colorspace: Linear
    tangent: generated
  roughness:
    path: textures/roughness.png
    size: 1024
    colorspace: Linear
  metallic:
    path: textures/metallic.png
    size: 1024
    colorspace: Linear
  ao:
    path: textures/ao.png
    size: 1024
    colorspace: Linear

animations:
  - idle
  - walk
  - run
  - jump_start
  - jump_loop
  - jump_land
  - attack_1h_left
  - attack_1h_right
  - block
  - dodge
  - hurt
  - death
  - respawn

material_overrides:
  - skin_tone: [tintable via vertex color or parameter]
  - armor_color: [material parameter]

networking:
  - networked_property: health (float 0.0-1.0)
  - networked_property: attack_state (int enum)
  - networked_property: is_dead (bool)

import_godot:
  generate_tangents: true
  generate_lod: false  # manually create LODs
  keep_above: 4
```

---

## DOCUMENT HISTORY

- 2026-04-29 v1.0 - Initial specification following DarkAges combat FSM completion

---

**END OF SPECIFICATION**  
**Next:** Create ART_PIPELINE_ASSET_INVENTORY.md with actual checklist  
**Then:** Begin Month 1 Week 1: toolchain installation + test export pipeline
