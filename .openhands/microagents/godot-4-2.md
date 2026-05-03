---
name: godot-4-2-pinned
type: knowledge
version: 1.0.0
agent: CodeActAgent
triggers:
  - godot
  - gdscript
  - c# godot
  - scene
  - node
  - animationtree
---
# Godot 4.2.4 Development — Pinned for DarkAges

## Project Context
- **Status**: MVP READY - Client builds successfully (0 errors, 208 warnings)
- **Client LOC**: ~9K LOC (C# Godot 4.2.4 Mono)
- **PRD Status**: PRD-005 (Client), PRD-008 (Combat FSM), PRD-011 (Foot IK), PRD-014/015 (Phantom Camera/Leaning), PRD-016 (SDFGI), PRD-019 (Blend Spaces), PRD-020 (Headless), PRD-022 (FSM), PRD-023 (Combat Text) ALL COMPLETE
- **Reference**: `AGENTS.md` for authoritative project state

## Version Lock: 4.2.4 (NOT 4.6)
All suggestions must be valid for Godot 4.2.4. Do NOT use 4.6 APIs.

### Breaking changes from 4.6 → 4.2
- IK: SkeletonModifier3D (4.6) → SkeletonIK3D (4.2)
- Physics engine: Jolt (4.6 default) → GodotPhysics3D (4.2 default)
- Rendering: AgX tonemap (4.6) → ACESFilmic (4.2)
- D3D12 default (4.6) → Vulkan (4.2)
- GDScript variadic arguments + @abstract (4.6 only) → not available in 4.2

## Client Architecture
- Language: C# Mono (NOT GDScript)
- Entry: `Main.tscn` → `GameManager.cs`
- Network: `NetworkMultiplayerENet` + `RemoteSceneTree` (server authoritative)
- Input: ExtendedInput with action maps (`movement`, `combat`, `interact`)
- Camera: ThirdPersonCamera smooth follow with deadzone and collision blocking
- Animation: `AnimationTree` state machine; `AnimationPlayer` blends; Foot IK via SkeletonIK3D

## Common Tasks
- RPC: Use `@rpc` attribute with `TransferMode = PeerToPeer` or `ServerAuthority`
- Scene loading: `get_tree().change_scene_to_file()` on main thread only
- File I/O: Use C# `System.IO` in user data dir; never block main thread
- Multiplayer authority: Server validates all state changes; client sends intent only

## Performance Tips
- Avoid `call_deferred` in hot loops; batch scene tree changes
- Use `MultiMeshInstance3D` for many identical props
- Monitor `Performance` singleton: `get_render_info().draw_call_count`

## Gap Notes (Client UI Systems NOT Implemented)
- Minimap/World Map
- Loading Screen
- Audio/Sound System
- Settings UI (basic only)
- Character Creation (minimal)
