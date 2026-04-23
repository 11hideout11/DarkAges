# DarkAges MMO — Demo Presentation Script

## Scene 1: Connection & Handshake (10s)
> "Welcome to DarkAges, a 60Hz ECS-powered MMO server written in C++20. Watch as the client handshakes and receives its entity ID in under 100 milliseconds."

Command:
```bash
python3 tools/demo/run_demo.py --smoke
```

## Scene 2: NPC Spawning & AI (20s)
> "NPCs spawn with AI-driven behaviors. These wolves detect the player at 15 meters, enter chase state, and attack with server-authoritative combat logic."

Command:
```bash
python3 tools/demo/run_demo.py --server-only --npcs 5 --duration 20
```

## Scene 3: Multi-Client Replication (20s)
> "Multiple clients see each other via entity replication. The server broadcasts snapshots to all connected players every tick."

Command:
```bash
python3 tools/validation/live_client_validator.py --server-bin build_validate/darkages_server --clients 3 --duration 20 --npcs
```

## Scene 4: Network Resilience (20s)
> "Even under 50ms latency and 5% packet loss, the server maintains smooth gameplay through lag compensation and client-side prediction."

Command:
```bash
python3 tools/validation/live_client_validator.py --latency 50 --packet-loss 0.05 --clients 2 --duration 20 --npcs
```

## Scene 5: Full Client Showcase (60s)
> "Here is the Godot 4.2 Mono client with entity interpolation, prediction, and the full UI."

Command:
```bash
python3 tools/demo/run_demo.py --full --duration 60 --npcs 10
```
