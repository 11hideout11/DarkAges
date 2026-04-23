# DarkAges MMO Demo Guide

## Quick Start

### Prerequisites
- Godot 4.2.2 Mono installed at `/usr/local/bin/godot`
- DarkAges server built and compiled
- 2+ terminals open

### One-Command Demo Launcher

The demo launcher handles everything: build, test, deploy, validate, report.

```bash
cd /root/projects/DarkAges

# Quick smoke test (server + validator, ~35s)
python3 tools/demo/run_demo.py --smoke

# Server-only validation
python3 tools/demo/run_demo.py --server-only --duration 30 --npcs 10

# Demo mode with curated zone config (NPC archetypes, quest, zone event)
python3 tools/demo/run_demo.py --server-only --demo-mode --duration 30 --npcs 10

# Headless Godot client
python3 tools/demo/run_demo.py --headless-client --duration 30 --npcs 10

# Full headed demo with dashboard
python3 tools/demo/run_demo.py --full --dashboard --duration 60 --npcs 10

# Resilience demo with chaos injection
python3 tools/demo/run_demo.py --server-only --chaos --duration 60 --npcs 10

# Deep validation (NPC movement + tick budget checks via live_client_validator)
python3 tools/demo/run_demo.py --server-only --demo-mode --validator-deep --duration 30 --npcs 10

# Godot C# client integration test (actual client, headless)
python3 tools/validation/godot_integration_test.py

# Full pipeline with all features
python3 tools/demo/run_demo.py --full --dashboard --record --chaos --duration 120 --npcs 15
```

### Demo Sequence (Manual)

#### Terminal 1: Start Server
```bash
cd /root/projects/DarkAges
./build_validate/darkages_server --port 7777 --npcs --npc-count 10

# Or with demo mode (curated zone config)
./build_validate/darkages_server --port 7777 --demo-mode --zone-config tools/demo/content/demo_zone.json --npcs --npc-count 10
```

#### Terminal 2: Launch Godot Client
```bash
cd /root/projects/DarkAges/src/client
godot --path . --server 127.0.0.1 --port 7777
```

### Demo Walkthrough

1. **Connection**: Client connects to server and receives entity ID
2. **Movement**: Use WASD to move around the world
3. **Camera**: Mouse to look around
4. **Combat**: Walk near NPCs to trigger aggro, use left-click to attack
5. **NPC Behavior**: Watch NPCs chase, attack, and return to spawn
6. **Death/Respawn**: Die to an NPC and observe respawn UI

## Demo Scenes

### Scene 1: Basic Movement
- Open the game
- Connect to localhost:7777
- Walk around using WASD
- Jump with Space
- Look around with mouse

### Scene 2: Combat
- Start server with `--npcs --npc-count 5`
- Walk up to an NPC (within 15m aggro range)
- NPC will chase and attack
- Use abilities (1-4 keys)
- Observe health changes and combat text

### Scene 3: Party System
- Start 2 Godot clients
- Send party invite with /party invite <name>
- Share loot and XP

## Demo Mode (--demo-mode)

The `--demo-mode` CLI flag enables curated demo behavior:

- **Zone 99**: Showcase Grounds with custom NPC spawns
- **NPC Archetypes**: 5 wolves, 3 bandits, 1 boss (configurable via JSON)
- **Auto-Quest**: "Welcome to DarkAges" quest auto-accepted on connect
- **Zone Event**: `demo_wave_defense` pre-configured
- **Auth Bypass**: JWT validation skipped for demo builds

### Demo Zone Config

Edit `tools/demo/content/demo_zone.json` to customize:

```json
{
  "zone_name": "Showcase Grounds",
  "zone_id": 99,
  "npc_spawns": [
    {"type": "wolf", "count": 5, "level": 3, "radius": 40},
    {"type": "bandit", "count": 3, "level": 5, "radius": 30},
    {"type": "boss", "count": 1, "level": 8, "radius": 15}
  ],
  "zone_event": "demo_wave_defense",
  "auto_quest": 1
}
```

## Network Features Demo

## Validation Levels

| Level | Command | Duration | What It Tests |
|-------|---------|----------|---------------|
| 1 (Smoke) | `python3 tools/demo/run_demo.py --smoke --demo-mode` | ~35s | Server start, E2E checks, clean teardown |
| 2 (Deep) | `python3 tools/validation/live_client_validator.py --clients 2 --npcs ...` | ~60s | Multi-client handshake, NPC replication, AI movement, tick budgets, combat |
| 3 (Godot) | `python3 tools/validation/godot_integration_test.py` | ~15s | **Actual Godot C# client** connects headlessly, receives snapshots, spawns entities, sends inputs |

**Godot Integration Test Expected Results:**
```
  Connected:     PASS
  Snapshots:     100+ (20Hz)
  Entities seen: 11 (1 player + 10 NPCs)
  Errors:        0
  OVERALL:       PASS
```

## Network Features Demo

### Latency Simulation
```bash
# Start server with latency simulation
python tools/validation/live_client_validator.py \
  --server-bin build_validate/darkages_server \
  --port 7777 --clients 3 --duration 10 \
  --latency 50 --packet-loss 0.05
```

### Multi-Zone
```bash
# Start server on different zone
./darkages_server --zone-id 2 --port 7778
```

## Content Configuration

### NPC Types
The server supports various NPC archetypes:
- **Guard**: High HP, low damage, defensive
- **Mage**: Low HP, high damage, ranged
- **Warrior**: Balanced stats, melee
- **Healer**: Can heal other NPCs

### Custom Spawns
Edit `SpawnSystem` to register custom spawn groups:
```cpp
spawnSystem.registerSpawnGroup({
    .groupId = 1,
    .name = "Forest Enemies",
    .npcTypeId = 5,
    .level = 10,
    .count = 20
});
```

## Troubleshooting

### Client won't connect
- Check server is running on correct port
- Verify firewall allows UDP port 7777

### NPCs not spawning
- Ensure `--npcs` flag is passed to server
- Check server logs for spawn errors
- Use `--demo-mode` for guaranteed NPC spawns from JSON config

### Godot errors
- Ensure Godot 4.2.2 Mono is installed
- Check .NET SDK 6.0+ is installed

### Supervisor zombie-kill during validation
- If running `--validator-deep`, the standalone validator may briefly overload UDP connections
- This is a known pre-existing issue; validation results are still correct
- Run standalone validator separately: `python3 tools/validation/live_client_validator.py --server-bin ...`
