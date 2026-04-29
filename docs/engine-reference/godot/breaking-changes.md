# Godot - Breaking Changes Reference

## From 4.2 to 4.6 (Upgrade Path Planning)

When upgrading DarkAges from Godot 4.2 to a later version, expect these breaking changes:

### 4.2 -> 4.3 (estimated mid-2025)
- GDExtension API stabilization
- C# 12 support
- Some deprecated APIs removed (check release notes)

### 4.2 -> 4.5 (estimated late 2025)
- New GDScript: @abstract decorator (enforce abstract classes)
- New GDScript: variadic arguments (func foo(args...))
- Shader Baker: Pre-compile shaders to reduce hitches
- SMAA 1x anti-aliasing option
- Stencil buffer accessible in shaders

### 4.2 -> 4.6 (January 2026 - HIGH IMPACT)
- **Jolt Physics becomes DEFAULT** for 3D (GodotPhysics still available but opt-out)
  If using GodotPhysics features (HingeJoint3D.damp), test thoroughly.
- **IK system restored**: SkeletonModifier3D replaces old SkeletonIK3D approach
  Old SkeletonIK3D node removed. Must migrate to SkeletonModifier3D children.
- **Rendering backend on Windows**: D3D12 default (was Vulkan). May affect shader compatibility.
- **Glow processing order**: Now BEFORE tonemapping (was after). Visuals will change.
- **Tonemapper**: AgX added (new default? Check). White point/contrast controls added.
- **UI focus split**: Dual-focus system — mouse/touch focus separate from keyboard/gamepad focus.
- **Quaternion default**: Identity initialization (was zero)
- **C# automatic string extraction**: Translation strings auto-found in C# code
- **Editor theme**: Modern theme default (grayscale vs blue tint)

For DarkAges, an upgrade to 4.6 would require:
- Rewriting foot IK from SkeletonIK3D (4.2) to SkeletonModifier3D (4.6)
- Testing physics with Jolt (or pin to GodotPhysics)
- Adjusting visual effects and glow parameters
- Verifying all shaders under new backend

Recommendation: Stay on 4.2 until after demo is complete, then evaluate upgrade cost vs benefit.
