# PRD: Foot IK System Implementation

## Introduction

The DarkAges player character currently lacks foot inverse kinematics (IK), causing feet to clip through terrain. Players walking on uneven ground show floating or clipping feet, breaking immersion. This PRD implements proper SkeletonIK3D-based foot IK for terrain alignment using Godot 4.2-compatible APIs.

## Goals

- Implement foot IK for terrain alignment
- Support sloping ground and stairs
- Maintain animation blending with IK adjust
- Use Godot 4.2 compatible SkeletonIK3D node

## User Stories

### FI-001: Terrain Foot Alignment
**Description:** As a player, I want my feet to align with the ground when walking on uneven terrain.

**Acceptance Criteria:**
- [ ] Foot positions adjust to terrain height
- [ ] No foot clipping through ground
- [ ] IK resolves in <2ms per frame
- [ ] Works on slopes up to 45 degrees

### FIK-002: Animation Blending
**Description:** As an animator, I want IK to blend with baked animations smoothly.

**Acceptance Criteria:**
- [ ] IK only applies when grounded
- [ ] Blend weight controlled by animation
- [ ] No foot sliding during blend
- [ ] Hit/damage animations override IK

### FIK-003: Stair Handling
**Description:** As a player, I want to walk up/down stairs naturally.

**Acceptance Criteria:**
- [ ] Foot lifts for step height
- [ ] Smooth transition between steps
- [ ] Works with NavigationMesh pathfinding

## Functional Requirements

- FR-1: SkeletonIK3D nodes for left/right foot
- FR-2: Raycast ground detection per foot
- FR-3: IK target position calculation
- FR-4: Blend with AnimationTree state
- FR-5: Enable/disable via animation parameter

## Non-Goals

- No hand IK (weapons don't interact with world)
- No complex terrain LOD (flat ground initially)
- No procedural locomotion (use baked walk cycle)
- No dynamic obstacle avoidance

## Technical Considerations

### Godot 4.2 Compatibility
Per godot-4-2-pinned skill:
- MUST use SkeletonIK3D NOT SkeletonModifier3D
- Use physical_bones_only_collision for raycasts
- AnimationTree blend via state machine parameters

### Implementation Approach
```gdscript
# Player.gd - Foot IK setup
func _ready():
    left_foot_ik = SkeletonIK3D.new()
    left_foot_ik.skeleton = skeleton
    left_foot_ik.target_node = left_foot_target
    left_foot_ik.chain_length = 2
    left_foot_ik.iterations = 4
    
    right_foot_ik = SkeletonIK3D.new()
    # ...same setup
    
func _process(delta):
    if is_on_ground():
        update_ik_targets()
        left_foot_ik.process(delta)
        right_foot_ik.process(delta)
```

### Raycast Ground Detection
- Use PhysicsRayQueryParameters3D
- Ray origin at hip height, direction down
- Max distance: 0.5m for step detection
- Filter by terrain collision layer

## Success Metrics

- No foot clipping in 100% of test cases
- IK resolves <2ms frame time
- Works on 0-45 degree slopes
- Blend transitions invisible

## Open Questions

- Should IK only apply when moving vs always?
- What blend time for IK enable/disable?
- Do we need hip height adjustment too?