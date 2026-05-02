# Character Model Import Guide

## Overview
This document guides the import of CC0 3D character models into the DarkAges Godot 4.2.4 client.

## Current State
- Player uses CapsuleMesh placeholder (see `Player.tscn` lines 30-32)
- RemotePlayer uses CapsuleMesh placeholder
- Need to replace with proper humanoid models

## CC0 Character Sources

### Recommended Assets
1. **Quaternius Universal Base Characters** - CC0 Public Domain
   - https://quaternius.com/ (search "Universal Base Character")
   - ~13k triangles, rigged
   - Includes idle, walk, run, attack animations

2. **Kenney Character Pack** - CC0 (when available)
   - https://kenney.nl/assets/character-pack

3. **Mixamo** - Free (requires account)
   - https://www.mixamo.com/
   - Free rigged characters + animations

## Import Steps

### Step 1: Download Model
1. Download .glb or .fbx file from chosen source
2. Place in `src/client/assets/characters/player_male/`

### Step 2: Import to Godot
1. Open Godot 4.2 Editor
2. Drag .glb file to FileSystem dock
3. Godot auto-imports as .glb scene
4. Configure import settings:
   - Import as: Skeleton (not Scene)
   - Sibling file with animations: Yes
   - Mesh compression: Normal quality

### Step 3: Replace CapsuleMesh
1. Open `Player.tscn`
2. Delete CapsuleMesh from MeshInstance3D
3. Add Skeleton3D child node
4. Add imported MeshInstance3D as child
5. Configure AnimationTree to find Skeleton3D

### Step 4: Configure Animations
AnimationTree expects these animations:
- idle
- walk
- run
- attack_light
- attack_heavy
- block
- hit
- death

If imported model uses different names:
1. Open AnimationTree
2. Set animation retargeting
3. Map imported names to expected

## Current Character Placeholder

The capsule mesh configuration (to be replaced):

```gdscript
[node name="MeshInstance3D" type="MeshInstance3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.9, 0)
mesh = SubResource("CapsuleMesh_1")
```

## Skeleton Setup (Post-Import)

After import, configure skeleton:

```gdscript
[node name="Skeleton3D" type="Skeleton3D" parent="Player"]

[node name="CharacterModel" type="MeshInstance3D" parent="Skeleton3D"]
# Import player model here
```

## AnimationTree Bindings

The PlayerAnimationTree.tres expects:
- AnimationPlayer node reference
- Skeleton3D for bone transforms

After model import:
1. Select AnimationTree node
2. Set `Root Node` to Player
3. Verify animation references work

## Verification Checklist

- [ ] CC0 model downloaded to assets folder
- [ ] Model imported to Godot as .glb
- [ ] Skeleton3D created in Player.tscn
- [ ] Model mesh attached to Skeleton3D
- [ ] Idle animation plays correctly
- [ ] Walk/Run animation plays
- [ ] Attack animation triggers
- [ ] No z-fighting with shadow plane
- [ ] Maintains 60fps

## Godot Version Requirement

Pinned to **Godot 4.2.4** (NOT 4.6)

Import settings differ in 4.2.x vs 4.3+:
- Use `Import>Skeleton` mode
- Don't use `Import>glTF` new format