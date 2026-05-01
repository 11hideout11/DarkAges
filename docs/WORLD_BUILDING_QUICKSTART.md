# DarkAges World-Building Research — Quick Start Index

**Date:** 2026-05-01
**Agent:** Hermes Research Phase

---

## Research Deliverables

| File | Purpose | Access |
|---|---|---|
| `docs/WORLD_BUILDING_CAPABILITIES_RESEARCH.md` | Complete research compendium (800 lines, 27 KB) | Agent read_file |
| `docs/ART_PIPELINE_SPECIFICATION.md` | 43-asset specification (CCS-style) | Agent read_file |
| `docs/ART_PIPELINE_MASTER_RESEARCH.md` | Asset sourcing + technical specs | Agent read_file |
| `docs/ART_PIPELINE_ASSET_INVENTORY.md` | Production checklist | Agent read_file |
| `docs/ART_PIPELINE_COMPLETE.md` | Technical architecture + LOD plan | Agent read_file |
| `docs/ART_PIPELINE_TOOLING.md` | DCC pipeline + file conventions | Agent read_file |
| `assets/manifest.json` | Asset tracking (status: missing/complete/placeholder) | Agent read_file |
| `~/.hermes/skills/gaming/world-building-capabilities/SKILL.md` | Skill auto-loads for queries | Automatic |

---

## What's Covered

- 3D modeling tools (Blender, MakeHuman, DAZ3D, Wings 3D)
- Texture/material PBR workflow (albedo, normal, roughness, metallic, AO, height)
- Shaders: toon/cel, outline, VFX, custom spatial shaders
- Terrain: GridMap, MeshLibrary, voxel plugins (godot_voxel), procedural generation  
- Foliage: MultiMesh instancing, Grass plugin
- Asset sources: Kenney, Polyhaven, AmbientCG, CC0 libraries
- Format: GLTF/GLB for models, tscn for scenes, material/gdshader for materials
- Godot 4.2 Forward+ integration specifics

---

## How Future Agents Use This

1. **Skill auto-load** — ask any query containing:
   - terrain generation, foliage tools, world building, asset pipeline,
   - gridmap, voxel, cc0 assets, procedural world, shader development
2. **Read manifest** — parse `assets/manifest.json` to see what's missing
3. **Follow pipeline** — use ART_PIPELINE_ASSET_INVENTORY.md to execute tasks
4. **Validate** — run build/test after each integration

---

## Next Recommended Actions

- `manifest.json` → filter status=missing → tier=HIGH → begin asset acquisition
- Select source library based on asset type (Kenney for props, Polyhaven for textures)
- Import into Godot → assign materials → configure collision → update manifest
- Use `darkages-client-feature-receptor` skill to integrate animation/networking