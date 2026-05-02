# PRD-020: SDFGI/SSAO Lighting System

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** Proposed  
**Priority:** Medium  
**Prerequisite:** None - Greenfield

---

## 1. Problem Statement

The lighting system in Godot 4.2 client lacks advanced rendering features:
- ⚠️ No SDFGI (Signed Distance Field Global Illumination)
- ⚠️ No SSAO (Screen Space Ambient Occlusion)
- ⚠️ No environment contact shadows

This affects visual quality and demo readiness. SDFGI/SSAO are standard features in Godot 4.x renderer.

### Current State
- ⚠️ Not implemented
- ⏸️ Requires Godot rendering expertise

### Impact
- Flat looking environments
- No contact shadows
- Visual quality below modern standards

---

## 2. Goals

### Primary Goals
1. Enable SDFGI for global illumination
2. Enable SSAO for contact shadows
3. Configure environment for performance
4. Optimize for 60fps target

### Success Criteria
- [ ] SDFGI enabled in World.tscn
- [ ] SSAO enabled and tuned
- [ ] 60fps maintained (400 entities)
- [ ] Demo scene looks polished

---

## 3. Technical Specification

### Godot 4.2 Rendering Pipeline

```
World.tscn:
├── WorldEnvironment
│   ├── Environment
│   │   ├── Background: Sky
│   │   ├── SDFGI: Enabled  ← Add
│   │   ├── SSAO: Enabled  ← Add
│   │   ├── Glow: Enabled
│   │   └── Adjustments: ACESFilmic
│   └── VolumetricFog (if needed)
├── DirectionalLight3D
│   └── VolumetricFog (optional)
└── RoomGeometry
```

### Configuration

```gdscript
# WorldEnvironment.environment resource
var env = Environment.new()

# SDFGI Configuration
env.sdfgi_enabled = true
env.sdfgi_energy = 1.0
env.sdfgi_normal_bias = 0.9
env.sdfgi_ao_clause = 1.0
env.sdfgi_ray_step = 0.08
env.sdfgi_anisotropy = false

# SSAO Configuration  
env.ssao_enabled = true
env.ssao_radius = 1.0
env.ssao_intensity = 1.0
env.ssao_bias = 0.5
env.ssao_blur = true
env.ssao_mix = 0.5
env.ssao_color = Color.BLACK

# Contact shadows via SSAO (no separate setting needed)
env.ssao_depth_tolerance = 0.5
```

### Performance Tuning

| Setting | Quality High | Quality Medium | Quality Low |
|---------|-------------|----------------|-------------|
| SDFGI Enabled | 1.0 | 1.0 | 1.0 (off) |
| SDFGI Rays | 8 | 6 | off |
| SSAO Quality | High | Medium | Low |
| Target FPS | 60 | 60 | 60 |

### Zone-Specific Settings

```gdscript
# zones/main_area.gd
extends Node3D

@onready var world_env = $WorldEnvironment

func _ready():
    # Boost quality for main hub
    if zone_type == ZoneType.HUB:
        world_env.environment.sdfgi_enabled = true
        world_env.environment.ssao_quality = Environment.SSAO_QUALITY_HIGH
    # Lower for combat zones
    elif zone_type == ZoneType.COMBAT:
        world_env.environment.sdfgi_enabled = true
        world_env.environment.sdfgi_energy = 0.5
        world_env.environment.ssao_quality = Environment.SSAO_QUALITY_MEDIUM
```

---

## 4. Implementation Plan

### Week 1: Configuration

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Enable SDFGI in World.tscn | GI visible |
| 3-4 | Enable SSAO in World.tscn | Shadows visible |
| 5 | Tune for performance | 60fps at 400 entities |
| 7 | Test across zones | All zones work |

### Week 2: Polish

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Add visual variety | Quality settings |
| 10 | Create presets | HUB/COMBAT/OUTDOOR |
| 11-12 | Documentation | Render settings guide |
| 14 | Final benchmark | 60fps confirmed |

### Dependencies
- World.tscn (exists)
- Godot 4.2.4 (pinned)

---

## 5. Testing Requirements

### Visual Validation
- SDFGI creates indirect lighting
- SSAO creates contacts shadows
- Performance: 60fps at 400 entities

### Platform Testing
- Linux (primary)
- Performance consistency

---

## 6. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Low (config only) |
| Time | 1 week |
| LOC | ~50 |
| Skills | Godot 4.x rendering |

---

## 7. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Performance | High | High | Zone-specific presets |
| Hardware variance | Medium | Medium | Quality settings |

---

## 8. Open Questions

1. **Q: Volumetric fog?**
   - A: Deferred - can add if performance allows

2. **Q: Ray tracing?**
   - A: Godot 4.2 doesn't have - use SDFGI

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Enable in World.tscn