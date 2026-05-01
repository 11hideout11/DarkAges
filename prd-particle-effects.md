# PRD: Particle Effects System

## Introduction

DarkAges has no particle effects system for environmental ambiance, combat effects, or ambient atmosphere. This PRD implements basic particle effects.

## Goals

- Ambient particles (dust, leaves)
- Combat particles (hit effects)
- Weather particles

## User Stories

### PART-001: Ambient Particles
**Description:** As a player, I want ambient particles.

**Acceptance Criteria:**
- [ ] Dust motes in zones
- [ ] Firefly effects at night
- [ ] Water splash near water

### PART-002: Combat Particles
**Description:** As a player, I want combat particles.

**Acceptance Criteria:**
- [ ] Hit effects (spark, blood)
- [ ] Trail effects on projectiles
- [ ] Buff/debuff visuals

### PART-003: Weather Particles
**Description:** As a player, I want weather particles.

**Acceptance Criteria:**
- [ ] Rain effect
- [ ] Snow effect
- [ ] Zone-based weather

## Functional Requirements

- FR-1: Particle node implementation
- FR-2: Pre-configured particle scenes
- FR-3: Weather system integration

## Non-Goals

- Complex custom shaders
- GPU particles
- Real-time weather API

## Technical Considerations

### Godot GPUParticles3D
- Use GPUParticles3D for performance
- Collision with ground
- Fade over lifetime

## Success Metrics

- Particles visible in zone
- Combat effects display

## Open Questions

- Particle counts?
- Performance budget?