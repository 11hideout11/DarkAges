# Godot Rendering Module - Quick Reference (4.2)

- **Backend**: Vulkan (DirectX 12 added in 4.6)
- **Camera3D**: Perspective or orthographic; controls tonemap, DOF, glow
- **Environment**: WorldEnvironment node with Environment resource (sky, ambient light, glow)
- **MeshInstance3D**: instance a Mesh resource; use GeometryInstance3D LODs
- **GPUParticles2D/3D**: particle effects (preferred over CPUParticles for performance)
- **ShaderMaterial**: Custom shading with Godot shading language (.gdshader)
- **Shaders**: vertex/fragment/light functions; uniform parameters exposed in inspector

No AgX (4.6), no SMAA 1.0 (4.5), no Shader Baker (4.5).

See: https://docs.godotengine.org/en/4.2/classes/class_meshinstance3d.html
