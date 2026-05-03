# PRD: Foot IK System
## Introduction
The player character lacks foot IK (Inverse Kinematics) for terrain alignment, meaning feet clip through slopes and stairs unnaturally. This is a critical visual polish item for immersion and is marked as deferred in the Compatibility Analysis.  **Problem Statement:** Character feet do not adapt to terrain slope, creating visual artifacts when walking on uneven ground.
---


## Goals
- Implement SkeletonIK3D-based foot IK
- Align feet to terrain surface normal
- Smooth transition on slope changes
- Support stairs and small obstacles
- Maintain 60fps performance
---

## User Stories
### US-001: Implement Foot IK Controller
**Description:** As a player, I want my feet to align with the ground so my character looks grounded.
**Acceptance Criteria:**
- [ ] SkeletonIK3D nodes on both feet
- [ ] Raycast from hip to ground
- [ ] Foot rotation matches terrain normal
- [ ] IK target updates at 60Hz
- [ ] No foot sliding on slopes

### US-002: Handle Stairs and Obstacles
**Description:** As a player, I want smooth foot adaptation on stairs so movement feels natural.
**Acceptance Criteria:**
- [ ] Detect step height changes
- [ ] Smooth foot lift on stairs
- [ ] Appropriate step height threshold
- [ ] Performance maintained at 400 entities
- [ ] Fallback when no terrain detected

### US-003: Integrate with AnimationTree
**Description:** As an animator, I need foot IK to blend with animations so motion is smooth.
**Acceptance Criteria:**
- [ ] IK influence controllable
- [ ] Blend with locomotion animations
- [ ] Suppressed during jumps/falls
- [ ] Additive rotation on slopes
- [ ] Root bone translation correct
---

## Functional Requirements
- FR-1: Add SkeletonIK3D nodes to player skeleton
- FR-2: Implement FootIKController extending Node3D
- FR-3: Raycast system for terrain detection
- FR-4: Foot rotation calculation
- FR-5: Stair detection and handling
- FR-6: Animation blending integration
- FR-7: Performance optimization (entity limits)
- FR-8: Unit tests for IK calculations
---

## Non-Goals
- No full body IK (only feet)
- No procedural animation generation
- No foot planting on dynamic objects
- No IK retargeting between rigs
---

## Technical Considerations
### Implementation Approach
1. **Raycast Method:** Cast ray from hip to find ground, calculate foot target rotation
2. **Two-Bone IK:** Use built-in SkeletonIK3D with pole targets
3. **Blend:** Smooth lerp between animation and IK rotation
### Performance Constraints
- Maximum 50 entities with full IK enabled
- LOD: Simplified IK for distant players
- LOD: Disable IK beyond threshold distance
---

## Open Questions
1. What is the max slope angle for IK activation?
2. Should NPC feet also have IK?
3. What is the fallback when no floor detected?
4. Is dynamic bone posing required?
---