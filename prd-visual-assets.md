# PRD: Visual Asset Pipeline Implementation

## Introduction

The DarkAges MMO currently has ZERO visual assets - only architectural scaffolding exists. The codebase contains sophisticated systems (Combat FSM, AnimationStateMachine, TargetLock, etc.) but no 3D models, textures, shaders, or environments. This prevents demo MVP readiness despite all server systems being operational.

This PRD defines the minimal visual asset pipeline to achieve MVP demo readiness with playable combat zones.

## Goals

- Provide basic player character model with animations
- Provide enemy NPC models for demo combat
- Create minimal playable zone environments
- Implement combat VFX (damage numbers, hit markers)
- Achieve visual demo readiness within asset creation budget

## User Stories

### VS-001: Player Character Model
**Description:** As a player, I need a visible character model so I can see my avatar in third-person view.

**Acceptance Criteria:**
- [ ] Player humanoid mesh (GLTF2 format) imported to Godot
- [ ] Basic skeleton with standard humanoid rig (30+ bones)
- [ ] Idle, Walk, Run, Attack animations working
- [ ] Model visible in Godot client at runtime

### VS-002: Enemy NPC Models
**Description:** As a player, I need enemy targets to attack in demo combat scenarios.

**Acceptance Criteria:**
- [ ] At least 2 NPC enemy variants (melee, ranged)
- [ ] Basic animations (idle, attack, hit, death)
- [ ] Health bars display above NPCs
- [ ] NPCs respond to combat with damage/death

### VS-003: Demo Zone Environment
**Description:** As a player, I need a basic playing environment with ground and cover.

**Acceptance Criteria:**
- [ ] Flat ground plane with basic texture
- [ ] At least 3 decorative props (rocks, columns, crates)
- [ ] Basic lighting (directional + ambient)
- [ ] Navigation mesh functional for NPC pathfinding

### VS-004: Combat VFX System
**Description:** As a player, I need visual feedback when hitting enemy targets.

**Acceptance Criteria:**
- [ ] Damage numbers float up from hit location
- [ ] Hit flash effect on successful attack
- [ ] Death effect when NPC dies
- [ ] Health bar updates visible in real-time

## Functional Requirements

- FR-1: Import pipeline for GLTF2 character models into Godot 4.2
- FR-2: Animation state machine connecting player input to animation playback
- VFX system floating damage numbers above hit location
- FR-3: Health bar component billboarded above entities
- FR-4: Simple ground plane with basic collision
- FR-5: Props system for environment decoration

## Non-Goals

- No advanced PBR materials (use basic colored materials)
- No complex terrain (flat ground only)
- No voice acting or dialogue cinematics
- No character customization/appearance system
- No environmental shaders (fog, volumetric lighting deferred)

## Design Considerations

### Godot 4.2 Compatibility
Per the godot-4-2-pinned skill, all implementations must use Godot 4.2 APIs:
- SkeletonIK3D not SkeletonModifier3D
- GodotPhysics3D not Jolt
- AnimationTree with StateMachine root

### Asset Sources (CC0/Low-Cost)
Based on research in docs/ART_PIPELINE_RESEARCH.md:
- Mixamo.com free rigged characters
- Kenney.game free assets (CC0)
- Blender primitive cylinders/cubes as placeholders initially

## Technical Considerations

### Import Pipeline
1. Export from Blender as .glb (binary GLTF2)
2. Godot auto-import handles materials
3. Create AnimationLibrary from imported animations
4. Connect AnimationTree node for state machine

### VFX Implementation
- Damage numbers: Label3D with Billboard enabled
- Hit flash: Tween albedo property on material
- Health bars: MeshInstance3D with gradient texture

## Success Metrics

- Player character visible and animated in <1 hour setup time
- Demo combat scenario playable with 2 enemy types
- VFX display correctly on all hit/death events
- No performance regression (maintain 60 FPS)

## Open Questions

- Should we use placeholder geometric shapes initially vs. time spent on proper models?
- What is the exact animation count needed for MVP?
- Should NPC AI use simple state machine or existing NPCAISystem?