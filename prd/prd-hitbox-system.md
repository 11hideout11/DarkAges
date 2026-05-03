# PRD: Server-Authoritative Hitbox System

## Introduction

The DarkAges combat system currently uses lag-compensated raycast validation but lacks proper server-side hitbox/hurtbox layers. This creates ambiguity in hit detection and limits combat depth. This PRD implements proper server-authoritative hitbox components for melee and ranged combat.

## Goals

- Add Hitbox component for attack hit regions
- Add Hurtbox component for damage-receiving regions  
- Implement server-side hitbox/hurtbox intersection validation
- Support multiple hitbox types (weapon, ability, projectile)
- Enable hitbox visualization for debugging

## User Stories

### HB-001: Hitbox Component
**Description:** As a combat designer, I need attack hit regions defined as components so hit detection is deterministic.

**Acceptance Criteria:**
- [ ] Hitbox component with shape (sphere/box), offset, size
- [ ] Attached to weapon BoneAttachment or as part of AbilityEffect
- [ ] Transform updates with parent entity animation
- [ ] Server owns hitbox state (not client input)

### HB-002: Hurtbox Component
**Description:** As a combat designer, I need vulnerability regions defined so hits register correctly.

**Acceptance Criteria:**
- [ ] Hurtbox component with shape, offset, size
- [ ] Per-entity (player has fullbody hurtbox)
- [ ] Can have multiple hurtboxes (head, body, limbs)
- [ ] Layer support for hit filtering

### HB-003: Server-Side Validation
**Description:** As a server, I need to validate hits against hitbox/hurtbox intersection.

**Acceptance Criteria:**
- [ ] LagCompensatedCombat checks hitbox/hurtbox overlap
- [ ] Historical validation uses hitbox position
- [ ] Hit registration includes hitbox source ID
- [ ] Single hurtbox hit per attack (no multi-hit)

### HB-004: Debug Visualization
**Description:** As a developer, I need to visualize hitboxes for debugging.

**Acceptance Criteria:**
- [ ] Debug draw hitbox wireframes
- [ ] Debug draw hurtbox wireframes
- [ ] Toggle via debug console command
- [ ] Color coded by type

## Functional Requirements

- FR-1: Add `Hitbox` component struct with shape, radius, localOffset
- FR-2: Add `Hurtbox` component with shape, radius, layer mask
- FR-3: HitBoxIntersection(hitbox, hurtbox) -> bool validation
- FR-4: Integrate into LagCompensatedCombat::processAttack
- FR-5: Attach hitbox to weapon skeleton or ability effect
- FR-6: Debug draw in instrumentation mode

## Non-Goals

- No client-side hitbox prediction (server authoritative)
- No dynamic hitbox resizing (fixed shapes)
- No projectile hitbox (handled separately by ProjectileSystem)
- No hitbox animation baking (runtime transforms)

## Technical Considerations

### Component Design
```cpp
// Hitbox component - attached to weapon/effect
struct Hitbox {
    float radius{0.3f};           // sphere radius
    Vec3 localOffset{0, 0, 0};    // offset from parent
    HitboxType type{HitboxType::Weapon};
    uint8_t damageMultiplier{100}; // percent
};

// Hurtbox component - attached to entity
struct Hurtbox {
    float radius{0.5f};
    Vec3 localOffset{0, 1.0f, 0}; // center of mass
    HurtboxLayer layer{HurtboxLayer::FullBody};
    uint8_t vulnerabilityMask{0xFF}; // which damage types
};

// Intersection test (sphere-sphere)
bool testHitboxHurtbox(const Hitbox& hb, const Transform& hbWorld,
                     const Hurtbox& db, const Transform& dbWorld);
```

### Integration with Existing Systems
- Extend `AttackInput` to include source hitbox ID
- Extend `HitResult` to include hit hurtbox ID
- Add hitbox transform to position history

### Performance
- Sphere-sphere test is O(1)
- Pre-filter by AABB before detailed test
- No allocation in hot path

## Success Metrics

- All attacks use hitbox/hurtbox validation
- Debug visualization functional
- No performance regression in tick time
- Hit detection parity between client prediction and server

## Open Questions

- Should hitboxes be attached to bones or entity-relative?
- Do we need hitbox scaling per ability?
- Should hurtbox layers filter by damage type (physical vs magical)?