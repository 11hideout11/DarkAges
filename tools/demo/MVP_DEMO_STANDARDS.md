# DarkAges MMO — MVP Demo Standards & Gap Analysis

**Generated:** 2026-04-25  
**Validated by:** Visual polish demo run (1978 tests, screenshots with combat UI visible)

---

## Demo Pipeline Status

| Phase | Tool | Status |
|-------|------|--------|
| Autonomous launcher | `./tools/demo/demo --quick` | PASS |
| Server build | `cmake --build build_validate` | PASS |
| Client build | `dotnet build` | PASS (0 errors) |
| Server startup | UDP health probe | PASS |
| Client connection | Auto-connect + snapshot receipt | PASS |
| Live validation | `live_client_validator.py` | PASS |
| Screenshot capture | ffmpeg/scrot | PASS |
| Instrumentation | `client_instrumentation_validator.py` | **8/8 PASS** |
| Video recording | ffmpeg x11grab | PASS |
| Report generation | Markdown + JSON | PASS |

---

## Automated Validation Criteria (Instrumented)

| Check | Threshold | Actual | Status |
|-------|-----------|--------|--------|
| Connection established | >=5 samples | 54 | PASS |
| Ground collision | Y >= -0.5 | min Y = 0.00 | PASS |
| IsOnFloor consistency | >=80% true | 100% | PASS |
| FPS average (headless) | >=5.0 | 9.9 | PASS |
| Entity visibility | >=3 entities | 13 peak | PASS |
| Prediction error | <=2.0m | 0.000m | PASS |
| Snapshot reception | >=20 ticks | 1779 | PASS |
| Camera tracking | 1-10m from player | 4.31m | PASS |

---

## Server-Side Systems (C++ / ZoneServer)

| System | Status | Notes |
|--------|--------|-------|
| UDP networking | WORKING | Port 7777, handshake + ping/pong |
| ECS entity management | WORKING | EnTT registry, 13 entities visible |
| Player spawning | WORKING | Position (0,0,0), starter kit granted |
| NPC spawning | WORKING | 5-10 NPCs per demo, red capsules |
| Snapshot replication | WORKING | 20Hz broadcast, float positions |
| Input handling | WORKING | ClientInput type=1 at 60Hz |
| Movement system | WORKING | Server-authoritative XZ, no gravity |
| Combat system | PRESENT | Not actively demoed |
| Chat system | PRESENT | Not actively demoed |
| Quest system | PRESENT | Not actively demoed |
| Inventory system | PRESENT | Not actively demoed |
| Zone events | PRESENT | Not actively demoed |
| Persistence (Redis/Scylla) | DISABLED | Build flag `-DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF` |

---

## Client-Side Systems (Godot 4.2 C#)

| System | Status | Notes |
|--------|--------|-------|
| UDP client networking | WORKING | Connection request/response, snapshot parsing |
| Client-side prediction | WORKING | Input buffer, reconciliation, 0.00m error |
| Ground collision | WORKING | StaticBody3D + WorldBoundaryShape3D |
| Player origin | WORKING | Capsule offset (0, 0.9, 0), feet at Y=0 |
| Gravity clamping | WORKING | Client Y=0 matches server (no gravity) |
| Camera follow | WORKING | SpringArm3D, 4.31m from player |
| NPC interpolation | WORKING | RemotePlayer smooth interpolation |
| Debug overlay | WORKING | FPS, RTT, entity count, prediction error |
| Auto-exit (`--demo-duration`) | WORKING | Graceful demo termination |
| Real-time instrumentation | WORKING | JSON export to `/tmp/darkages_instrumentation.json` |
| 3D models/art | BASIC | Capsule meshes only (gray player, red NPCs) |
| Animation | MINIMAL | No walk/run/attack animations |
| UI panels | MINIMAL | Connection + debug panels only |
| Inventory/loot UI | MISSING | Not implemented |
| Quest UI | MISSING | Not implemented |
| Chat UI | MISSING | Not implemented |
| Health bars | **MISSING** → **WORKING** | Billboarded 3D bars above entities (green/yellow/red) |
| NPC name labels | **PRESENT** | Label3D above RemotePlayers |
| Death/respawn | PARTIAL | DeathRespawnUI exists, not demoed |

---

## Critical Fixes Applied (This Session)

1. **Ground collision**: Added `StaticBody3D` + `CollisionShape3D` with `WorldBoundaryShape3D` to `Main.tscn`
2. **Correction packet parsing**: Fixed `PredictedPlayer.OnServerCorrection()` to parse floats (`BitConverter.ToSingle`) instead of broken int16/64 division
3. **Gravity mismatch**: Disabled client gravity, clamped Y velocity to 0 to match server `MovementSystem` behavior
4. **Player origin**: Offset `CollisionShape3D` and `MeshInstance3D` by `(0, 0.9, 0)` in both `Player.tscn` and `RemotePlayer.tscn`
5. **Client auto-exit**: Added `--demo-duration` flag to demo orchestrator so Godot client exits gracefully
6. **Real-time instrumentation**: Created `DemoInstrumentation.cs` + `client_instrumentation_validator.py`

---

## Remaining Demo Gaps (Priority Order)

### P1 — Makes the demo feel like a real game
- [x] **Player movement visualization**: Camera follows player, position changes visible
- [x] **NPC animations**: Red capsules have subtle idle bobbing + rotation
- [x] **Health bars**: Green/yellow/red billboarded bars above all entities (FIXED)
- [x] **Attack animation feedback**: Visual swing via animation state system (FIXED)
- [x] **Terrain variety**: Checkerboard terrain texture applied (FIXED)

### P2 — Feature completeness
- [ ] **Inventory panel**: Grid of item slots in GameUI
- [ ] **Quest tracker**: Active quests display
- [ ] **Chat UI**: Scrollable chat panel
- [ ] **NPC interaction**: 'E' key to interact

### P3 — Polish & production readiness  
- [ ] **Proper 3D models**: Replace capsules with actual models
- [ ] **Sound effects**: Footsteps, attacks, ambience
- [ ] **Particle effects**: Spawn/death/sparkles

---

## Running the Demo

```bash
# Quick demo (~35s, 5 NPCs, 3 screenshots)
cd /root/projects/DarkAges
./tools/demo/demo --quick

# Extended demo (~120s, 20 NPCs, 12 screenshots)
./tools/demo/demo --extended

# GUI mode (for manual inspection)
./tools/demo/demo --headed --quick

# Skip builds (fastest iteration)
./tools/demo/demo --quick --no-build
```

---

## Demo Artifacts

After each run, check:
- `tools/demo/artifacts/reports/` — Markdown report
- `tools/demo/artifacts/autonomous_demo_summary_*.json` — JSON summary
- `tools/demo/artifacts/screenshots/` — PNG screenshots
- `tools/demo/artifacts/videos/` — MP4 screen recording
- `tools/demo/artifacts/instrumentation_report.json` — Physics/network validation
- `tools/demo/artifacts/logs/` — Server + client logs
