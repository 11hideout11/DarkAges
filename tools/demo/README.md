# DarkAges MMO - Complete Demo Documentation

## Overview

The DarkAges MMO demo showcases a fully functional multiplayer game server with:
- 60Hz tick rate UDP networking
- Entity interpolation and client-side prediction
- NPC AI with pathfinding and combat
- Party, guild, and chat systems
- Comprehensive test suite (1212 test cases)

## Prerequisites

### System Requirements
- Linux x86_64 (Ubuntu 22.04+)
- 4GB RAM minimum
- .NET SDK 6.0+ (for Godot Mono)
- Godot 4.2.2 with Mono

### Installation

```bash
# Install Godot 4.2.2 Mono
cd /tmp
curl -L "https://github.com/godotengine/godot/releases/download/4.2.2-stable/Godot_v4.2.2-stable_mono_linux.x86_64.zip" -o godot.zip
unzip -o godot.zip -d /usr/local/bin/
chmod +x /usr/local/bin/Godot_v4.2.2-stable_mono_linux.x86_64
ln -sf /usr/local/bin/Godot_v4.2.2-stable_mono_linux_x86_64/Godot_v4.2.2-stable_mono_linux.x86_64 /usr/local/bin/godot

# Verify installation
godot --version
```

## Quick Start

### Option 1: Automated Demo (Recommended)

```bash
cd /root/projects/DarkAges
python3 tools/demo/demo_launcher.py --server-only --npcs 10 --duration 15
```

This will:
1. Check prerequisites
2. Start server with 10 NPCs
3. Run automated validation
4. Display demo walkthrough

### Option 2: Manual Setup

**Terminal 1 - Start Server:**
```bash
cd /root/projects/DarkAges
./build_validate/darkages_server --port 7777 --npcs --npc-count 10
```

**Terminal 2 - Launch Godot Client:**
```bash
cd /root/projects/DarkAges/src/client
godot --path . -- --server 127.0.0.1 --port 7777
```

## Demo Scenes

### Scene 1: Basic Connection

Demonstrates:
- Server startup and initialization
- Client connection handshake
- Entity spawning and replication
- Snapshot broadcast (60Hz)

```bash
python3 tools/demo/demo_launcher.py --server-only --npcs 0 --duration 5
```

### Scene 2: NPC Combat

Demonstrates:
- NPC spawning and AI behaviors
- Aggro detection and chase
- Melee combat and damage
- NPC respawn after death

```bash
python3 tools/demo/demo_launcher.py --server-only --npcs 5 --duration 15
```

### Scene 3: Multi-Client

Demonstrates:
- Multiple simultaneous connections
- Entity visibility across clients
- Cross-client interaction
- Party system

```bash
python3 tools/demo/demo_launcher.py --server-only --npcs 5 --clients 3 --duration 20
```

### Scene 4: Network Resilience

Demonstrates:
- Latency simulation (50ms)
- Packet loss handling (5%)
- Server-side correction
- Entity interpolation

```bash
python3 tools/validation/live_client_validator.py \
  --server-bin build_validate/darkages_server \
  --port 7777 --clients 3 --duration 10 \
  --latency 50 --packet-loss 0.05
```

## Features Demo Walkthrough

### 1. Movement System
- WASD for movement
- Mouse for camera control
- Space for jump
- Server-authoritative position

### 2. Combat System
- Walk near NPCs (within 15m) to trigger aggro
- Left-click to attack
- Number keys 1-4 for abilities
- Health bar display
- Combat text (damage numbers)

### 3. NPC Behaviors
- **Idle**: Standing still, scanning for players
- **Chase**: Running toward detected player
- **Attack**: Melee attacks with cooldowns
- **Flee**: Running back to spawn when low HP
- **Return**: Walking back to spawn point

### 4. Social Features
- Local chat: `Type message + Enter`
- Party: `/party invite <name>`
- Guild: `/guild <message>`
- Whisper: `/w <name> <message>`

### 5. Death and Respawn
- Health reaches 0 = death
- Death screen with respawn timer
- Respawn at last checkpoint
- XP loss on death

## Architecture

```
DarkAges MMO Architecture
├── Server (C++20)
│   ├── ECS (EnTT)
│   │   ├── Components (Position, Velocity, CombatState, etc.)
│   │   └── Systems (Combat, AI, Spawn, etc.)
│   ├── Network (UDP)
│   │   ├── Connection handling
│   │   ├── Input processing
│   │   └── Snapshot replication
│   └── Zones
│       ├── ZoneServer
│       └── Entity migration
├── Client (Godot 4.2 / C#)
│   ├── NetworkManager
│   ├── Prediction (Client-side)
│   ├── Interpolation (Entity)
│   └── UI (HUD, Chat, etc.)
└── Tests (Catch2)
    ├── 1212 test cases
    └── 11 test suites
```

## Validation Output

Expected output from `--server-only` run:

```
============================================================
Checking Prerequisites
============================================================

  [OK  ] Godot 4.2 found
  [OK  ] .NET SDK 8.0 installed
  [OK  ] Godot project found

============================================================
Starting Server
============================================================

  Command: ./build_validate/darkages_server --port 7777 --npcs --npc-count 5
  [OK  ] Server started on port 7777 with 5 NPCs

============================================================
Running Validator
============================================================

  Command: python3 tools/validation/live_client_validator.py ...
  [OK  ] Validator passed
        Snapshots received: 268
        Entities per snapshot: 11.6
  [VALIDATOR] ALL CHECKS PASSED

============================================================
Demo Complete
============================================================
```

## Troubleshooting

### Server won't start
- Check if port 7777 is already in use: `netstat -tuln | grep 7777`
- Check logs: `/tmp/darkages_server.log`

### Client can't connect
- Verify server is running
- Check firewall rules: `ufw status`
- Try localhost: `127.0.0.1`

### NPCs not spawning
- Ensure `--npcs` flag is passed
- Check server logs for spawn errors

### Godot crashes on launch
- Ensure Godot 4.2.2 Mono is installed
- Check .NET SDK version: `dotnet --version`

## Demo Commands Reference

### Server Flags
```
--port <num>          Server port (default: 7777)
--zone-id <num>       Zone ID (default: 1)
--npcs                Auto-populate zone with NPCs
--npc-count <num>     Number of NPCs to spawn (default: 10)
--redis-host <host>   Redis host (default: localhost)
--redis-port <num>    Redis port (default: 6379)
```

### Demo Launcher Options
```
--build               Build server before demo
--server-only         Only start server (no client)
--client-only         Only start client (server must be running)
--npcs <num>          Number of NPCs (default: 10)
--port <num>          Server port (default: 7777)
--clients <num>       Validator clients (default: 2)
--duration <num>      Validator duration in seconds (default: 15)
```

### Validator Options
```
--server-bin <path>   Path to server binary
--port <num>          Server port
--clients <num>       Number of test clients
--duration <num>      Test duration in seconds
--npcs                Enable NPC spawning
--latency <ms>        Simulate network latency
--packet-loss <0-1>   Simulate packet loss
--combat             Enable combat testing
```

## Files Reference

### Demo Tools
- `tools/demo/demo_launcher.py` - Main demo orchestration script
- `tools/demo/DEMO_GUIDE.md` - Quick reference guide
- `tools/validation/live_client_validator.py` - Network validation tool
- `tools/validation/demo_validator.py` - Legacy demo harness

### Key Source Files
- `src/server/src/main.cpp` - Server entry point with CLI flags
- `src/client/src/networking/NetworkManager.cs` - Client network layer
- `src/server/src/combat/SpawnSystem.cpp` - NPC spawning
- `src/server/src/combat/NPCAISystem.cpp` - NPC behaviors

### Configuration
- `src/server/include/Constants.hpp` - Game constants (tick rate, ports, etc.)
- `src/client/project.godot` - Godot project configuration
