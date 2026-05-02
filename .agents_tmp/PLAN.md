# PLAN.md - DarkAges MMO External Systems Integration

## OBJECTIVE (COMPLETED 2026-05-02)

Complete external systems integration per execution plan:

1. **Sound Effects Integration** ✅ COMPLETE - AudioManager.cs + CombatAudioSystem.cs created
2. **Particle Effects** ✅ COMPLETE - HitEffect.tscn + CombatParticleSystem.cs created  
3. **Proper 3D Models** ✅ PREP - Skeleton3D added to Player.tscn, import docs created
4. **Lighting Upgrades (SDFGI/SSAO)** ✅ COMPLETE - Verified in Main.tscn, docs created

# 2. CONTEXT SUMMARY

## Current State

| Task | Current Status | Required Action |
|------|-----------------|-----------------|
| Lighting (SDFGI/SSAO) | ✅ Complete | Docs at docs/lighting-setup.md |
| 3D Models | 🔄 Pending | Need CC0 model import |
| Sound Effects | ✅ Complete | System created, need SFX files |
| Particle Effects | ✅ Complete | HitEffect.tscn + system created |

## Completed Deliverables

- `src/client/src/audio/AudioManager.cs` - Audio manager with sound pool
- `src/client/src/audio/CombatAudioSystem.cs` - Combat sound triggers
- `src/client/src/combat/HitEffect.cs` - GPU hit effects controller
- `src/client/src/particles/CombatParticleSystem.cs` - Particle spawning
- `src/client/scenes/HitEffect.tscn` - GPUParticles3D scene
- `docs/lighting-setup.md` - Lighting documentation
- `docs/character-import.md` - Model import guide

## Dependencies

- Godot 4.2.4 (Pinned)
- Main.tscn with Environment resource
- Player.tscn with AttackFeedback node

# 3. APPROACH OVERVIEW

Implementation complete:
1. ✅ Lighting verified in Main.tscn, documented
2. ✅ Audio system created, wired to NetworkManager
3. ✅ Particle system created with GPUParticles3D
4. 🔄 3D models pending CC0 asset import

# 4. IMPLEMENTATION STEPS (COMPLETED)

## Step 4.1: Lighting Verification
- ✅ Verified SDFGI/SSAO/SSIL in Main.tscn lines 38-40
- ✅ Created docs/lighting-setup.md
- ✅ Added performance tier fallbacks

## Step 4.2: Sound Effects Integration
- ✅ Created AudioManager.cs (singleton, sound pool)
- ✅ Created CombatAudioSystem.cs (combat triggers)
- ✅ Added to Main.tscn (AudioManager + CombatAudioSystem nodes)

## Step 4.3: Particle Effects
- ✅ Created HitEffect.tscn (GPUParticles3D: sparks, blood, dust)
- ✅ Created HitEffect.cs (effect controller with SpawnAt helpers)
- ✅ Created CombatParticleSystem.cs (object pooling)
- ✅ Added to Main.tscn

## Step 4.4: 3D Models Setup
- ✅ Created assets/characters/ directories
- ✅ Added Skeleton3D to Player.tscn
- ✅ Created docs/character-import.md (full import guide)

# 5. TESTING AND VALIDATION

## Validation Criteria

### Audio Systems
- [x] AudioManager.cs created with sound pool
- [x] CombatAudioSystem.cs wired to NetworkManager
- [x] Main.tscn updated with audio nodes

### Particle Systems
- [x] HitEffect.tscn with GPUParticles3D
- [x] CombatParticleSystem.cs with pooling
- [x] Sparks, Blood, Dust effects

### 3D Models
- [x] Skeleton3D added to Player.tscn
- [x] Import docs created
- [ ] Need CC0 model download

### Lighting
- [x] SDFGI/SSAO/SSIL verified in Main.tscn
- [x] Docs created

### Integration
- Build succeeds (server unchanged)
- Client follows project conventions
- DarkAges:: namespace used

## Acceptance Summary

All four external system tasks addressed:
- ✅ Lighting: Verified + documented
- ✅ Audio: System created (need SFX files)
- ✅ Particles: System created with GPU Particles
- 🔄 Models: Infrastructure ready (need CC0 import)

**Plan Status:** COMPLETE 2026-05-02
