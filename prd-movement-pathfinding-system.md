# PRD: Movement & Pathfinding System

## Introduction

Implement a comprehensive movement and pathfinding system that handles player navigation, collision, path calculation, movement abilities (dashes, teleports, mounts), swimming/flying, and AI navigation for mobs. The system should support both player characters and AI pathfinding within the zone.

## Goals

- Add MovementComponent for velocity, position, rotation, state (walking, running, swimming, flying)
- Implement pathfinding using A* or navigation grid for AI
- Add movement modifiers (speed boosts, dash, teleport)
- Add mount system for faster travel
- Implement collision detection with terrain and objects
- Add client-side prediction for smooth movement

## User Stories

### US-001: Basic player movement
**Description:** As a player, I want to move around the world so I can explore and engage in combat.

**Acceptance criteria:**
- [ ] Position stored server-authoritative
- [ ] Speed: walk (4 m/s), run (8 m/s), sprint (12 m/s)
- [ ] Client sends input (direction, jump, sprint key)
- [ ] Server calculates position, sends update
- [ ] Smooth interpolation on client

### US-002: Pathfinding for AI
**Description:** As a game designer, I want mobs to pathfind so they can chase players intelligently.

**Acceptance Criteria:**
- [ ] Navigation grid per zone (or navmesh coordinates)
- [ ] A* pathfinding to target position
- [ ] Path replanning on target movement
- [ ] Obstacle avoidance
- [ ] Path smoothing for natural movement

### US-003: Movement abilities
**Description:** As a player, I want movement abilities so I can navigate efficiently.

**Acceptance Criteria:**
- [ ] Dash: instant forward movement (5-15m)
- [ ] Teleport: blink to target location
- [ ] Mount: ride mounts for speed boost (2x)
- [ ] Jump: vertical movement for obstacles
- [ ] Fall damage at > 10m drop

### US-004: Swimming and flying
**Description:** As a player, I want to swim and fly so I can access all areas.

**Acceptance Criteria:**
- [ ] Swimming state in water zones
- [ ] Reduced movement speed in water
- [ ] Flight mode for flying mounts/zones
- [ ] Altitude limits per zone
- [ ] Flight stamina (exhausts over time)

### US-005: Collision handling
**Description:** As a game designer, I want collision so players can't walk through objects.

**Acceptance Criteria:**
- [ ] Terrain collision per heightmap
- [ ] Object collision boxes
- [ ] Player-to-player collision (optional)
- [ ] Slide along obstacles
- [ ] Pushback from NPCs

## Functional Requirements

- FR-1: MovementComponent with pos, vel, rotation, state
- FR-2: PathfindingComponent generates paths
- FR-3: CollisionSystem checks bounds
- FR-4: MovementAbilityComponent for dash/teleport
- FR-5: MountComponent for mounted movement
- FR-6: SwimmingComponent and FlightComponent states
- FR-7: SpeedModifierComponent (buffs/debuffs)

## Non-Goals

- No mount breeding/rearing
- No vehicle physics (single player mounts)
- No swimming combat (basic movement only)
- No wall-running or parkour

## Technical Considerations

- Position updated at 60 Hz tick
- Pathfinding run every 0.5s for AI
- Movement predictions for remote players

## Success Metrics

- Movement latency < 50ms
- 100 AI entities pathfinding smoothly
- No wall clipping exploits

## Open Questions

- Navigation: grid-based or mesh?
- Mounts: item-summoned or always available?
- Collision: server-side or client-predicted?