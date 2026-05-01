# DarkAges Collision Matrix

## Overview
This document describes the collision layer system for hitbox/hurtbox detection in the DarkAges MMO server.

## Collision Layers

| Layer | Bit Position | Purpose |
|-------|-------------|---------|
| Default | 1 << 0 | Environment collision |
| Player | 1 << 1 | Player character body |
| NPC | 1 << 2 | Non-player characters |
| Hitbox | 1 << 3 | Attack hitboxes |
| Hurtbox | 1 << 4 | Vulnerable hit areas |
| Projectile | 1 << 5 | Ranged attacks |
| Trigger | 1 << 6 | Trigger volumes |

## Collision Matrix

### Who Collides With Whom

| Collider \ Target | Player | NPC | Hitbox | Hurtbox | Projectile | Trigger |
|-----------------|--------|-----|-------|--------|----------|---------|
| Player | ✓ | ✓ | - | - | ✓ | ✓ |
| NPC | ✓ | ✓ | - | - | ✓ | ✓ |
| Hitbox | - | - | - | ✓ | - | - |
| Hurtbox | - | - | ✓ | - | - | - |
| Projectile | ✓ | ✓ | - | - | - | ✓ |
| Trigger | ✓ | ✓ | - | - | - | - |

### Team Filtering

- **Same team collisions**: Blocked (no friendly fire within team)
- **Opposite teams**: Allowed
- **Neutral team**: Collides with all (environmental/projectiles)

## Hitbox Validation Rules

### Server-Authoritative Checks

1. **Layer Intersection**: Hitbox layer must intersect Hurtbox layer
2. **Team Filtering**: Same team = no collision (except Neutral)
3. **Spatial Query**: BroadPhaseSystem provides candidates
4. **Fine Check**: Cylinder-sphere intersection test
5. **Cone Check**: Target must be in attacker's melee cone

### Melee Cone

- Default cone angle: 120 degrees
- Half-cone from attacker's facing direction
- Yaw-only (2D) - vertical angle not considered

### Edge Cases

| Edge Case | Handling |
|----------|----------|
| Zero-size hitbox | Rejected (minimum 0.1m radius) |
| Extreme distance | Rejected (>50m auto-miss) |
| IFrames | Invincibility frames block hits |
| Multihit limit | Max 1 hit per attack frame |
| Lag rewind | Server validates historical position |

## Anti-Cheat Validation

1. **Impossible damage**: Server validates hit timing
2. **Speed hacks**: Block attacks during teleport
3. **wallhacks**: Server doesn't trust client position

---

**Last Updated:** 2026-05-01