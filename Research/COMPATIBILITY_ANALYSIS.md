# DarkAges UE5.5+ Combat Standards Alignment

## Executive Summary

This document maps the existing DarkAges codebase to the UE5.5+/GASP/ALGS combat standards defined in `Research/ThirdPersonCombatStandardsResearch/`. It identifies implemented features, gaps, and prioritized action items to achieve industry-standard quality.

---

## Part 1: Research Standards Summary

### Core Architecture (from `GodotMultiPlayerCombatSystemDirective.txt`)

| Standard | Description | Priority |
|----------|------------|----------|
| Server-Authoritative | MultiplayerSynchronizer + RPC validation | Critical |
| CharacterBody3D + FSM | Node-based finite state machine | Critical |
| AnimationTree | State Machine + BlendTree for procedural | High |
| SDFGI/SSAO/SSIL | AAA cinematic lighting | High |
| Phantom Camera | PCam for 3rd-person follow/lock-on | Medium |

### Combat Protocol (from `CoreFrameworksCodeReferences.md`)

```
Client (Local): Visual Prediction → RPC request → Server Validation → Sync variable → Remote clients animate
```

### Visual Coherence (from `GodotSourceAssets.txt`)

- Foot IK (SkeletonIK3D) for terrain alignment
- Procedural leaning based on velocity
- Floating combat text for damage numbers

---

## Part 2: Current Implementation Mapping

### ✅ Implemented Systems

| Research Standard | DarkAges Implementation | Status |
|-----------------|----------------------|--------|
| Server-Authoritative | `PredictedPlayer.cs` reconciliation (WP-7-2) | Complete |
| CharacterBody3D Movement | `PredictedPlayer._PhysicsProcess()` | Complete |
| Input Prediction Buffer | 120 input buffer (2s @ 60Hz) | Complete |
| AnimationTree | `Player.tscn` + `PlayerAnimations.tres` | Complete |
| Dodge System | Q-key dodge with invulnerability frames | Complete |
| Attack System | Attack animation + timing | Complete |
| Hit Reaction | `TriggerHitReaction()` method | Complete |
| Death/Respawn | `TriggerDeath()`, `TriggerRespawn()` | Complete |
| Multiplayer Sync | `RemotePlayerManager` + ghost position | Complete |
| Smooth Correction | Lerp-based error correction | Complete |
| Client Prediction | Immediate local execution | Complete |

### ⚠️ Partial Implementation

| Research Standard | DarkAges Implementation | Gap |
|-------------------|----------------------|-----|
| Formal FSM Structure | Inline state flags | No node-based FSM |
| Phantom Camera | Basic SpringArm3D | No PCam plugin |
| Foot IK | None | No terrain alignment |
| Procedural Leaning | None | No velocity-based tilt |
| Hitbox/Hurtbox Layers | Single collision | Layer 3/4 separation |
| Floating Combat Text | `DamageNumber.cs` exists | Not integrated |

### ❌ Not Implemented

| Research Standard | Status | Action |
|-----------------|--------|--------|
| SDFGI/SSAO Post-Processing | Not configured | Add WorldEnvironment |
| State Charts Plugin | Not installed | Evaluate |
| Mixamo Animations | Kenney placeholders | Consider upgrade |
| Lock-on Camera | Basic follow only | Use Phantom Camera |
| Target Frame UI | Partially done | Complete target lock |

---

## Part 3: Gaps & Prioritized Action Items

### Priority 1: Networking & Combat Logic (Critical)

| Gap | Current | Target | Action |
|-----|---------|--------|--------|
| Hitbox/Hurtbox | Layer 3=Hitbox, Layer 4=Hurtbox (implemented) | Layer 3=Hitbox, Layer 4=Hurtbox | ✅ Completed — Area3D nodes, collision layers, client prediction |
| Server Validation | Server-authoritative RPC handshake (implemented) | Server-authoritative damage | ✅ Completed — CombatActionPacket RPC with server validation and result response |
| GCD | 1.2s global cooldown enforced (implemented) | 1.2s global cooldown | ✅ Completed — server `canAttack` GCD gate, client-side GCD timer, RPC result codes 3/4 |

### Priority 2: Animation & Feel (High)

|| Gap | Current | Target | Action | Status |
|-----|---------|--------|--------|--------|
| Procedural Leaning | None → Added `UpdateProceduralLeaning()` with configurable MaxLeanAngle (12°), smooth lerp on velocity | Spine tilt on velocity | Add to PredictedPlayer | ✅ Done — commit pending |
| Foot IK | None | SkeletonIK3D terrain alignment | Install Advanced Foot IK | ⏸️ Deferred — requires skeletal rig |
| Animation Blend | Basic transitions → Crossfade `Play(anim, blendTime)` (0.15s), AnimationBlendTime export | BlendTree/smooth crossfade | Enhance AnimationTree | ✅ Done — commit pending |
| Hit Stop | None → TimeScale freeze (0.05s @ 0.1x) + camera shake | 0.05s freeze on hit | Implement in CombatEventSystem | ✅ Done — commit pending |

### Priority 3: Visual Quality (Medium)

|| Gap | Current | Target | Action | Status |
|-----|---------|--------|--------|--------|
| SDFGI | Enabled (Main.tscn Environment_default) | SDFGI for GI | Configuration | ✅ Done |
| SSAO | Enabled (Main.tscn Environment_default) | SSAO for AO | Configuration | ✅ Done |
| Floating Text | ✅ Completed — CombatEventSystem.SpawnDamageNumber called on local DamageTaken | Full integration | ✅ Completed — floating damage number shows when local player takes damage (PR #20) | ✅ Done |

### Priority 4: Polish (Nice to Have)

| Gap | Current | Target | Action |
|-----|---------|--------|--------|
| Phantom Camera | SpringArm3D | Install PCam plugin | Optional |
| State Charts | Inline | Install Godot State Charts | Evaluate |

---

## Part 4: Architectural Consistency Checklist

### Multiplayer Authority (from `CoreFrameworksCodeReferences.md`)

```csharp
// Every function modifying game state MUST include:
if (!is_multiplayer_authority()) return;
```

✅ Already implemented in `PredictedPlayer.cs` methods where appropriate

### AnimationTree Parameters (from research)

Research specifies using `parameters/Sprinting`, `parameters/Jumping`, etc.

✅ Already implemented in `UpdateAnimation()`:
```csharp
_animTree.Set("parameters/Sprinting", isSprinting);
_animTree.Set("parameters/Jumping", isJumping);
_animTree.Set("parameters/Attacking", _isAttacking);
```

### RPC Pattern (Combat Handshake)

Research pattern:
```
1. Local: execute_local_visuals() → Predict
2. Client: rpc_id(1, "request_attack", type, timestamp)
3. Server: validate and sync
4. Remote: observe and animate
```

⚠️ Current: Client predicts locally but validation is incomplete

---

## Part 5: Recommended Implementation Order

### Week 1: Combat Foundation
1. Add Hitbox Area3D with Layer 3
2. Add Hurtbox Area3D with Layer 4
3. Implement server-authoritative damage RPC
4. Add Global Cooldown (1.2s)

### Week 2: Animation Polish
1. Add procedural leaning in PredictedPlayer
2. Enhance AnimationTree with BlendTree
3. Add Hit Stop effect (0.05s)
4. Integrate floating combat text

### Week 3: Visual Quality
1. Configure WorldEnvironment with SDFGI
2. Enable SSAO post-processing
3. Verify lighting coherence

### Week 4: Camera (Optional)
1. Evaluate Phantom Camera plugin
2. Implement lock-on targeting

---

## Part 6: Code References

| File | Purpose | Research Alignment |
|------|---------|------------------|
| `PredictedPlayer.cs` | Core movement, prediction | WP-7-2 ✅ |
| `RemotePlayerManager.cs` | Multiplayer sync | MultiplayerSynchronizer ✅ |
| `CombatEventSystem.cs` | Combat events | Combat handshake ⚠️ |
| `AttackFeedbackSystem.cs` | Attack visualization | Visual prediction ✅ |
| `DamageNumber.cs` | Floating text | Floating combat text ⚠️ |
| `TargetLockSystem.cs` | Target selection | Lock-on system ⚠️ |

---

## Appendix A: Research Source Files

| File | Key Content |
|------|------------|
| `CoreFrameworksCodeReferences.md` | Snaiel prototype, Liblast networking |
| `GodotMultiPlayerCombatSystemDirective.txt` | Master agent directive |
| `GodotSourceAssets.txt` | Plugin recommendations |
| `JakubWTemplateExampleAnalysis.txt` | AGLS/GASP reference |
| `GodotSourceAssets - Copy.txt` (corrupted) | N/A |

## Appendix B: External References

| Resource | URL | Use |
|----------|-----|-----|
| Snaiel Combat Prototype | github.com/Snaiel/Godot4ThirdPersonCombatPrototype | Combat FSM reference |
| Liblast | Open source | Multiplayer networking |
| Phantom Camera | github.com/ramokz/phantom-camera | Camera system |
| Godot State Charts | github.com/derkork/godot-statecharts | FSM alternative |
| Mixamo | mixamo.com | Animation library |
| Kenney Assets | kenney.nl | Placeholder visuals |

---

## Appendix C: Key Code Templates (from Research)

### Combat State Machine Base

```csharp
// From research - Node-based FSM
public class State : Node
{
    public signal finished(string next_state_name);
    public CharacterBody3D character;
    
    public virtual void Enter() {}
    public virtual void Exit() {}
    public virtual void Update(double delta) {}
    public virtual void HandleInput(InputEvent @event) {}
}
```

### Hit Detection Template (Server)

```csharp
// From research - Server-side damage
public void OnHitboxAreaEntered(Area3D area)
{
    if (!is_multiplayer_authority()) return;
    
    if (area.IsInGroup("hurtbox"))
    {
        var victim = area.Owner;
        if (victim != attacker)
        {
            // Calculate damage with armor formula
            float damage = CalculateDamage(attackerStats, victimStats);
            victim.ApplyDamage(damage);
        }
    }
}
```

---

*Generated from Research/ThirdPersonCombatStandardsResearch/ on 2026-04-24*