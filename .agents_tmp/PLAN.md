# PLAN.md - External Systems Integration for DarkAges MMO

## 1. OBJECTIVE

Complete the remaining visual and audiovisual tasks for the DarkAges MMO demo:

1. **Sound Effects Integration** - Add audio system code + acquire CC0 combat SFX
2. **Particle Effects** - Create GPUParticles3D combat hit effect system  
3. **Proper 3D Models** - Replace capsule placeholders with CC0 rigged character models
4. **Lighting Upgrades (SDFGI/SSAO)** - Verify and tune existing environment config

These are the final visual/audio polish items needed before MVP.

# 2. CONTEXT SUMMARY

## Current State

| Task | Current Status | Required Action |
|------|-----------------|-----------------|
| Lighting (SDFGI/SSAO) | ✅ Configured in Main.tscn (lines 38-40) | Verify + tune parameters |
| 3D Models | ❌ CapsuleMesh placeholders only | Import CC0 .glb characters |
| Sound Effects | ❌ No audio system, no files | Create system + acquire SFX |
| Particle Effects | ❌ No particle code exists | Create GPUParticles3D system |

## External Resources Identified (CC0)

- **3D Characters**: Quaternius Universal Base Characters (~13k tris, rigged)
- **Sound Effects**: OpenGameArt CC0 combat sounds (sword swing, hit)
- **Particle Textures**: Kenney Particle Pack (CC0 sprite sheet)
- All sources confirmed CC0/public domain - no licensing concerns

## Dependencies

- Godot 4.2.4 (Pinned) - uses Vulkan rendering
- Main.tscn already configured with Environment resource
- Player.tscn/Projectile.cs exist for effect attachment points

# 3. APPROACH OVERVIEW

## Chosen Approach

**Parallel execution with asset acquisition:**

1. **Lighting** - Verify SDFGI/SSAO settings, tune for performance/quality balance
2. **Audio** - Create AudioManager system + download CC0 SFX files + integrate
3. **Particles** - Create GPUParticles3D hit effect prefab + integrate with attack system
4. **Models** - Download CC0 character packs, re-export as .glb, replace placeholders

**Rationale:**
- All tasks are independent - can run in parallel
- CC0 assets are free - no budget constraints
- No external API or service dependencies
- Visual coherence requires all four working together

## Alternatives Considered

- **Models**: Procedural generation (too expensive, wrong look)
- **Audio**: Text-to-speech (inappropriate for medieval)
- **Particles**: CPUParticles (GPUParticles3D has better performance)

# 4. IMPLEMENTATION STEPS

## Step 4.1: Lighting Verification and Tuning
**Goal:** Confirm SDFGI/SSAO active, tune for demo performance
**Method:** 
1. Review Main.tscn Environment settings (already present)
2. Document config in `docs/lighting-setup.md`
3. Add performance fallback tiers for low-end GPU

Reference: Main.tscn lines 33-40 (Environment resource)

**Deliverables:**
- `docs/lighting-setup.md` (NEW or UPDATE)
- Lighting tier settings in Main.tscn Environment resource

**Estimated:** 2 hours
**Risk:** LOW - settings already present

## Step 4.2: Sound Effects Integration
**Goal:** Add combat audio feedback system
**Method:**
1. Create `AudioManager.cs` for audio resource management
2. Create `CombatAudioSystem.cs` for combat sound triggers  
3. Download CC0 SFX from OpenGameArt/Kenney (sword swing, hit impact)
4. Wire to AttackFeedbackSystem or CombatEventSystem

Reference: `src/client/src/combat/AttackFeedbackSystem.cs` (existing)

**Assets to acquire:**
- sfx_sword_swing.ogg 
- sfx_hit_flesh.ogg
- sfx_block.ogg (optional)
- sfx_death.ogg (optional)

**Deliverables:**
- `AudioManager.cs` (NEW)
- `CombatAudioSystem.cs` (NEW)
- `assets/audio/*.ogg` (DOWNLOAD)
- Main.tscn integration (attach AudioManager)

**Estimated:** 6 hours
**Risk:** LOW - standalone script, no server changes

## Step 4.3: Particle Effects System
**Goal:** Visual hit feedback (sparks, blood, impact bursts)
**Method:**
1. Create `HitEffect.tscn` with GPUParticles3D node
2. Configure ParticleProcessMaterial for burst effect
3. Create `CombatParticleSystem.cs` to manage effect spawning
4. Integrate with AttackFeedbackSystem/CombatEventSystem

Reference: Player.tscn AttackFeedback child node (existing)

**Particle types to create:**
- Hit sparks (metal on metal)
- Blood splatter (flesh hit)
- Impact dust (miss/block)

**Deliverables:**
- `HitEffect.tscn` (NEW - GPUParticles3D scene)
- `CombatParticleSystem.cs` (NEW)
- Integrate with Main.tscn

**Estimated:** 6 hours  
**Risk:** LOW - visual only

## Step 4.4: Proper 3D Character Models
**Goal:** Replace capsule placeholders with proper humanoid models
**Method:**
1. Download Quaternius Universal Base Characters (CC0)
2. Import .glb into Godot project
3. Re-export with proper settings (rig, animations)
4. Replace CapsuleMesh in Player.tscn with imported model
5. Configure AnimationTree bindings

Reference: Player.tscn lines 30-32 (current CapsuleMesh), manifest.json lines 14-108 (asset spec)

**Deliverables:**
- `assets/characters/player_male/model.glb` (IMPORT)
- Player.tscn (MODIFY - replace mesh)
- Animation retargeting config

**Estimated:** 8 hours
**Risk:** MEDIUM - requires animation retuning

## Step 4.5: Integration Testing
**Goal:** Verify all visual/audio systems work together
**Method:**
1. Run demo client in editor
2. Verify lighting renders correctly
3. Play attack animation, verify sound + particles fire
4. Verify character model animation plays correctly
5. Performance check (maintain 60fps)

**Deliverables:**
- All systems operational in Main.tscn

**Estimated:** 4 hours
**Risk:** MEDIUM - integration dependent

# 5. TESTING AND VALIDATION

## Validation Criteria

### Lighting
- [ ] SDFGI enabled (quality >= 1) - verify in WorldEnvironment
- [ ] SSAO visible - check corner darkening
- [ ] SSAO/SSIL settings documented
- [ ] No render warnings in Godot console

### Audio  
- [ ] Attack triggers sword swing sound
- [ ] Hit triggers impact sound
- [ ] No audio crackling or latency
- [ ] Sounds are mono/mixed correctly for 3D

### Particles
- [ ] Hit effect spawns at collision point
- [ ] Particles fade within 0.5s
- [ ] No particle pooling issues
- [ ] GPUParticles3D (not CPU)

### Models
- [ ] Character renders with proper mesh
- [ ] Idle animation plays
- [ ] Walk/Run animation plays  
- [ ] Attack animation plays
- [ ] No visual artifacts or z-fighting

### Performance
- [ ] Maintains 60fps with all effects active
- [ ] No memory leaks in effects system
- [ ] Particles properly clean up on entity destroy

## Acceptance Summary

All four remaining external system requirements addressed:
- Lighting verified
- Audio system created + CC0 assets acquired
- Particle effects created
- 3D character models imported

Build continues to pass (no server changes)
Demo runs in Godot editor with visual polish complete

**Plan Status:** Ready for implementation  
**Focus:** Visual and audiovisual polish for MVP completion
