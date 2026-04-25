# Instrumentation Pipeline Integration — Comprehensive Implementation Plan

> **For Hermes:** Use `subagent-driven-development` skill to implement tasks in order. Each task is fully specified with exact files, code, and verification steps.

## Goal

Complete the real-time game state capture and correlation pipeline by integrating server-side instrumentation, extending the Python analysis tools, and wiring everything into the demo harness. This produces a **queryable, coherent 3D world model** from live client-server runs.

## Architecture Summary

```
┌─────────────────┐      ┌─────────────────┐
│ Godot Client    │      │ DarkAges Server │
│ 10 Hz snapshots │─────▶│ 10 Hz snapshots │       (per-tick JSON files)
│ /tmp/client/*   │      │ /tmp/server/*   │
└────────┬────────┘      └────────┬────────┘
         │                        │
         └────────┬───────────────┘
                  ▼
        ┌────────────────────┐
        │ Python Analyzer    │
        │ - correlate ticks  │
        │ - divergence calc  │
        │ - entity timeline  │
        │ - SQLite/Flask UI  │
        └────────────────────┘
```

**Current Status:**
- ✅ Client: `DemoInstrumentation` (rewritten), 10 Hz, per-tick JSON output; triggered by `DARKAGES_INSTRUMENT=1`; replaces `ClientDeepInstrumentation`
- ✅ Server: `ServerStateExporter` implemented and **integrated** (CLI flag, ZoneServer hook, CMake) via PR #17
- ✅ Python: Analysis tools exist including `tick_correlator.py` for JSON snapshot correlation
- ✅ Demo: `full_demo.py` and demo harness support `--instrument` flag

## Tasks Overview

| # | Task | Dependencies | Est. |
|---|------|-------------|------|
| 1 | ZoneConfig: add `enableInstrumentation` flag | None | 2 min |
| 2 | main.cpp: add `--instrument` CLI flag | Task 1 | 3 min |
| 3 | CMakeLists: add ServerStateExporter to build | None (build order independent) | 2 min |
| 4 | ZoneServer: instantiate exporter in ctor | Task 1 | 3 min |
| 5 | ZoneServer: hook `maybeExport()` in tick() | Task 4 | 3 min |
| 6 | full_demo.py: add `--instrument` mode | None | 5 min |
| 7 | Python: extend `unified_analysis.py` to read tick JSON | None (after 1-5 complete for testing) | 15 min |
| 8 | Integration test: run instrumented demo & generate report | Tasks 1-6,7 | 10 min |
| 9 | Commit and merge to main | All above | 5 min |

**Total:** ~45 min

---

## Task Details

### Task 1: ZoneConfig — Add instrumentation flag

**Objective:** Add `bool enableInstrumentation{false}` to `struct ZoneConfig` to control server-side instrumentation.

**Files:**
- `src/server/include/zones/ZoneServer.hpp:75` (ZoneConfig struct definition)

**Exact Changes:**

```cpp
// In ZoneConfig struct, after line 104 (zoneConfigPath), add:
    bool enableInstrumentation{false};  // Enable server tick-state export
```

**Verification:**

```bash
cd /root/projects/DarkAges
grep -n "enableInstrumentation" src/server/include/zones/ZoneServer.hpp
# Should show: 105:    bool enableInstrumentation{false};
```

---

### Task 2: main.cpp — Parse `--instrument` CLI flag

**Objective:** Add `--instrument` flag that sets `config.enableInstrumentation = true`.

**Files:**
- `src/server/src/main.cpp:13-27` (printUsage)
- `src/server/src/main.cpp:47-83` (argument parsing)
- `src/server/src/main.cpp:95-100` (config echo)

**Step 1 — Update usage text (around line 25):**

```diff
   << "  --zone-config <path>  Load zone configuration from JSON file\n" \
+  << "  --instrument          Enable server-side instrumentation (tick snapshots)\n" \
   << "  --help, -h            Show this help\n";
```

**Step 2 — Add CLI handler (after line 74, before `--zone-config` block):**

```cpp
} else if (arg == "--instrument") {
    config.enableInstrumentation = true;
}
```

**Step 3 — Echo in config summary (after line 100):**

```diff
   std::cout << "World Bounds: [" << config.minX << ", " << config.maxX << "] x ["
             << config.minZ << ", " << config.maxZ << "]\n";
+  std::cout << "Instrumentation: " << (config.enableInstrumentation ? "enabled" : "disabled") << "\n";
```

**Verification:**

```bash
cd /root/projects/DarkAges
# Build should succeed
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON \
  -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)

# Run server with --instrument, check output
./build_demo/darkages_server --instrument --demo-mode 2>&1 | grep -i instrument
# Should show: "Instrumentation: enabled"
```

---

### Task 3: CMakeLists — Build ServerStateExporter

**Objective:** Add `src/server/src/instrumentation/ServerStateExporter.cpp` to the server target sources.

**Files:**
- Root `CMakeLists.txt` (NOT `src/server/CMakeLists.txt` — that file is vestigial)

**Steps:**

```bash
cd /root/projects/DarkAges

# Locate where SERVER_SOURCES is defined (around line 474 in root CMakeLists.txt)
# Insert the instrumentation source file in alphabetical order or near other server/src files
```

**Exact edit (use patch):**

Find the block that looks like:

```cmake
list(APPEND SERVER_SOURCES
    src/server/src/zones/ZoneServer.cpp
    src/server/src/zones/ZoneOrchestrator.cpp
    # ... many more ...
)
```

Insert **alphabetically**:

```
    src/server/src/instrumentation/ServerStateExporter.cpp
```

**Verification:**

```bash
# Re-configure and ensure CMake sees the file
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON \
  -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF 2>&1 | grep -i "ServerStateExporter"
# Should show in the sources list or no errors

# Build should succeed
cmake --build build_validate -j$(nproc) 2>&1 | tail -5
```

---

### Task 4: ZoneServer — Constructor member initialization

**Objective:** Instantiate `ServerStateExporter` as a member of `ZoneServer`.

**Files:**
- `src/server/include/zones/ZoneServer.hpp` — add member declaration
- `src/server/src/zones/ZoneServer.cpp` — initialize in constructor initializer list

**Step 1 — Header (ZoneServer.hpp):**

Find private section (around line 220 in header). Add:

```cpp
private:
    // ... existing private members ...
    std::unique_ptr<DarkAges::Instrumentation::ServerStateExporter> instrumentationExporter_;
```

**Step 2 — Constructor (ZoneServer.cpp line ~45 initializer list):**

```diff
 ZoneServer::ZoneServer()
-    : smallPool_(std::make_unique<Memory::SmallPool>()),
+    : instrumentationExporter_(std::make_unique<DarkAges::Instrumentation::ServerStateExporter>("/tmp/darkages_snapshots/server")),
+      smallPool_(std::make_unique<Memory::SmallPool>()),
```

(Note: `DarkAges::Instrumentation::ServerStateExporter` already defined in ServerStateExporter.hpp)

**Step 3 (optional but recommended) — initialize from config in initialize():**

If you want output directory configurable, you can defer construction to `initialize()`. But simpler: always construct in ctor and enable based on config:

In `ZoneServer::initialize()` after `config_ = config;` (line 60), add:

```cpp
instrumentationExporter_->setEnabled(config.enableInstrumentation);
```

**Verification:**

```bash
# Header must compile
g++ -std=c++20 -I. -c src/server/src/zones/ZoneServer.cpp -o /dev/null 2>&1
echo $?
# Should be 0

# Build should pass
cmake --build build_validate -j$(nproc) 2>&1 | grep -i "error" || echo "Build OK"
```

---

### Task 5: ZoneServer — Hook exporter into tick()

**Objective:** Call `maybeExport()` once per tick after world state updates are complete.

**Files:**
- `src/server/src/zones/ZoneServer.cpp` — tick() method

**Location:** After `updateDatabase()` (around line 651) but before snapshots are sent to clients. Best: after line 651 or just before metrics calculation.

**Exact insertion:**

```cpp
bool ZoneServer::tick() {
    // ... existing code ...
    {
        ZONE_TRACE_EVENT("updateDatabase", Profiling::TraceCategory::DATABASE);
        updateDatabase();
    }

+   // [INSTRUMENTATION] Export world snapshot at configured interval
+   if (instrumentationExporter_ && instrumentationExporter_->isEnabled()) {
+       instrumentationExporter_->maybeExport(
+           registry_, currentTick_, getCurrentTimeMs()
+       );
+   }

    // Calculate tick time
    auto tickEnd = std::chrono::steady_clock::now();
    // ...
```

**Verification:**

```bash
# Build must pass
cmake --build build_validate -j$(nproc) && echo "BUILD PASS" || echo "BUILD FAIL"

# Run a zero-duration demo with instrumentation and check for output files
rm -rf /tmp/darkages_snapshots
./build_demo/darkages_server --instrument --demo-mode --npcs &
SERVER_PID=$!
sleep 3
kill $SERVER_PID 2>/dev/null
sleep 1
ls /tmp/darkages_snapshots/server/ 2>/dev/null | head -3
# Should list JSON files like snapshot_tick_000000.json
```

---

### Task 6: full_demo.py — Add `--instrument` flag

**Objective:** Enable instrumentation on both client and server when `--instrument` is passed to the demo harness.

**Files:**
- `tools/demo/full_demo.py:1` (argparse section)
- `tools/demo/full_demo.py:XXX` (where server and client are launched)

**Step 1 — Add argument:**

Find `parser.add_argument` section (around line 50–80). Add:

```python
parser.add_argument(
    "--instrument", "-i",
    action="store_true",
    help="Enable server + client instrumentation (writes tick snapshots to /tmp/darkages_snapshots)"
)
```

**Step 2 — Pass to server launch:**

Find where `server_cmd` is built (likely around line 150–200). Add:

```python
if args.instrument:
    server_cmd.append("--instrument")
```

**Step 3 — Set env var for client:**

Find where `client_env` is set (or create it). Add:

```python
client_env = os.environ.copy()
if args.instrument:
    client_env["DARKAGES_INSTRUMENT"] = "1"
# ... then pass client_env to subprocess.Popen for client
```

**Step 4 — Post-run analysis (optional):**

After demo completes (around line 450–500), if `args.instrument`, run analysis:

```python
if args.instrument:
    subprocess.run([
        sys.executable, "tools/analysis/unified_analysis.py",
        "--model", "--output", "tools/analysis/reports/instrumented_run.json"
    ])
    print("\n📊 Instrumentation report: tools/analysis/reports/instrumented_run.json")
```

**Verification:**

```bash
cd /root/projects/DarkAges
python3 tools/demo/full_demo.py --quick --instrument --demo-duration 5
# Should run 5-second demo, then produce JSON snapshots in /tmp/darkages_snapshots/
ls /tmp/darkages_snapshots/client/ 2>/dev/null | wc -l   # > 0?
ls /tmp/darkages_snapshots/server/ 2>/dev/null | wc -l   # > 0?
```

---

### Task 7: Python analyzer — Correlate tick JSON snapshots

**Objective:** Extend analysis pipeline to ingest per-tick JSON files and produce unified world model, divergence metrics, and reports.

**Files to create/modify:**
- `tools/analysis/tick_correlator.py` — NEW module (imported by unified_analysis)
- `tools/analysis/unified_analysis.py` — add support for `--model` and `--instrument-dir` flags

**Step 1 — Create `tick_correlator.py`:**

```python
#!/usr/bin/env python3
"""Correlates client and server tick snapshots into a unified world model."""

import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class EntityState:
    entity_id: int
    position: Tuple[float, float, float]
    velocity: Tuple[float, float, float]
    health: int
    state: int
    source: str  # "server" or "client"
    tick: int
    timestamp_ms: int

class TickCorrelator:
    def __init__(self, client_dir: Path, server_dir: Path):
        self.client_dir = client_dir
        self.server_dir = server_dir
        self.client_ticks: Dict[int, dict] = {}
        self.server_ticks: Dict[int, dict] = {}

    def load_snapshots(self):
        """Load all client and server JSON snapshots."""
        for f in sorted(self.client_dir.glob("*.json")):
            with open(f) as fp:
                data = json.load(fp)
            tick = data.get("tick", 0)
            self.client_ticks[tick] = data

        for f in sorted(self.server_dir.glob("*.json")):
            with open(f) as fp:
                data = json.load(fp)
            tick = data.get("tick", 0)
            self.server_ticks[tick] = data

    def get_entity_timeline(self, entity_id: int) -> List[EntityState]:
        """Build continuous timeline for a given entity across ticks."""
        timeline = []
        for tick in sorted(set(self.client_ticks.keys()) | set(self.server_ticks.keys())):
            # Prefer server as authoritative
            if tick in self.server_ticks:
                for ent in self.server_ticks[tick].get("entities", []):
                    if ent["entity_id"] == entity_id:
                        timeline.append(EntityState(
                            entity_id=entity_id,
                            position=tuple(ent["position"]),
                            velocity=tuple(ent["velocity"]),
                            health=ent.get("health", 0),
                            state=ent.get("state", 0),
                            source="server",
                            tick=tick,
                            timestamp_ms=self.server_ticks[tick].get("timestamp_ms", 0)
                        ))
                        break
            # Fallback to client if not in server
            elif tick in self.client_ticks:
                for ent in self.client_ticks[tick].get("entities", []):
                    if ent["entity_id"] == entity_id:
                        timeline.append(EntityState(
                            entity_id=entity_id,
                            position=tuple(ent["position"]),
                            velocity=tuple(ent.get("velocity", (0,0,0))),
                            health=ent.get("health", 0),
                            state=ent.get("state", 0),
                            source="client",
                            tick=tick,
                            timestamp_ms=self.client_ticks[tick].get("timestamp_ms", 0)
                        ))
                        break
        return timeline

    def compute_divergence_metrics(self) -> Dict:
        """Compute average client-server divergence per tick."""
        divergences = []
        matched_ticks = 0
        for tick in self.server_ticks:
            if tick not in self.client_ticks:
                continue
            server_snap = self.server_ticks[tick]
            client_snap = self.client_ticks[tick]

            # Cross-match entities by ID
            server_entities = {e["entity_id"]: e for e in server_snap.get("entities", [])}
            client_entities = {e["entity_id"]: e for e in client_snap.get("entities", [])}

            per_tick_deltas = []
            for eid, s_ent in server_entities.items():
                if eid in client_entities:
                    c_ent = client_entities[eid]
                    s_pos = s_ent["position"]
                    c_pos = c_ent.get("position", [0,0,0])
                    dx = s_pos[0] - c_pos[0]
                    dy = s_pos[1] - c_pos[1]
                    dz = s_pos[2] - c_pos[2]
                    dist = (dx*dx + dy*dy + dz*dz) ** 0.5
                    per_tick_deltas.append(dist)

            if per_tick_deltas:
                divergences.append(sum(per_tick_deltas) / len(per_tick_deltas))
                matched_ticks += 1

        return {
            "tick_count": matched_ticks,
            "avg_position_delta_m": sum(divergences) / len(divergences) if divergences else 0.0,
            "max_delta_m": max(divergences) if divergences else 0.0,
        }

    def to_unified_model(self) -> Dict:
        """Export full unified timeline as a queryable model."""
        all_entities = set()
        for snap in self.server_ticks.values():
            for e in snap.get("entities", []):
                all_entities.add(e["entity_id"])
        for snap in self.client_ticks.values():
            for e in snap.get("entities", []):
                all_entities.add(e["entity_id"])

        model = {
            "generated_at": datetime.now().isoformat(),
            "total_ticks": len(set(self.server_ticks) | set(self.client_ticks)),
            "total_entities": len(all_entities),
            "entities": {},
        }

        for eid in all_entities:
            model["entities"][str(eid)] = [
                asdict(s) for s in self.get_entity_timeline(eid)
            ]

        return model
```

**Step 2 — Patch `unified_analysis.py` to detect tick snapshots:**

At the top, after imports, detect if tick directories exist and invoke correlator:

```python
# In UnifiedAnalysis class __init__, after existing code:
client_snap_dir = Path("/tmp/darkages_snapshots/client")
server_snap_dir = Path("/tmp/darkages_snapshots/server")

if client_snap_dir.exists() and server_snap_dir.exists():
    from tick_correlator import TickCorrelator
    self.correlator = TickCorrelator(client_snap_dir, server_snap_dir)
    self.correlator.load_snapshots()
    self.unified_model = self.correlator.to_unified_model()
    self.divergence = self.correlator.compute_divergence_metrics()
else:
    self.correlator = None
    self.unified_model = {}
    self.divergence = {}
```

**Step 3 — Add `--model` and `--instrument-dir` options to unified_analysis.py:**

In `main()`:

```python
parser.add_argument("--model", action="store_true", help="Generate unified tick-level model")
parser.add_argument("--output", "-o", type=Path, default=Path("tools/analysis/reports/unified_model.json"),
                    help="Output path for model JSON")
parser.add_argument("--instrument-dir", type=Path, default=Path("/tmp/darkages_snapshots"),
                    help="Base directory containing client/ and server/ subdirs")
```

Modify argument passing to `UnifiedAnalysis`:

```python
ua = UnifiedAnalysis(
    client_log=args.log_dir / "client.log",
    server_log=args.log_dir / "server.log",
    instrument_dir=args.instrument_dir
)
```

**Verification:**

```bash
# After running instrumented demo:
python3 tools/analysis/unified_analysis.py --model --instrument-dir /tmp/darkages_snapshots
# Should output:
#   Generated unified model with N entities, M ticks
#   Avg divergence: X.XX m

# Check output file exists and is valid JSON
python3 -c "import json; json.load(open('tools/analysis/reports/unified_model.json'))" && echo "JSON OK"
```

---

### Task 8: Integration test — Full instrumented demo run

**Objective:** Run a live demo with full instrumentation end-to-end and validate the pipeline.

**Steps:**

```bash
cd /root/projects/DarkAges

# Clean any previous snapshots
rm -rf /tmp/darkages_snapshots

# Build (if not already built)
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON \
  -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)

# Run instrumented demo
python3 tools/demo/full_demo.py --quick --instrument --demo-duration 10

# Validate output
echo "=== Client snapshots ==="
ls /tmp/darkages_snapshots/client/ | wc -l
echo "=== Server snapshots ==="
ls /tmp/darkages_snapshots/server/ | wc -l

# Run analysis
python3 tools/analysis/unified_analysis.py --model --instrument-dir /tmp/darkages_snapshots
python3 -c "import json; d=json.load(open('tools/analysis/reports/unified_model.json')); print('Entities:', d['total_entities'], 'Ticks:', d['total_ticks'])"
```

**Expected criteria:**
- Client snapshots: > 50 files (10 Hz × 5+ sec)
- Server snapshots: > 50 files
- Unified model: non-empty `entities` dict
- Divergence: avg position delta < 1.0m (network prediction OK)

---

### Task 9: Commit and merge to main

**Objective:** Commit all changes on a feature branch, validate with two-evaluator pipeline, then merge to main.

**Workflow per autonomous pattern:**

```bash
cd /root/projects/DarkAges
git checkout main
git pull --ff-only 2>/dev/null || git merge -X theirs origin/main --no-edit

git checkout -b "autonomous/$(date +%Y%m%d)-instrumentation-complete"
git add -A
git commit -m "feat(instrumentation): complete server tick exporter integration + unified analysis

- ZoneConfig enableInstrumentation flag
- Server --instrument CLI flag
- CMake build rules for ServerStateExporter
- ZoneServer tick hook with 10 Hz export
- full_demo.py --instrument mode
- TickCorrelator for unified world model
- Extended unified_analysis.py with tick-level model output"
```

**Pre-completion gate:**

1. **Objective evaluator:** Build + test must pass
   ```bash
   cmake --build build_validate -j$(nproc) && ctest --output-on-failure -j8
   ```

2. **Subjective reviewer:** Run via OpenCode (or simulate review):
   - Check namespace usage (`DarkAges::`)
   - Check for correct CMakeLists (root, not src/server/)
   - Check EnTT pattern compliance (all_of, re-fetch pointers, etc.)
   - Zero build warnings treated as warnings (non-blocking) unless new

**Merge:**

```bash
# Only if both evaluators pass
git checkout main
git merge --no-ff autonomous/$(date +%Y%m%d)-instrumentation-complete
git branch --delete autonomous/$(date +%Y%m%d)-instrumentation-complete
echo "✅ Instrumentation pipeline complete and merged to main"
```

---

## Notes for Implementer

- All server-side changes are in the `DarkAges` namespace. Double-check every new file has `namespace DarkAges` or proper nested namespaces.
- The server tick export uses a temp allocator pattern identical to the server's existing zero-allocation policy — `ServerStateExporter` already implements this.
- Client and server output directories must match `/tmp/darkages_snapshots/{client,server}` for the correlator to find them.
- Per-tick JSON files are named `client_{tick:012d}.json` and `server_*.json` (the server exporter will name them appropriately — ensure consistency).
- No DB dependencies are touched; Redis/Scylla are stubs.

---

## Acceptance Criteria

1. **Build:** 1978 tests compile and run on `main` after merge
2. **Runtime:** `full_demo.py --instrument --quick` completes without crash and produces ≥100 snapshots per side
3. **Analysis:** `unified_analysis.py --model` produces valid JSON with:
   - `total_entities > 0`
   - `total_ticks > 0`
   - `entities` dict non-empty
   - `avg_position_delta_m < 2.0` (tolerance for network prediction error)
4. **Code quality:** Zero new compiler warnings, no new clang-tidy violations, follows DarkAges conventions (see AGENTS.md)
5. **Documentation:** Updated `tools/analysis/README.md` with tick-correlation section (if missing)
