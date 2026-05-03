---
name: darkages-networking
type: knowledge
version: 1.0.0
agent: CodeActAgent
triggers:
  - network
  - multiplayer
  - protocol
  - entt
---
# DarkAges Networking + ECS

## Project Context
- **Status**: MVP READY - Core networking working
- **PRD Status**: PRD-001 (Server Core), PRD-002 (Networking), PRD-004 (Sharding) ALL COMPLETE
- **Reference**: `AGENTS.md` for authoritative project state

## Server-authoritative model
- Client: sends intent (move, attack, cast)
- Server: validates, applies to ECS, broadcasts snapshot
- Client: interpolates between snapshots, renders state

## Tick Loop
- Fixed timestep: 60 Hz (16.666 ms per tick)
- Order: Network receive → ECS state advance → Snapshot generation → send
- Budget: entire tick must finish in < 20 ms (with headroom)

## Zone Architecture
- ZoneServer: single ECS world, ~400 entities max
- ZoneOrchestrator: manages multiple ZoneServer instances; handles handoffs
- EntityMigration: portable state shipping when player crosses zone boundary

## Packet Design
- Bitfield-coded enum field for packet type (uint8, bit shifts)
- Delta compression on snapshots (only changed components)
- Sequence numbers + ACKs for reliability (custom layer over UDP)

## Validators
- MovementValidator: checks speed/position validity (against max velocity, teleport-dist)
- StatisticalDetector: flags anomalous stats (averages, distributions) for anti-cheat review
- RateLimiter: per-action cooldowns enforced server-side

## Gap Notes
- GNS Runtime: BLOCKED (WebRTC auth issue) -PRD-012
- Production DB: BLOCKED (Docker) - PRD-018
- These are external blockers, not networking implementation gaps
