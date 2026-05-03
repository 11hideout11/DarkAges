# DarkAges MMO

![Build and Test](https://github.com/cammurray/DarkAges/workflows/Build%20and%20Test/badge.svg)
![Redis Tests](https://github.com/cammurray/DarkAges/workflows/Redis%20Integration%20Tests/badge.svg)
![Test Coverage](https://img.shields.io/badge/coverage-99.2%25-brightgreen)
![Redis Performance](https://img.shields.io/badge/redis-232k%20ops%2Fsec-blue)
![License](https://img.shields.io/badge/license-MIT-green)

A high-density PvP MMO inspired by Dark Age of Camelot and Ark Raiders, targeting 100-1000 concurrent players per shard with zero budget.

## Project Status: NOT READY for Demo MVP (Updated 2026-04-28)

**New MVP Criteria**: Full third-person combat multiplayer template with demo zones and gameplay required.

**Previous Phases (0-9)**: ✅ Implementation Complete (~51K LOC server, ~9K LOC client)
**Current Status**: NOT READY under updated MVP criteria. Previously completed visual polish does NOT satisfy new bar.
**Server Status**: ✅ Operational (60Hz tick rate, 2129 tests passing)

See [CURRENT_STATUS.md](CURRENT_STATUS.md) for daily updates • [PROJECT_STATUS.md](PROJECT_STATUS.md) for detailed history • [PHASE8_EXECUTION_PLAN.md](PHASE8_EXECUTION_PLAN.md) for roadmap

### Completed Work Packages

**Phase 6 - External Integration:**
- ✅ **WP-6-2**: Redis Hot-State Integration (hiredis, connection pooling)
- ✅ **WP-6-4**: FlatBuffers Protocol (delta compression, binary serialization)
- ✅ **WP-6-5**: Integration Testing Framework (Docker, Python bots, CI/CD)

**Phase 7 - Client Implementation:**
- ✅ **WP-7-3**: Client Entity Interpolation (Godot C#, 100ms delay buffer)

### Phase 8 Work Packages (Completed)

|| WP | Component | Status | Agent |
||----|-----------|--------|-------|
|| WP-8-1 | Production Monitoring | ✅ Complete | DEVOPS |
|| WP-8-2 | Security Audit | ✅ Complete | SECURITY |
|| WP-8-3 | Performance Optimization | ✅ Complete | PHYSICS |
|| WP-8-4 | Load Testing | ✅ Complete | DEVOPS |
|| WP-8-5 | Documentation Cleanup | ✅ Complete | ALL |
|| WP-8-6 | GNS Full Integration | ✅ Complete | NETWORK |

### Phase 9 Work Packages (Completed)

|| WP | Component | Status |
||----|-----------|--------|
|| WP-9-1 | Performance Test Infrastructure | ✅ Complete |
|| WP-9-2 | Benchmark Runner | ✅ Complete |
|| WP-9-3 | 7 Budget Checks | ✅ All Pass |

### Phase 10 Work Packages (In Progress)

|| WP | Component | Status |
||----|-----------|--------|
|| WP-10-1 | Anti-Cheat Validation | 🔄 In Progress |
|| WP-10-2 | DDoS Protection Testing | 🔄 In Progress |
|| WP-10-3 | Fuzz Testing | ⏳ Planned |
|| WP-10-4 | Penetration Testing | ⏳ Planned |

### What's Implemented
- ✅ **Server**: ~32,000 lines (C++20, EnTT ECS, 60Hz tick, 2096 tests passing)
- ✅ **Client**: ~6,200 lines (Godot 4.2 C#, prediction, interpolation, combat UI)
- ✅ **Testing**: ~15,000 lines (Catch2, 11 suites, 2096 test cases)
- ✅ **Build System**: CMake, MSVC 2022, cross-platform CI/CD — COMPLETE
- ✅ **External Libraries**: Redis stubs, FlatBuffers, Protobuf
- ✅ **Demo Pipeline**: Autonomous launcher — build, test, validate, screenshots, video — **OPERATIONAL**
- ✅ **Combat System**: Server-authoritative with lag compensation — **OPERATIONAL**
- ✅ **Visual Polish (2026-04-25)**: Remote health bars scaled 20% + emissive boost; Player AnimationLibrary wired; fallback animation state switching — **COMPLETE**

## Quick Start

### One-Command Demo

The fastest way to see DarkAges in action:

```bash
cd DarkAges

# Quick demo (~45s): server + Godot client + validation + screenshots + video
./tools/demo/demo --quick --no-build

# Standard demo (~60s)
./tools/demo/demo --no-build

# Extended demo (~120s) with more NPCs
./tools/demo/demo --extended --no-build
```

This command automatically: validates dependencies, starts the server, launches the Godot client (headless via xvfb-run), validates network/physics/combat, captures screenshots and MP4 video, and generates a report.

### Prerequisites

- **Windows**: Visual Studio 2022 or MinGW-w64
- **Linux**: GCC 11+ or Clang 14+
- **macOS**: Xcode 14+
- CMake 3.20+
- Docker Desktop (for infrastructure)
- Godot 4.2+ (for client)
- xvfb-run, ffmpeg (for headless demo)

### Clone and Build

```bash
# Clone the repository
git clone <repository-url>
cd DarkAges

# Start infrastructure (Redis + ScyllaDB)
docker-compose up -d

# Build server (Release)
./build.sh Release --tests

# Or on Windows
.\build.ps1 Release -Tests
```

### Run the Server

```bash
# Run the zone server
./build/bin/darkages-server --port 7777 --zone 1

# Or with custom options
./build/bin/darkages-server --port 7777 --zone 1 --redis-host localhost --redis-port 6379
```

### Run the Client

1. Open Godot 4.2+
2. Import `src/client/project.godot`
3. Run the project (F5)
4. Enter server address (default: `127.0.0.1:7777`)
5. Click Connect

## Architecture

```text
┌─────────────────────────────────────────────────────────────┐
│                         CLIENT (Godot 4)                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │   Input      │  │  Prediction  │  │  Interpolation   │  │
│  │   System     │→ │   Buffer     │→ │  Remote Players  │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
└────────────────────────────────┬────────────────────────────┘
                                 │ UDP (GameNetworkingSockets)
                                 ▼
┌─────────────────────────────────────────────────────────────┐
│                    ZONE SERVER (C++/EnTT)                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │   Network    │  │   Physics    │  │   Replication    │  │
│  │   (GNS)      │→ │   (Spatial)  │→ │   (Snapshots)    │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐                         │
│  │   Movement   │  │   Anti-Cheat │                         │
│  │   System     │  │   Validator  │                         │
│  └──────────────┘  └──────────────┘                         │
└────────────────────────────────┬────────────────────────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         ▼                       ▼                       ▼
┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
│      Redis       │   │    ScyllaDB      │   │   Zone Server    │
│   (Hot State)    │   │  (Persistence)   │   │    (Shard N)     │
└──────────────────┘   └──────────────────┘   └──────────────────┘
```

## Technology Stack

| Layer | Technology | Rationale |
|-------|------------|-----------|
| **Client** | Godot 4.x | Zero licensing, adequate 3D |
| **Server** | C++20 + EnTT | Zero-overhead ECS |
| **Networking** | GameNetworkingSockets | Production-proven UDP |
| **Protocol** | FlatBuffers | Zero-copy serialization |
| **Hot State** | Redis | Sub-millisecond access |
| **Persistence** | ScyllaDB | High write throughput |
| **Physics** | Custom Kinematic | Deterministic, O(n) spatial hash |

## Project Structure

```text
C:\Dev\DarkAges\
├── src/
│   ├── client/          # Godot 4.x project
│   │   ├── src/
│   │   │   ├── networking/
│   │   │   ├── prediction/
│   │   │   └── combat/
│   │   └── project.godot
│   │
│   ├── server/          # C++ ECS server
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── ecs/     # Components
│   │   │   ├── physics/ # Spatial hash, movement
│   │   │   ├── netcode/ # GNS wrapper
│   │   │   └── zones/   # Zone server
│   │   ├── src/
│   │   └── tests/       # Catch2 tests
│   │
│   └── shared/          # Protocol definitions
│       ├── proto/       # FlatBuffers
│       └── constants/   # Shared enums
│
├── infra/               # Docker, K8s
├── tools/               # Stress tests, utilities
└── docs/                # Architecture docs
```

## Development Phases

### Completed: Phases 0-9
All core architecture, external integrations, client features, and production hardening implemented:
- ✅ Phase 0: Foundation (ECS, spatial hash, movement)
- ✅ Phase 1: Networking stubs (protocol, delta compression)
- ✅ Phase 2: Multi-player sync (AOI, replication)
- ✅ Phase 3: Combat system (lag compensation, hit detection)
- ✅ Phase 4: Spatial sharding (zones, entity migration, Aura Projection)
- ✅ Phase 5: Optimization & security (DDoS, memory pools, profiling)
- ✅ Phase 6: External Integration (Redis ✅, FlatBuffers ✅, Testing Framework ✅)
- ✅ Phase 7: Client Implementation (Interpolation ✅, partial completion)
- ✅ Phase 8: Production Hardening (Monitoring, Security, Performance, Load Testing)
- ✅ Phase 9: Performance Testing (Infrastructure, Benchmarks, Budget Validation)

**Quality Gates Passed**: Server operational at 60Hz, 798 test cases, 4,600 assertions, all budget checks pass

### Current: Phase 10 Security Testing
Final security validation before production deployment:
- 🔄 WP-10-1: Anti-Cheat Validation (speed hack, teleport, fly hack)
- 🔄 WP-10-2: DDoS Protection Testing
- ⏳ WP-10-3: Fuzz Testing (AFL++)
- ⏳ WP-10-4: Penetration Testing

**Quality Gate**: Production-ready server with comprehensive security validation

See [PHASE8_EXECUTION_PLAN.md](PHASE8_EXECUTION_PLAN.md) for detailed 8-week roadmap • [CURRENT_STATUS.md](CURRENT_STATUS.md) for daily progress

## Performance Budgets

|| Resource | Limit |
||----------|-------|
|| Tick Budget | 16.67ms (60Hz) |
|| Network Down | 20 KB/s per player |
|| Network Up | 2 KB/s per player |
|| Memory/Player | 512 KB |
|| Max Entities/Zone | 4000 (tested to 800) |

## Coding Standards

### Performance (Non-Negotiable)
```cpp
// ZERO allocations during game tick
// BAD: std::vector inside update loop
// GOOD: Reuse member variable buffer

// Cache coherence: Structure of Arrays (SoA)
// Use EnTT's storage patterns

// Determinism: Use fixed-point arithmetic
using Fixed = int32_t;  // 1000 units = 1.0f
```

### Safety
- Input validation: Clamp all floats to ±10000.0f
- Memory safety: AddressSanitizer in debug builds
- Circuit breakers: External service failures don't crash server

### Commit Messages

Format: `[AGENT] Brief description - Performance impact`

**Examples:**

- `[NETWORK] Implement delta compression - Reduces bandwidth by 80%`
- `[PHYSICS] Optimize spatial hash queries - 2x faster collision`

## Testing

### Unit Tests
```bash
# Build and run tests
./build.sh Debug --tests
ctest --test-dir build --output-on-failure
```

### Stress Testing
```bash
# Run Python bot swarm
cd tools/stress-test
python bot_swarm.py --count 100 --duration 300
```

## Documentation

- [Implementation Roadmap](ImplementationRoadmapGuide.md) - Technical specs
- [AI Coordination](AI_COORDINATION_PROTOCOL.md) - Multi-agent workflow
- [Research](ResearchForPlans.md) - Architectural decisions
- [Agent Instructions](AGENTS.md) - Development guidelines

## License

MIT License - See LICENSE file for details

## Contributing

This project follows a structured AI agent workflow. See [AI_COORDINATION_PROTOCOL.md](AI_COORDINATION_PROTOCOL.md) for contribution guidelines.

---

**Remember**: *The client is the enemy. Validate everything. Trust nothing.*
