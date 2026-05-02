# Lighting Setup Documentation

## Overview
The DarkAges demo uses Godot 4.2.4's Vulkan rendering pipeline with advanced global illumination features.

## Configuration (Main.tscn)

### Environment Resource
Located at: `src/client/scenes/Main.tscn` - SubResource `Environment_default`

```gdscript
[sub_resource type="Environment" id="Environment_default"]
background_mode = 1
background_color = Color(0.529412, 0.807843, 0.921569, 1)
ambient_light_source = 2
ambient_light_color = Color(0.3, 0.3, 0.3, 1)
ssao_enabled = true
ssil_enabled = true
sdfgi_enabled = true
```

### Enabled Features

| Feature | Value | Description |
|---------|-------|-------------|
| SDFGI | true | Screen-space directional field global illumination |
| SSAO | true | Screen-space ambient occlusion |
| SSIL | true | Screen-space indirect lighting |
| Ambient Light | 2 (COLOR) | Ambient color-based lighting |
| Background | SKY | Sky-based background mode |

### Performance Tiers

For low-end GPUs, adjust the Environment resource:

**High Quality (Default)**
- sdfgi_enabled = true
- sdfgi_cascades = 6
- sdfgi_secondary = true

**Medium Quality**
- sdfgi_enabled = true
- sdfgi_cascades = 4
- sdfgi_secondary = false

**Low Quality (Fallback)**
- sdfgi_enabled = false
- ssao_enabled = true
- ssil_enabled = true

### WorldEnvironment Node
Located at: `Main.tscn` → World → Environment node (line 96-97)

```gdscript
[node name="Environment" type="WorldEnvironment" parent="World"]
environment = SubResource("Environment_default")
```

## Directional Light

The main directional light is configured for shadow casting:

```gdscript
[node name="DirectionalLight3D" type="DirectionalLight3D" parent="World"]
transform = Transform3D(0.707107, -0.5, 0.5, 0, 0.707107, 0.707107, -0.707107, -0.5, 0.5, 0, 10, 0)
shadow_enabled = true
shadow_bias = 0.05
```

## Ambient Settings

- **Ambient Light Source**: 2 (COLOR) - Uses ambient_light_color
- **Ambient Light Color**: RGB(0.3, 0.3, 0.3) - Soft gray fill
- **Sky Tint**: Matches sky background

## Verification Checklist

- [x] SDFGI enabled in Environment resource
- [x] SSAO enabled in Environment resource  
- [x] SSIL enabled in Environment resource
- [x] WorldEnvironment node references correct Environment
- [x] DirectionalLight3D has shadow_enabled = true
- [x] Ground mesh uses StandardMaterial3D for proper GI response

## Known Limitations

1. SDFGI requires Vulkan renderer (not OpenGL)
2. Performance impact ~15-30% on mid-range GPUs
3. Mobile not supported (desktop Vulkan only)

## Godot Version

Pinned to **Godot 4.2.4** (NOT 4.6) - See `docs/engine-reference/godot/` for details.