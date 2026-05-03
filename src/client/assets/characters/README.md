# Character Assets Directory

This directory contains 3D character and monster models for the DarkAges client.

## Infrastructure (2026-05-03)

The game now includes **`CharacterModelLoader.cs`** - a runtime model loading system that:
- Automatically attempts to load 3D models when available
- Falls back to colored capsule placeholders when no model exists
- Supports caching to avoid repeated loading
- Provides color-coded distinction between character types

## Directory Structure

```
characters/
├── player_male/           # Male player character models
│   └── player_male.glb   # Add .glb file here
├── player_female/        # Female player character models (optional)
├── monsters/
│   ├── goblin/          # Goblin enemy models
│   ├── skeleton/        # Skeleton enemy models
│   ├── orc/            # Orc enemy models
│   └── troll/          # Troll enemy models
└── README.md           # This file
```

## Model Requirements

### Format
- **Preferred**: glTF 2.0 (.glb binary or .gltf + .bin)
- **Alternative**: FBX (.fbx)

### Specifications
- **Polygon count**: 5,000-15,000 triangles
- **Skeleton**: Humanoid rig preferred (Mixamo-compatible bone names)
- **Animations**: Idle, Walk, Run, Attack, Hit, Death

### Bone Naming (Mixamo Standard)
For animation compatibility, use these bone names:
- `Hips`
- `Spine`, `Spine1`, `Spine2`
- `Neck`, `Head`
- `LeftShoulder`, `LeftArm`, `LeftForeArm`, `LeftHand`
- `RightShoulder`, `RightArm`, `RightForeArm`, `RightHand`
- `LeftUpLeg`, `LeftLeg`, `LeftFoot`, `LeftToe`
- `RightUpLeg`, `RightLeg`, `RightFoot`, `RightToe`

## Download Sources (CC0 / Public Domain)

### Quaternius Universal Characters
- URL: https://quaternius.com/
- License: CC0 Public Domain
- Contains rigged characters with animations

### Kenney Assets
- URL: https://kenney.nl/assets/character-pack
- License: CC0 (when available)

## Import Steps

### Step 1: Download
1. Download a .glb model from a CC0 source
2. Place in appropriate directory (e.g., `player_male/`)

### Step 2: Godot Import
1. Open Godot 4.2.4 Editor
2. Drag .glb to FileSystem dock
3. Configure import:
   - Import as: Skeleton
   - Sibling file with animations: Yes

### Step 3: Scene Update
1. Open Player.tscn
2. Add Skeleton3D child node
3. Add imported MeshInstance3D under Skeleton
4. Hide/disable CapsuleMesh placeholder

## Verification Checklist

After model import:
- [ ] Model appears correctly in-game
- [ ] Idle animation loops smoothly
- [ ] Walk animation plays at correct speed
- [ ] Attack animations trigger on input
- [ ] Hit/death animations play
- [ ] 60fps performance maintained
- [ ] No visual z-fighting

## Fallback Behavior

When no model is loaded, the game uses CapsuleMesh placeholder:
- Radius: 0.3m
- Height: 1.8m
- Color: Blue (#3399FF)

The capsule provides basic visual feedback and collision until a proper model is imported.