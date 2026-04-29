# Godot Animation Module - Quick Reference (4.2)

- **AnimationPlayer**: Classic timeline-based animation (still works)
- **AnimationTree**: State machine + blend spaces (recommended for character animation)
  - `AnimationNodeStateMachine` for FSM
  - `AnimationNodeBlendSpace1D/2D` for parametric blending (speed, direction)
  - `AnimationNodeAnimation` for single clip
- **IK in 4.2**: `SkeletonIK3D` node (NOT SkeletonModifier3D which is 4.6)
  - Enable `Override Pose` to take control from animation
  - Target: Marker3D node (placed by raycast or script)
- **AnimationPlayer vs AnimationTree**: Use AnimationTree for complex characters with state logic

See official docs: https://docs.godotengine.org/en/4.2/classes/tag_animation.html
