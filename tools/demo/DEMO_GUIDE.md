# DarkAges MMO Demo Guide

## Quick Start

### Prerequisites
- Godot 4.2.2 Mono installed at `/usr/local/bin/godot`
- DarkAges server built and compiled
- 2+ terminals open

### Demo Sequence

#### Terminal 1: Start Server
```bash
cd /root/projects/DarkAges
./build_validate/darkages_server --port 7777 --npcs --npc-count 10
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

### Godot errors
- Ensure Godot 4.2.2 Mono is installed
- Check .NET SDK 6.0+ is installed
