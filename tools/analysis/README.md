# DarkAges Analysis Tools

Analysis pipeline for comprehensive game state understanding.

## Quick Start

```bash
# Run demo + validation in one command
python3 tools/analysis/run_and_analyze.py --quick

# Or validate existing logs
python3 tools/analysis/unified_analysis.py --validate
```

## Tool Chain

```
[Demo Run] → [Logs] → [Analysis] → [Report]
              ↓
         +-- realtime_analyzer.py   (client + server log parsing)
         +-- server_instrumentation.py (deep server state)
         +-- unified_analysis.py    (correlation + validation)
         ↓
         [Validated Model]
```

## Tools

### 1. run_and_analyze.py
One-command pipeline that runs demo then validates.

```bash
python3 tools/analysis/run_and_analyze.py --quick        # Quick demo
python3 tools/analysis/run_and_analyze.py --validate # Just validate
```

### 2. unified_analysis.py  
Combines all data sources, runs hypothesis tests.

```bash
# Validation with detailed output
python3 tools/analysis/unified_analysis.py --validate

# Generate JSON model  
python3 tools/analysis/unified_analysis.py --model --output model.json
```

### 3. realtime_analyzer.py  
Base log parsing - client/server events.

```bash
python3 tools/analysis/realtime_analyzer.py --analyze
python3 tools/analysis/realtime_analyzer.py --analyze --output report.json
```

### 4. server_instrumentation.py
Deep server-side analysis.

```bash
python3 tools/analysis/server_instrumentation.py analyze --log SERVER_LOG
python3 tools/analysis/server_instrumentation.py tick-data --log SERVER_LOG
python3 tools/analysis/server_instrumentation.py combat-log --log SERVER_LOG
```

## Validation Tests

| Test | Pass Criteria |
|------|---------------|
| combat_works | hits > 0 AND kills > 0 |
| network_sync | prediction error < 2m |
| multiplayer | clients >= 2 |

## Output Artifacts

- `tools/analysis/reports/` - JSON reports
- `tools/demo/artifacts/logs/` - Raw logs
- `/tmp/darkages_client_state.json` - Real-time client state (when instrumentation enabled)

## Common Issues

RTT shows negative? Network timing artifact, ignore if other metrics pass.
Prediction error high? Check client reconciliation settings.