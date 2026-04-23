# DarkAges MMO — Guarded Demo Pipeline & Agent Skill Master Plan v2
**Date:** April 23, 2026
**Status:** Authoritative — Implementation ~70% complete, remaining 30% detailed below
**Scope:** Installer, harness, agent skills, MCP integration, redundancy, server-client-demo orchestration

---

## Executive Summary

This plan defines the complete end-to-end system required to showcase the DarkAges MMO project in a demo-ready state, comparable to an Unreal Engine sample project demo. It covers:

- A **guarded installer** that validates, builds, tests, and deploys without human intervention.
- A **redundant harness** that supervises the server and Godot client, auto-recovers from crashes, and gates on health checks.
- An **agent skill ecosystem** (Hermes-compatible) allowing AI agents to start, stop, query, and interact with every component.
- **Godot client control** via both native MCP server integration and CLI fallback.
- **Demo content pipeline** with curated zones, NPC presets, quest chains, and scripted narration.
- **Validation & reporting** that proves every run worked with artifacts, screenshots, and metrics.

**Current maturity:** Core pipeline operational (`run_demo.py --smoke` passes in ~35s). Remaining work focuses on visual client automation, MCP server completion, chaos resilience, live dashboard, and advanced reporting.

---

## I. Current State Inventory

### Verified Operational (Do Not Rebuild)

| Component | Location | Evidence |
|-----------|----------|----------|
| C++ Server (stubbed UDP) | `build_validate/darkages_server` | 1212 tests pass, live validator connects |
| Godot 4.2.2 Mono | `/usr/local/bin/godot` | `godot --version` returns 4.2.2.stable.mono |
| C# Client | `src/client/` | `godot --headless --build-solutions` exits 0 |
| Dependency Checker | `tools/demo/harness/dependency_check.py` | 8-point validation, JSON output |
| Build Manager | `tools/demo/harness/build_manager.py` | CMake wrapper with auto-retry |
| Test Runner | `tools/demo/harness/test_runner.py` | CTest wrapper with flaky retry |
| Process Supervisor | `tools/demo/harness/supervisor.py` | PID watch, UDP health probe, auto-restart |
| Godot Controller | `tools/demo/harness/godot_controller.py` | Headless/headed launch, xvfb fallback |
| E2E Validator | `tools/demo/harness/e2e_validator.py` | UDP ping, handshake, snapshot rate, log scan |
| Master Runner | `tools/demo/run_demo.py` | `--smoke`, `--server-only`, `--headless-client`, `--full` |
| Installer | `tools/demo/install.py` | One-time setup, state tracking |
| Demo Content | `tools/demo/content/` | `demo_zone.json`, `demo_script.md` |
| Agent Skills (5x) | `~/.hermes/skills/` | Harness, server lifecycle, Godot control, narrator, validation |

### Partial / Scaffolded

| Component | Location | Gap |
|-----------|----------|-----|
| Report Generator | `harness/report_generator.py` (concept) | No Markdown/PDF generation yet |
| Log Aggregator | (not yet created) | No real-time colorized tail |
| Metrics Collector | (not yet created) | No Prometheus/proc polling |
| Local Chaos | (not yet created) | No bare-metal fault injection |
| Terminal Dashboard | (not yet created) | No `rich` live UI |
| Godot MCP Server | `tools/automated-qa/godot-mcp/` | Client exists; Node.js server not installed/running |
| Demo Zone Loader | `content/demo_zone.json` | Server does not auto-load JSON configs |
| Screenshot Capture | `godot_controller.py` | Relies on OS tools not yet integrated |
| Video Recording | `run_demo.py --record` (planned) | `ffmpeg` x11grab not wired |

### External Blockers (Out of Scope)

| Blocker | Reason |
|---------|--------|
| GNS full build | WebRTC submodule (`webrtc.googlesource.com`) requires authenticated access. Stubbed UDP layer is sufficient for demo. |

---

## II. Target Architecture: Guarded Demo Pipeline

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         AGENT INTERFACE (Hermes Skills)                      │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐       │
│  │ demo-harness │ │server-lifecycle│ │godot-control │ │  validation  │       │
│  │   (master)   │ │  (start/stop)  │ │(launch/query)│ │   (report)   │       │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘       │
├─────────┴────────────────┴────────────────┴────────────────┴────────────────┤
│                      MASTER RUNNER (run_demo.py)                             │
│   --smoke | --server-only | --headless-client | --full | --chaos | --record  │
├─────────────────────────────────────────────────────────────────────────────┤
│                         ORCHESTRATOR / SUPERVISOR                            │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │ Process Spawn │ Health Probes (UDP/PID/FPS) │ Circuit Breaker    │    │
│  │ Log Tail      │ Auto-Restart (max 3)        │ Graceful Teardown  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────────────────────┤
│                         HARNESS LAYER                                         │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐  │
│  │depends  │ │ build   │ │ tests   │ │ godot   │ │e2e val  │ │ report  │  │
│  │ check   │ │ manager │ │ runner  │ │control  │ │ idator  │ │ gen     │  │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘  │
├─────────────────────────────────────────────────────────────────────────────┤
│                         GODOT CONTROL (Dual Path)                            │
│  ┌─────────────────────────────┐  ┌─────────────────────────────────────┐  │
│  │ PATH A: MCP Server          │  │ PATH B: CLI Fallback                │  │
│  │ bradypp/godot-mcp adapted   │  │ godot --script / --headless         │  │
│  │ stdio transport, 7+ tools   │  │ GDScript execution, screenshot OS   │  │
│  │ Screenshot via grim/scrot   │  │ Input via xdotool / pynput          │  │
│  └─────────────────────────────┘  └─────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────────────┤
│                         RUNTIME PROCESSES                                    │
│  ┌─────────────────────┐      ┌─────────────────────┐      ┌─────────────┐ │
│  │ darkages_server     │◄────►│  Godot 4.2 Client   │      │  Validator  │ │
│  │ --port 7777 --npcs  │ UDP  │  C# + GDScript      │      │  (Python)   │ │
│  └─────────────────────┘      └─────────────────────┘      └─────────────┘ │
├─────────────────────────────────────────────────────────────────────────────┤
│                         CONTENT & ARTIFACTS                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │demo_zone.json│ │demo_script.md│ │screenshots/ │ │reports/ recordings/ logs││
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## III. External Tool Research & Selection

### Godot Agent Control Landscape

| Project / Tool | Transport | Linux | Headless | Effort | Verdict |
|----------------|-----------|-------|----------|--------|---------|
| **bradypp/godot-mcp** | stdio (Node.js) | Patch needed | Partial | Medium | **Primary path** — implements scene query, node tree, script exec |
| **Coding-Solo/godot-mcp** | stdio | Unknown | Unknown | High | Skip — unmaintained, no docs |
| **GDAI / Godot AI Tools** | Proprietary | N/A | N/A | High | Skip — commercial |
| **Godot CLI + `--script`** | CLI | Yes | Yes | Low | **Mandatory fallback** — always works |
| **Godot Remote Debug (TCP 6007)** | TCP | Yes | Yes | Medium | **Secondary fallback** — GDScript remote exec |
| **xdotool / pynput** | X11 | Yes | No | Low | **Headed input only** — keyboard/mouse injection |
| **ffmpeg x11grab** | CLI | Yes | No | Low | **Video capture** — for headed demos |
| **grim / scrot / import** | CLI | Yes | N/A | Low | **Screenshots** — Wayland/X11 respectively |

### MCP Server Selection: bradypp/godot-mcp

**Why:** It is the most mature open-source Godot MCP available. It exposes:
- `get_scene_tree` — Query node hierarchy
- `execute_gdscript` — Run arbitrary GDScript
- `get_project_settings` — Read engine.cfg equivalents
- `get_node_property` / `set_node_property` — Inspect/modify nodes
- `take_screenshot` — Trigger OS screenshot (needs Linux patch)
- `play_scene` / `stop_scene` — Scene lifecycle

**Adaptation required for Linux:**
1. Replace Windows path separators and drive letters.
2. Replace screenshot backend (`printWindow` → `grim` or `scrot`).
3. Ensure Godot binary path is configurable via `GODOT_PATH`.
4. Add `chmod +x` guard for self-contained Godot exports.

**Registration:** Via Hermes `native-mcp` skill in `~/.config/hermes/mcp_config.json` or `mcporter` CLI.

### Agent Skills Inventory

| Skill Name | Category | Status | Purpose |
|------------|----------|--------|---------|
| `darkages-demo-harness` | Demo | **Created** | Master pipeline orchestration |
| `darkages-server-lifecycle` | Demo | **Created** | Start/stop/monitor server |
| `godot-demo-control` | Demo | **Created** | Launch/build/query Godot |
| `darkages-demo-narrator` | Demo | **Created** | Scripted showcase narration |
| `darkages-validation-suite` | Demo | **Created** | E2E validation + reports |
| `native-mcp` | Hermes | Built-in | Register/list/call MCP servers |
| `mcporter` | Hermes | Built-in | Ad-hoc MCP server bridge |
| `cpp-build-fix` | Dev | Built-in | Fix C++ compilation errors |
| `godot-4-csharp-build-fixes` | Dev | Built-in | Fix Godot C# errors |
| `darkages-codebase-conventions` | Dev | Built-in | Namespace, EnTT, CMake gotchas |
| `autonomous-codebase-iteration` | Dev | Built-in | Non-destructive improvement loops |

---

## IV. Phase-by-Phase Implementation (Remaining Work)

### PHASE 0: Foundation Hardening (DONE — Maintenance Only)

All core harness modules are operational. Maintenance tasks:
- [ ] Pin Godot version in `dependency_check.py` (currently checks >= 4.2.2, pin to exact)
- [ ] Add `node --version` check to `dependency_check.py` (for MCP path)
- [ ] Add `ffmpeg` optional check (for `--record`)
- [ ] Add `grim`/`scrot` optional check (for screenshots on Linux)

### PHASE 1: Godot MCP Server Integration (Priority 1 — 6 hours)

**Goal:** Agent can query and control Godot via MCP tools, not just CLI subprocess.

#### 1.1 Install & Patch MCP Server
```bash
# Destination
mkdir -p /root/projects/DarkAges/tools/external
cd /root/projects/DarkAges/tools/external
git clone https://github.com/bradypp/godot-mcp.git
cd godot-mcp
npm install
```

**Required patches:**
- File: `src/index.ts` or built JS — replace hardcoded Windows paths with `process.env.GODOT_PATH`.
- File: `src/tools/screenshot.ts` — replace `printWindow` with `child_process.execSync("grim ...")` or `scrot`.
- Add Linux Godot binary detection: check `/usr/local/bin/godot`, `/usr/bin/godot`, `$GODOT_PATH`.

#### 1.2 Build & Verify
```bash
npm run build
node build/index.js --help
```

#### 1.3 Register with Hermes
Create or update `~/.config/hermes/mcp_config.json`:
```json
{
  "mcpServers": {
    "godot-darkages": {
      "command": "node",
      "args": ["/root/projects/DarkAges/tools/external/godot-mcp/build/index.js"],
      "env": {
        "GODOT_PATH": "/usr/local/bin/godot",
        "PROJECT_PATH": "/root/projects/DarkAges/src/client",
        "DEBUG": "true"
      },
      "transport": "stdio"
    }
  }
}
```

#### 1.4 Validate Tool Discovery
```bash
# Via mcporter
mcporter list-servers
mcporter call godot-darkages get_scene_tree

# Or via native-mcp skill (Hermes auto-discovers)
```

#### 1.5 Fallback Guarantees
If MCP server fails to build or connect:
- `godot_controller.py` remains the authoritative interface.
- `run_demo.py` detects MCP absence and degrades to CLI-only mode.
- No pipeline failure due to MCP issues.

#### 1.6 Skill Update: `godot-demo-control`
Add MCP-aware procedures:
- If MCP server registered → use `get_scene_tree`, `execute_gdscript` for state queries.
- If MCP unavailable → use `godot --script` fallback.
- Always prefer MCP for complex queries; always use CLI for launch/stop.

---

### PHASE 2: Redundancy & Failure Recovery (Priority 1 — 6 hours)

**Goal:** Demo survives common failures without human intervention.

#### 2.1 Supervisor Enhancement (`harness/supervisor.py`)

Current state: Auto-restart on crash, UDP health probe.

Additions:
- **Client health probe:** If Godot client is running, verify it still writes to stdout every 5s. If silent for 15s → restart client.
- **Server zombie detection:** If PID exists but port 7777 does not respond → kill zombie, restart.
- **Port escalation:** If 7777 is taken, try 7778, 7779, up to 7799. Write active port to `artifacts/active_port.txt` so validator and client can read it.
- **Memory guard:** Poll `psutil.virtual_memory()`. If > 85% → emit warning; if > 92% → kill client first, then server, with failure report.
- **Circuit breaker:** If > 3 restarts in 60s → halt entire demo with diagnostic dump.

#### 2.2 Local Chaos Monkey (`harness/local_chaos.py`)

New module. Actions (only active with `--chaos` flag):

| Action | Method | Verification |
|--------|--------|--------------|
| `kill_server_graceful()` | `SIGTERM` | Supervisor restarts within 5s |
| `kill_server_force()` | `SIGKILL` | Supervisor restarts within 10s |
| `network_delay()` | `tc qdisc add dev lo netem delay 100ms` | Validator RTT increases |
| `network_loss()` | `tc qdisc add dev lo netem loss 5%` | Validator detects loss |
| `network_recover()` | `tc qdisc del dev lo root` | RTT returns to baseline |
| `cpu_spike()` | `stress-ng --cpu 4 --timeout 10s` | Server tick time increases |
| `client_kill()` | `SIGKILL` to Godot PID | Supervisor restarts client |

**Safety:**
- Check `os.geteuid() == 0` before `tc` commands; skip if not root.
- Check `stress-ng` is installed; skip if missing.
- Never run chaos in `--smoke` mode.
- Always call `network_recover()` at end of demo.

#### 2.3 Client Reconnection Logic

If server restarts mid-demo:
- Supervisor kills existing client (to force clean state).
- Supervisor restarts server on new/old port.
- Supervisor restarts client with updated `--port` argument.
- E2E validator detects reconnection and continues tracking.

---

### PHASE 3: Observability & Dashboard (Priority 2 — 4 hours)

#### 3.1 Log Aggregator (`harness/log_aggregator.py`)

New module. Responsibilities:
- Launch threads that `tail -f` server log and client stdout.
- Color-code lines: ERROR = red bold, WARN = yellow, INFO = white, METRIC = cyan.
- Detect critical patterns and emit immediate alerts:
  - `segmentation fault` → red alert, capture backtrace
  - `assertion failed` → red alert
  - `bind(): Address already in use` → yellow alert, port escalation
  - `Server tick exceeded budget` → yellow alert
- Write combined log to `artifacts/logs/demo_YYYY-MM-DD_HH-MM-SS.log`.

#### 3.2 Metrics Collector (`harness/metrics_collector.py`)

New module. Poll every 2 seconds:
- **Server:** Parse stdout for `tick_duration_ms`, `entity_count`, `snapshot_rate` (if logged). Fallback: parse `/proc/<pid>/status` for memory.
- **Client:** Parse Godot stdout for `FPS` (if GDScript prints it).
- **System:** `psutil.cpu_percent()`, `psutil.virtual_memory().percent`.
- Write JSON timeseries: `artifacts/metrics/metrics_YYYY-MM-DD_HH-MM-SS.json`.

#### 3.3 Terminal Dashboard (`harness/dashboard.py`)

New module. Uses `rich` library (add to `requirements.txt` or pip install).

Layout:
```
┌─ DarkAges Demo Dashboard ─────────────────────────────────────────┐
│ Server: ● RUNNING (PID 12345)  Tick: 12ms  Entities: 42          │
│ Client: ● RUNNING (PID 12346)  FPS: 60                             │
│ Uptime: 00:02:34  Port: 7777  Mode: headless-client                │
├─ Metrics (last 60s) ──────────────────────────────────────────────┤
│ [sparkline tick duration]  [sparkline memory]  [sparkline CPU]   │
├─ Latest Logs ─────────────────────────────────────────────────────┤
│ 14:32:01 [INFO]  Server started on port 7777                     │
│ 14:32:02 [INFO]  Client connected, entity ID: 6                  │
│ 14:32:05 [WARN]  Godot shader compilation stalled                │
└───────────────────────────────────────────────────────────────────┘
```

- Updates every 1 second.
- Press `q` or Ctrl+C to exit with graceful teardown.
- Integrate into `run_demo.py --dashboard`.

---

### PHASE 4: Demo Content & Asset Pipeline (Priority 2 — 4 hours)

#### 4.1 Server Demo Mode (`--demo-mode` or `--zone-config`)

Extend `darkages_server` CLI:
- `--demo-mode` → shorthand that loads `tools/demo/content/demo_zone.json`.
- `--zone-config PATH` → load arbitrary JSON zone config.
- When demo mode active:
  - Disable DB persistence (no Redis/Scylla writes).
  - Set fixed RNG seed for reproducible NPC spawns.
  - Enable verbose snapshot logging (for validator metrics).
  - Reduce player respawn timer to 3s (faster showcase).

JSON schema for `demo_zone.json`:
```json
{
  "zone_id": 99,
  "name": "Showcase Grounds",
  "spawn_points": [
    {"x": 0, "y": 0, "z": 0, "type": "player_start"},
    {"x": 10, "y": 0, "z": 10, "type": "npc_guard"}
  ],
  "npc_presets": [
    {"archetype": "wolf", "count": 5, "radius": 25, "behavior": "aggressive"},
    {"archetype": "boss_ogre", "count": 1, "radius": 5, "event": "world_boss"}
  ],
  "demo_quest": {
    "id": "demo_welcome",
    "title": "Welcome to DarkAges",
    "objectives": ["kill_3_wolves", "speak_to_merchant"],
    "rewards": {"xp": 100, "gold": 50}
  },
  "zone_event": {
    "type": "wave_defense",
    "trigger_after_seconds": 60,
    "waves": 3
  }
}
```

#### 4.2 Demo Script Engine (`darkages-demo-narrator` skill update)

`content/demo_script.md` structure:
```markdown
# Scene 1: Connection
Duration: 10s
Narration: "Welcome to DarkAges. We are connecting to the ECS server..."
AgentAction: wait_for_handshake

# Scene 2: Exploration
Duration: 15s
Narration: "The world is populated by 10 NPCs. Notice the interpolation..."
AgentAction: move_forward 5s

# Scene 3: Combat
Duration: 20s
Narration: "Engaging a wolf. The server uses lag compensation..."
AgentAction: target_nearest_npc, attack
```

The narrator skill:
1. Parses `demo_script.md`.
2. Prints narration with `time.sleep()` pacing.
3. Executes `AgentAction` by calling `godot_controller.py` methods or sending UDP inputs via validator.
4. Can be skipped with `--skip-narration`.

#### 4.3 Input Macros (`content/input_macros/`)

JSON files describing input sequences for validator or Godot:
```json
{
  "name": "walk_forward",
  "duration_ms": 5000,
  "inputs": [
    {"key": "W", "action": "press", "delay_ms": 0},
    {"key": "W", "action": "release", "delay_ms": 5000}
  ]
}
```

Used by:
- `e2e_validator.py` for automated movement validation.
- `local_chaos.py` to verify client recovery after disturbance.

---

### PHASE 5: Validation, Reporting & Artifacts (Priority 2 — 4 hours)

#### 5.1 E2E Validator Expansion (`harness/e2e_validator.py`)

Current checks (all passing):
- UDP ping, handshake, snapshot rate, log cleanliness, binary verification.

Additional checks:
- [ ] NPC spawn validation: snapshots contain >= expected NPC count within 10s.
- [ ] Movement validation: send inputs, verify position changes in subsequent snapshots.
- [ ] Combat validation: send attack inputs, verify health component changes.
- [ ] Client connection validation (if Godot launched): parse Godot stdout for `Connected to server`.
- [ ] Tick budget validation: server tick < 20ms for 95th percentile.
- [ ] Memory leak guard: server RSS memory growth < 50% over demo duration.

#### 5.2 Screenshot Capture

Integration points:
- **Headed + xvfb:** Use `grim` (Wayland) or `scrot` (X11) or `import` (ImageMagick).
- **MCP path:** If MCP server patched, use its `take_screenshot` tool.
- **Godot GDScript:** `get_viewport().get_texture().get_image().save_png(path)` — most reliable.

Implementation:
```gdscript
# screenshot.gd — executed via godot --script
extends SceneTree
func _init():
    await create_timer(1.0).timeout
    var img = get_root().get_viewport().get_texture().get_image()
    img.save_png("/tmp/godot_screenshot.png")
    quit()
```

`godot_controller.py` method `take_screenshot(path)`:
1. Write temp GDScript.
2. Run `godot --headless --script /tmp/screenshot.gd`.
3. Move result from `/tmp/godot_screenshot.png` to desired path.

Trigger every 5 seconds during `--full` runs.

#### 5.3 Video Recording (`--record`)

If `--record` and headed display available:
```bash
ffmpeg -f x11grab -r 30 -s 1920x1080 -i :99 \
  -c:v libx264 -preset fast -crf 23 \
  artifacts/recordings/demo_YYYY-MM-DD_HH-MM-SS.mp4
```

- Spawn `ffmpeg` via supervisor.
- Stop recording gracefully on teardown (SIGTERM to ffmpeg).
- Requires `Xvfb` or real display.

#### 5.4 Report Generator (`harness/report_generator.py`)

New module. Generates `artifacts/reports/Demo_Report_YYYY-MM-DD_HH-MM-SS.md`:

```markdown
# DarkAges Demo Report
**Date:** 2026-04-23 14:32 UTC
**Duration:** 45.2s
**Git Commit:** 51aac79
**Mode:** --full --npcs 10 --duration 30

## Results
| Check | Status | Detail |
|-------|--------|--------|
| Dependencies | PASS | 8/8 |
| Build | PASS | 4.1s |
| Tests | PASS | 11/11 suites |
| Server Start | PASS | PID 12345 |
| Godot Client | PASS | PID 12346 |
| UDP Handshake | PASS | Entity ID 6 |
| Snapshot Rate | PASS | 148 snapshots |
| NPC Spawn | PASS | 10/10 NPCs visible |
| Tick Budget | PASS | p95 8.2ms |
| Log Clean | PASS | 0 errors |

## Screenshots
![Scene 1](screenshots/scene_1.png)
![Scene 2](screenshots/scene_2.png)

## Performance
- Server Tick p50: 6.1ms, p95: 8.2ms, max: 12.4ms
- Client FPS: avg 58, min 42
- Memory: Server 145MB, Client 312MB

## Logs
- Server: artifacts/logs/server_2026-04-23_14-32-01.log
- Client: artifacts/logs/client_2026-04-23_14-32-01.log
```

Also generate a machine-readable JSON copy for CI parsing.

---

### PHASE 6: Master Skill & One-Command Interface (Priority 3 — 2 hours)

#### 6.1 Skill: `darkages-demo-orchestrator` (Master)

Create a sixth skill that composes all others:
- **Trigger:** "run darkages demo", "showcase", "demo pipeline", "start demo"
- **Parameters:** `--mode {smoke,server-only,headless-client,full}`, `--duration`, `--npcs`, `--chaos`, `--record`, `--dashboard`
- **Flow:**
  1. Load `darkages-demo-harness` → dependency/build/test phase.
  2. Load `darkages-server-lifecycle` → start server.
  3. If client needed → load `godot-demo-control` → start client.
  4. Load `darkages-validation-suite` → run validation.
  5. If narration enabled → load `darkages-demo-narrator` → run script.
  6. Generate report.
  7. Teardown.
- **Returns:** Structured JSON with `success`, `report_path`, `metrics_summary`, `screenshot_paths`.

#### 6.2 `run_demo.py` Flag Completeness

Ensure all flags work:
| Flag | Implemented | Tested |
|------|-------------|--------|
| `--smoke` | Yes | Yes |
| `--server-only` | Yes | Yes |
| `--headless-client` | Yes | Yes |
| `--full` | Yes | Partial (needs display) |
| `--no-build` | Yes | Yes |
| `--no-tests` | Yes | Yes |
| `--clean-build` | Yes | Yes |
| `--duration` | Yes | Yes |
| `--npcs` | Yes | Yes |
| `--content` | Partial | No (server doesn't load JSON yet) |
| `--record` | No | No |
| `--chaos` | No | No |
| `--dashboard` | No | No |
| `--skip-narration` | No | No |

---

## V. File Structure (Target)

```
tools/demo/
├── install.py                      # One-time installer
├── run_demo.py                     # Master entry point
├── demo_launcher.py                # Legacy launcher (deprecated, keep for compat)
├── harness/
│   ├── __init__.py
│   ├── dependency_check.py         # Prerequisite validation (DONE)
│   ├── build_manager.py            # CMake wrapper with retry (DONE)
│   ├── test_runner.py              # CTest runner with flaky retry (DONE)
│   ├── supervisor.py               # Process manager + health + auto-restart (DONE, enhance)
│   ├── godot_controller.py         # Godot CLI + MCP client wrapper (DONE, enhance)
│   ├── e2e_validator.py            # End-to-end validation checklist (DONE, enhance)
│   ├── report_generator.py         # Markdown + JSON report generation (NEW)
│   ├── log_aggregator.py           # Real-time colorized log tail (NEW)
│   ├── metrics_collector.py        # Prometheus / proc / stdout polling (NEW)
│   ├── local_chaos.py              # Bare-metal fault injection (NEW)
│   ├── dashboard.py                # Terminal UI via rich (NEW)
│   └── install_godot_mcp.py        # MCP server clone/build/patch script (NEW)
├── content/
│   ├── demo_zone.json              # Zone config with NPCs, quest, event
│   ├── demo_script.md              # Presenter narration & scene actions
│   └── input_macros/               # Pre-recorded input sequences
│       ├── walk_forward.json
│       ├── attack_combo.json
│       └── party_invite.json
└── artifacts/                      # Created at runtime
    ├── screenshots/
    ├── recordings/
    ├── logs/
    ├── metrics/
    └── reports/

tools/external/
└── godot-mcp/                      # Cloned from bradypp/godot-mcp, patched for Linux

~/.hermes/skills/
├── darkages-demo-harness/
├── darkages-server-lifecycle/
├── godot-demo-control/
├── darkages-demo-narrator/
├── darkages-validation-suite/
└── darkages-demo-orchestrator/     # NEW — master composer skill
```

---

## VI. Implementation Roadmap

| Phase | Task | Effort | Priority | Owner |
|-------|------|--------|----------|-------|
| 1.1 | Clone & patch bradypp/godot-mcp | 2h | P1 | Agent |
| 1.2 | Register MCP in Hermes config | 1h | P1 | Agent |
| 1.3 | Update `godot-demo-control` skill for MCP | 1h | P1 | Agent |
| 1.4 | Add MCP availability check to `dependency_check.py` | 0.5h | P1 | Agent |
| 2.1 | Enhance supervisor: client probe, port escalation, memory guard | 2h | P1 | Agent |
| 2.2 | Implement `local_chaos.py` | 2h | P1 | Agent |
| 2.3 | Wire `--chaos` into `run_demo.py` | 1h | P1 | Agent |
| 2.4 | Test chaos + recovery end-to-end | 1h | P1 | Agent |
| 3.1 | Implement `log_aggregator.py` | 1.5h | P2 | Agent |
| 3.2 | Implement `metrics_collector.py` | 1.5h | P2 | Agent |
| 3.3 | Implement `dashboard.py` with `rich` | 1h | P2 | Agent |
| 3.4 | Wire `--dashboard` into `run_demo.py` | 0.5h | P2 | Agent |
| 4.1 | Add `--demo-mode` / `--zone-config` to server CLI | 2h | P2 | Agent |
| 4.2 | Finalize `demo_zone.json` schema and content | 1h | P2 | Agent |
| 4.3 | Implement input macro parser in validator | 1h | P2 | Agent |
| 5.1 | Expand validator: NPC, movement, combat, tick budget checks | 1.5h | P2 | Agent |
| 5.2 | Implement screenshot via GDScript in `godot_controller.py` | 1h | P2 | Agent |
| 5.3 | Implement `--record` with `ffmpeg` | 1h | P2 | Agent |
| 5.4 | Implement `report_generator.py` | 1.5h | P2 | Agent |
| 6.1 | Create `darkages-demo-orchestrator` skill | 1h | P3 | Agent |
| 6.2 | Full integration test: `--full --dashboard --record --chaos` | 1h | P3 | Agent |

**Total remaining effort:** ~26 hours of focused implementation.

---

## VII. Risk Register

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| bradypp/godot-mcp fails on Linux after patching | Medium | Medium | Fallback to CLI-only; pipeline degrades gracefully |
| Godot Mono startup > supervisor timeout | Low | Low | Increase timeout to 30s; show spinner in dashboard |
| Headless Godot cannot render for screenshots | Medium | Low | Use GDScript `get_viewport().get_texture()` which works headless; screenshots are server-side rendered |
| `tc` netem requires root and fails | Medium | Low | Check `os.geteuid() == 0`; skip chaos tests if non-root |
| WSL memory exhaustion during demo | Medium | Medium | Memory guard in supervisor; limit NPC count default to 10 |
| Build flake on incremental CMake | Low | High | Build manager auto-cleans and retries once |
| ffmpeg not installed for recording | Medium | Low | Make `--record` optional; check in dependency_check |
| Screenshot tools missing | Medium | Low | Use GDScript method first; only fallback to OS tools |

---

## VIII. Success Criteria

### Minimum Viable Demo (MVD) — Already Achieved
- [x] `python3 tools/demo/run_demo.py --smoke` exits 0 in < 60s
- [x] Server starts, validator connects, NPCs spawn, snapshots flow
- [x] No manual intervention required

### Full Showcase — Target
- [ ] `python3 tools/demo/run_demo.py --full --dashboard` shows live terminal UI
- [ ] Godot client launches (headed or xvfb), connects, renders world with NPCs
- [ ] Screenshots captured automatically every 5 seconds
- [ ] Demo script narration prints with pacing
- [ ] On Ctrl+C, everything stops cleanly
- [ ] Markdown report generated with all checks, metrics, screenshots

### Resilience Showcase — Target
- [ ] `--chaos` mode kills server mid-demo; auto-restart within 10 seconds
- [ ] Client reconnects automatically after server restart
- [ ] Final report marks "recovery test" as passed
- [ ] Network delay/loss simulation validated via validator metrics

### Agent-Ready — Target
- [ ] Hermes agent can say "run full demo" and orchestrator skill executes end-to-end
- [ ] Agent can say "show me the scene tree" and MCP or CLI returns Godot node hierarchy
- [ ] Agent can say "validate demo" and receive structured pass/fail JSON

---

## IX. Tooling & Reference Cheat Sheet

### Commands
```bash
# Quick smoke test
python3 tools/demo/run_demo.py --smoke

# Server-only with NPCs
python3 tools/demo/run_demo.py --server-only --npcs 10 --duration 30

# Headless client
python3 tools/demo/run_demo.py --headless-client --duration 30 --npcs 10

# Full headed demo (requires display)
python3 tools/demo/run_demo.py --full --duration 60 --npcs 10

# With chaos (requires root for tc)
sudo python3 tools/demo/run_demo.py --full --chaos --duration 60

# With recording
python3 tools/demo/run_demo.py --full --record --duration 60

# With dashboard
python3 tools/demo/run_demo.py --full --dashboard --duration 60
```

### Hermes Skills
```bash
# View any skill
skill_view("darkages-demo-harness")
skill_view("godot-demo-control")
skill_view("darkages-server-lifecycle")
skill_view("darkages-validation-suite")
skill_view("darkages-demo-narrator")

# MCP tools
skill_view("native-mcp")
skill_view("mcporter")
```

### Manual Server + Client
```bash
# Terminal 1
./build_validate/darkages_server --port 7777 --npcs --npc-count 10

# Terminal 2 (headed)
cd src/client && godot --path . -- --server 127.0.0.1 --port 7777

# Terminal 2 (headless xvfb)
xvfb-run godot --path . -- --server 127.0.0.1 --port 7777
```

### Godot MCP (when installed)
```bash
# List tools
mcporter list-servers
mcporter call godot-darkages get_scene_tree

# Or via Hermes native MCP (auto-discovered)
# Tools appear as native Hermes tools when server is running
```

---

*End of Master Plan v2*
