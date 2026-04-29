# Godot Physics Module - Quick Reference (4.2)

DarkAges does NOT rely on Godot physics for gameplay - server authoritative.

For client-side visual only:
- **CharacterBody3D**: kinematic body, use _PhysicsProcess, MoveAndSlide()
- **CollisionShape3D**: primitive shapes (sphere, box, capsule)
- **RayCast3D**: ground detection for foot placement, LOS
- **Area3D**: trigger volumes

Default engine: GodotPhysics3D (Jolt is opt-in in 4.2, not default).

See: https://docs.godotengine.org/en/4.2/classes/class_characterbody3d.html
