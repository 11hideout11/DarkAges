# Godot 4.2 - Current Best Practices

Last verified: 2026-04-29 | Engine: Godot 4.2.4

This file documents idioms valid for Godot 4.2. For post-4.2 features (4.3-4.6), see upgrading guide.

## General

- Use **Vulkan** rendering backend (default). Do not assume D3D12 (4.6 only).
- Target 60 FPS. Server tick 60 Hz (16.67ms).
- Use **Project Settings** for configuration, not hardcoded values.

## C# (DarkAges Client)

- C# version: 10 (.NET 6 in Godot 4.2)
- Node access:
  ```csharp
  [Export] public PackedScene EnemyScene { get; set; }
  [Export] public NodePath PlayerCameraPath { get; set; }
  private Camera3D _camera;
  
  public override void _Ready()
  {
      _camera = GetNode<Camera3D>(PlayerCameraPath);
  }
  ```
- Signals: Subscribe with `+=`; unsubscribe in `_ExitTree()` to avoid leaks
- Resource loading:
  ```csharp
  var texture = GD.Load<Texture2D>("res://icon.svg");
  // or async:
  var loader = ResourceLoader.LoadInteractive("res://large.tscn");
  ```
- Avoid `GD.Print` in hot loops; use conditional logging with `OS.IsDebugBuild()`

## Animation (4.2)

- **AnimationTree**: Use `AnimationNodeStateMachine` for FSM; `AnimationNodeBlendSpace1D/2D` for parametric blend
- **IK**: Godot 4.2 has `SkeletonIK3D` node (3D). Configure in editor:
  - Add SkeletonIK3D as child of Skeleton3D
  - Set target bone, tip bone
  - Add Marker3D node as IK target
  - Enable `Override Pose`. SkeletonIK runs after physics.
- **Foot IK**: Attach SkeletonIK3D to LeftFoot/RightFoot bones, target to Marker3D positioned by ground raycast (from player capsule)
- Note: `SkeletonModifier3D` (CCIK, FABRIK, etc.) introduced in 4.6 — not available.

## Physics (4.2)

- **Default engine**: GodotPhysics3D (JoltPhysics3D is opt-in as third-party in 4.2, not default)
- **CharacterBody3D**: Use for players/NPCs. Move with `MoveAndSlide()` in `_PhysicsProcess`
- **Collision layers/masks**: Bitmask system (32 layers). Configure in project settings.
- **Raycasts**: `RayCast3D` node for ground detection (for foot IK, line-of-sight). Call `ForceRaycastUpdate()` or process in physics step.

## Rendering (4.2)

- **Backend**: Vulkan. DirectX 12 not supported until 4.6.
- **Tonemapping**: `Camera3D.Tonemap` can be `ACESFilmic`, `Reinhardt`, `Linear`. No AgX.
- **Anti-aliasing**: FXAA (cheap) or TAA (temporal, can ghost). No SMAA 1x (4.5+).
- **Glow**: Post-processing via `Glow` layer; rendered *after* tonemapping in 4.2 (changed in 4.6).
- **Environment**: `WorldEnvironment` with `Environment` resource for sky, ambient light, glow.

## GDScript (4.2 — If ever used)

- GDScript 2.0 available (type hints, `@export`, `@onready`, `@export_group`)
- No `@abstract` decorator (added 4.5). Document abstract methods with comments and `NotImplementedException` in body.
- No variadic arguments (added 4.5). Use `Array` parameter instead.
- No script backtraces in release builds (added 4.5).

## Networking (Godot-native)

DarkAges does NOT use Godot's high-level multiplayer. Uses custom UDP + Protobuf.
- For local testing only: `ENetMultiplayerPeer` (P2P, not authoritative)
- Never use for production MMO.

## UI (4.2)

- Control node hierarchy: `CanvasLayer` -> `Control` children
- Use signals to decouple UI from gameplay
- `LineEdit`, `Button`, `Panel`, `RichTextLabel`, etc.
- No UI Toolkit (Unity) or CommonUI (Unreal) equivalents.

## Important Modules

For detailed API reference for each subsystem, see module-specific docs in this directory.

