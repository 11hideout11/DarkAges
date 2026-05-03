# PRD: Procedural Camera System

## Introduction

The DarkAges camera uses basic SpringArm3D for third-person following but lacks cinematic feel. Current implementation has basic collision avoidance but no procedural framing, dynamic distance, or cinematic shake. This PRD enhances the camera system for improved gameplay feel.

## Goals

- Dynamic camera distance based on action
- Camera shake on impacts
- Smooth collision push-in
- Cinematic framing for combat

## User Stories

### CAM-001: Dynamic Distance
**Description:** As a player, I want the camera to zoom out during combat for better awareness.

**Acceptance Criteria:**
- [ ] Camera pulls back when in combat
- [ ] Returns to close third-person when idle
- [ ] Smooth interpolation between distances
- [ ] Configurable distance values

### CAM-002: Impact Camera Shake
**Description:** As a player, I want the camera to shake when I take damage.

**Acceptance Criteria:**
- [ ] Screen shake on player hit
- [ ] Heavy shake on critical hits
- [ ] Shake intensity scales with damage
- [ ] Configurable shake curve

### CAM-003: Collision Push-In
**Description:** As a player, I want the camera to push in when near obstacles.

**Acceptance Criteria:**
- [ ] Camera moves closer when obstructed
- [ ] Push-in threshold configurable
- [ ] Smooth return when clear
- [ ] No jitter during transition

### CAM-004: Combat Framing
**Description:** As a player, I want the camera to frame enemies optimally during combat.

**Acceptance Criteria:**
- [ ] Camera biases toward locked target
- [ ] Target centered horizontally
- [ ] Offset for weapon visibility

## Functional Requirements

- FR-1: Add CameraController node with dynamic distance
- FR-2: Implement shake system with curves
- FR-3: Raycast push-in detection
- FR-4: Target bias during lock-on
- FR-5: Configuration via exported values

## Non-Goals

- No cinematic cutscenes (post-MVP)
- No mouse orbit controls
- No camera transition animations
- No first-person mode

## Technical Considerations

### Godot 4.2 Camera Implementation
```gdscript
# CameraController.gd - Godot 4.2
extends Node3D

@export var base_distance: float = 5.0
@export var combat_distance: float = 8.0
@export var shake_intensity: float = 0.1
@export var shake_decay: float = 5.0

var current_distance: float
var shake_offset: Vector3

func _process(delta):
    update_distance_target(delta)
    apply_shake(delta)
    update_camera_position(delta)

func apply_impact_shake(intensity: float):
    shake_offset += random_vector() * intensity
```

### Configuration
- Export variables for all distances/times
- Curve resource for shake decay
- Tween for smooth transitions

## Success Metrics

- Camera responds to combat state
- Shake visible on damage
- No camera clipping through geometry
- Smooth transitions in all cases

## Open Questions

- Should camera behavior differ by class?
- Do we need camera presets for menus?
- Should lock-on affect camera freely?