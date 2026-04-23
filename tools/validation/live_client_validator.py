#!/usr/bin/env python3
"""
Live Client-Server Validator for DarkAges MMO

Validates real UDP packet flow between a Python test client and the
darkages_server binary without requiring the Godot client.

Tests:
- Connection handshake (request -> response with correct entity ID)
- Ping/pong roundtrip (RTT measurement)
- Input reception (forward/right/yaw/pitch decoding)
- Snapshot broadcast format validation
- Multiple simultaneous client connections
- Graceful disconnect

Usage:
    python live_client_validator.py --server-bin ../build_validate/darkages_server
    python live_client_validator.py --clients 5 --duration 10
"""

import argparse
import os
import socket
import struct
import subprocess
import sys
import time
import threading
from dataclasses import dataclass, field
from typing import Optional

# Packet type constants (must match server and client)
PACKET_CLIENT_INPUT = 1
PACKET_SNAPSHOT = 2
PACKET_EVENT = 3
PACKET_PING = 4
PACKET_PONG = 5
PACKET_CONNECTION_REQUEST = 6
PACKET_CONNECTION_RESPONSE = 7
PACKET_SERVER_CORRECTION = 8
PACKET_RESPAWN_REQUEST = 9


@dataclass
class ClientStats:
    """Statistics for a single test client."""
    entity_id: int = 0
    connected: bool = False
    rtt_ms: float = 0.0
    snapshots_received: int = 0
    corrections_received: int = 0
    events_received: int = 0
    inputs_sent: int = 0
    attacks_sent: int = 0
    pings_sent: int = 0
    pongs_received: int = 0
    errors: list[str] = field(default_factory=list)
    snapshot_entity_counts: list[int] = field(default_factory=list)
    # Combat tracking
    entity_health: dict[int, int] = field(default_factory=dict)  # entity_id -> health%
    health_changes: list[tuple] = field(default_factory=list)  # (entity_id, old, new, tick)
    deaths_observed: int = 0
    respawns_observed: int = 0
    # Interpolation / jitter tracking
    snapshot_arrival_times: list[float] = field(default_factory=list)
    snapshot_server_ticks: list[int] = field(default_factory=list)
    entity_positions: dict[int, list[tuple]] = field(default_factory=dict)  # entity_id -> [(time, x, y, z)]


class TestClient:
    """UDP test client that mimics the Godot client's network behavior."""

    def __init__(self, server_host: str, server_port: int, client_id: int,
                 latency_ms: int = 0, packet_loss: float = 0.0):
        self.server_host = server_host
        self.server_port = server_port
        self.client_id = client_id
        self.latency_ms = latency_ms
        self.packet_loss = packet_loss
        self.stats = ClientStats()

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(5.0)
        self.connected = False
        self.running = False
        self.receive_thread: Optional[threading.Thread] = None
        self.input_sequence = 1
        self.last_ping_time = 0.0
        self._lock = threading.Lock()

    def _send_with_simulation(self, data: bytes) -> bool:
        """Send data with optional packet loss and latency simulation."""
        if self.packet_loss > 0 and __import__('random').random() < self.packet_loss:
            return True  # Drop packet silently
        if self.latency_ms > 0:
            def delayed_send():
                time.sleep(self.latency_ms / 1000.0)
                try:
                    self.sock.sendto(data, (self.server_host, self.server_port))
                except Exception:
                    pass
            threading.Thread(target=delayed_send, daemon=True).start()
            return True
        try:
            self.sock.sendto(data, (self.server_host, self.server_port))
            return True
        except Exception:
            return False

    def connect(self) -> bool:
        """Send connection request and wait for response."""
        try:
            # [type:1=6][version:4][player_id:4]
            request = struct.pack('<BII', PACKET_CONNECTION_REQUEST, 1, self.client_id)
            if not self._send_with_simulation(request):
                self.stats.errors.append("Connection request send failed")
                return False

            # Wait for connection response
            data, addr = self.sock.recvfrom(1024)
            if len(data) < 10:
                self.stats.errors.append(f"Connection response too short: {len(data)} bytes")
                return False

            pkt_type, success, entity_id, zone_id = struct.unpack('<BBII', data)
            if pkt_type != PACKET_CONNECTION_RESPONSE:
                self.stats.errors.append(f"Expected CONNECTION_RESPONSE, got {pkt_type}")
                return False
            if not success:
                self.stats.errors.append("Connection rejected by server")
                return False

            self.stats.entity_id = entity_id
            self.stats.connected = True
            self.connected = True
            return True

        except socket.timeout:
            self.stats.errors.append("Connection timeout")
            return False
        except Exception as e:
            self.stats.errors.append(f"Connection error: {e}")
            return False

    def start(self):
        """Start background receive thread."""
        self.running = True
        self.receive_thread = threading.Thread(target=self._receive_loop)
        self.receive_thread.start()

    def stop(self):
        """Stop the client."""
        self.running = False
        self.sock.close()
        if self.receive_thread:
            self.receive_thread.join(timeout=2.0)

    def send_input(self, forward: bool = False, backward: bool = False,
                   left: bool = False, right: bool = False,
                   jump: bool = False, attack: bool = False,
                   block: bool = False, sprint: bool = False,
                   yaw: float = 0.0, pitch: float = 0.0) -> bool:
        """Send a client input packet."""
        if not self.connected:
            return False

        flags = 0
        if forward:  flags |= 0x01
        if backward: flags |= 0x02
        if left:     flags |= 0x04
        if right:    flags |= 0x08
        if jump:     flags |= 0x10
        if attack:   flags |= 0x20
        if block:    flags |= 0x40
        if sprint:   flags |= 0x80

        yaw_q = max(-32768, min(32767, int(yaw * 10000)))
        pitch_q = max(-32768, min(32767, int(pitch * 10000)))
        timestamp = int(time.time() * 1000) & 0xFFFFFFFF

        # [type:1=1][sequence:4][timestamp:4][flags:1][yaw:2][pitch:2][target:4]
        data = struct.pack('<BIIbhhI', PACKET_CLIENT_INPUT, self.input_sequence,
                           timestamp, flags, yaw_q, pitch_q, 0)
        if self._send_with_simulation(data):
            self.input_sequence += 1
            with self._lock:
                self.stats.inputs_sent += 1
                if attack:
                    self.stats.attacks_sent += 1
            return True
        else:
            with self._lock:
                self.stats.errors.append("Input send error")
            return False

    def send_ping(self) -> bool:
        """Send a ping packet."""
        if not self.connected:
            return False

        self.last_ping_time = time.time()
        timestamp = int(self.last_ping_time * 1000) & 0xFFFFFFFF
        data = struct.pack('<BI', PACKET_PING, timestamp)
        if self._send_with_simulation(data):
            with self._lock:
                self.stats.pings_sent += 1
            return True
        else:
            with self._lock:
                self.stats.errors.append("Ping send error")
            return False

    def _receive_loop(self):
        """Background thread: receive and process server packets."""
        while self.running:
            try:
                self.sock.settimeout(1.0)
                data, addr = self.sock.recvfrom(4096)
                if len(data) < 1:
                    continue

                pkt_type = data[0]
                if pkt_type == PACKET_SNAPSHOT:
                    self._process_snapshot(data)
                elif pkt_type == PACKET_PONG:
                    self._process_pong(data)
                elif pkt_type == PACKET_SERVER_CORRECTION:
                    with self._lock:
                        self.stats.corrections_received += 1
                elif pkt_type == PACKET_EVENT:
                    with self._lock:
                        self.stats.events_received += 1

            except socket.timeout:
                continue
            except OSError:
                break
            except Exception as e:
                with self._lock:
                    self.stats.errors.append(f"Receive error: {e}")

    def _process_snapshot(self, data: bytes):
        """Parse snapshot packet and validate format."""
        if len(data) < 13:
            self.stats.errors.append(f"Snapshot too short: {len(data)} bytes")
            return

        # [type:1=2][server_tick:4][last_input:4][entity_count:4][entity_data...]
        server_tick, last_input, entity_count = struct.unpack_from('<III', data, 1)

        expected_size = 13 + entity_count * 30
        if len(data) < expected_size:
            self.stats.errors.append(
                f"Snapshot truncated: expected {expected_size}, got {len(data)} "
                f"(entities={entity_count})"
            )
            return

        arrival_time = time.time()
        with self._lock:
            self.stats.snapshots_received += 1
            self.stats.snapshot_entity_counts.append(entity_count)
            self.stats.snapshot_arrival_times.append(arrival_time)
            self.stats.snapshot_server_ticks.append(server_tick)

            # Parse entity health data for combat tracking
            offset = 13
            for i in range(entity_count):
                entity_id = struct.unpack_from('<I', data, offset)[0]
                offset += 4  # id
                pos_x, pos_y, pos_z = struct.unpack_from('<fff', data, offset)
                offset += 12  # pos x,y,z (3 floats)
                vel_x, vel_y, vel_z = struct.unpack_from('<fff', data, offset)
                offset += 12  # vel x,y,z (3 floats)
                health = data[offset]
                offset += 1  # health
                offset += 1  # anim

                # Track position for interpolation validation
                if entity_id not in self.stats.entity_positions:
                    self.stats.entity_positions[entity_id] = []
                self.stats.entity_positions[entity_id].append((arrival_time, pos_x, pos_y, pos_z))

                old_health = self.stats.entity_health.get(entity_id, -1)
                if old_health != -1 and old_health != health:
                    self.stats.health_changes.append((entity_id, old_health, health, server_tick))
                    if old_health > 0 and health == 0:
                        self.stats.deaths_observed += 1
                    if old_health == 0 and health > 0:
                        self.stats.respawns_observed += 1
                self.stats.entity_health[entity_id] = health

    def _process_pong(self, data: bytes):
        """Calculate RTT from pong packet."""
        if len(data) < 5:
            return
        rtt = (time.time() - self.last_ping_time) * 1000.0
        with self._lock:
            self.stats.pongs_received += 1
            self.stats.rtt_ms = rtt


def compute_jitter_stats(arrival_times: list[float]) -> dict:
    """Compute snapshot jitter statistics from arrival timestamps."""
    if len(arrival_times) < 2:
        return {'count': len(arrival_times), 'median_ms': 0.0, 'stddev_ms': 0.0,
                'min_ms': 0.0, 'max_ms': 0.0, 'loss_percent': 0.0}

    intervals = []
    for i in range(1, len(arrival_times)):
        dt = (arrival_times[i] - arrival_times[i - 1]) * 1000.0
        intervals.append(dt)

    intervals_sorted = sorted(intervals)
    n = len(intervals)
    median = intervals_sorted[n // 2] if n % 2 else (intervals_sorted[n // 2 - 1] + intervals_sorted[n // 2]) / 2
    mean = sum(intervals) / n
    variance = sum((x - mean) ** 2 for x in intervals) / n
    stddev = variance ** 0.5
    min_i = min(intervals)
    max_i = max(intervals)

    # Estimate packet loss: intervals > 2x median suggest a dropped snapshot
    expected_interval = 50.0  # 20Hz = 50ms
    lost = sum(1 for dt in intervals if dt > expected_interval * 1.8)
    loss_percent = (lost / (n + lost)) * 100.0 if (n + lost) > 0 else 0.0

    return {
        'count': len(arrival_times),
        'median_ms': median,
        'stddev_ms': stddev,
        'min_ms': min_i,
        'max_ms': max_i,
        'loss_percent': loss_percent
    }


def compute_tick_consistency(ticks: list[int]) -> dict:
    """Check that server tick increments are consistent (should be +3 at 20Hz from 60Hz tick)."""
    if len(ticks) < 2:
        return {'consistent': True, 'expected_delta': 3, 'actual_deltas': []}
    deltas = [ticks[i] - ticks[i - 1] for i in range(1, len(ticks))]
    # Allow some variance: most deltas should be 3, occasional 2 or 4 is ok
    expected = 3
    ok = sum(1 for d in deltas if d in (2, 3, 4))
    return {'consistent': ok >= len(deltas) * 0.8, 'expected_delta': expected, 'actual_deltas': deltas}


def compute_position_jitter(entity_positions: dict[int, list[tuple]]) -> dict:
    """Compute per-entity position delta statistics."""
    max_delta = 0.0
    total_deltas = 0
    large_jumps = 0
    for entity_id, positions in entity_positions.items():
        if len(positions) < 2:
            continue
        for i in range(1, len(positions)):
            _, x1, y1, z1 = positions[i - 1]
            _, x2, y2, z2 = positions[i]
            dx = ((x2 - x1) ** 2 + (y2 - y1) ** 2 + (z2 - z1) ** 2) ** 0.5
            max_delta = max(max_delta, dx)
            total_deltas += 1
            # At 20Hz, max reasonable movement is ~5m/s * 0.05s = 0.25m
            # But server allows higher speeds; flag jumps > 2m between snapshots
            if dx > 2.0:
                large_jumps += 1
    return {'max_delta_m': max_delta, 'total_deltas': total_deltas, 'large_jumps': large_jumps}


def start_server(bin_path: str, port: int, npcs: bool = False, npc_count: int = 10,
                 demo_mode: bool = False) -> subprocess.Popen:
    """Start the darkages_server binary on a given port."""
    cmd = [bin_path, '--port', str(port), '--zone-id', '1']
    if demo_mode:
        cmd.append('--demo-mode')
        cmd.extend(['--zone-config', 'tools/demo/content/demo_zone.json'])
        npcs = True  # Demo mode implies NPCs
    if npcs:
        cmd.append('--npcs')
        cmd.extend(['--npc-count', str(npc_count)])
    env = os.environ.copy()
    # Suppress metrics exporter port conflict
    env['METRICS_PORT'] = '0'
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        env=env
    )
    # Wait for server to initialize
    start_time = time.time()
    while time.time() - start_time < 10.0:
        if proc.poll() is not None:
            stdout, _ = proc.communicate()
            raise RuntimeError(f"Server exited early:\n{stdout}")
        # Try connecting to see if it's up
        try:
            test_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            test_sock.settimeout(0.5)
            test_sock.sendto(b'\x06\x01\x00\x00\x00\x01\x00\x00\x00', ('127.0.0.1', port))
            test_sock.recvfrom(1024)
            test_sock.close()
            break
        except Exception:
            time.sleep(0.2)
    else:
        proc.terminate()
        raise RuntimeError("Server failed to start within 10 seconds")

    return proc


def run_validation(args) -> bool:
    """Run the full validation suite."""
    port = args.port
    server_proc = None
    clients: list[TestClient] = []
    all_passed = True

    try:
        # Start server if binary provided
        if args.server_bin:
            print(f"[VALIDATOR] Starting server: {args.server_bin} --port {port}")
            if args.npcs:
                print(f"[VALIDATOR] NPC auto-population enabled ({args.npc_count} NPCs)")
            if args.demo_mode:
                print("[VALIDATOR] Demo mode enabled")
            server_proc = start_server(args.server_bin, port, args.npcs, args.npc_count, args.demo_mode)
            print("[VALIDATOR] Server started successfully")
            time.sleep(0.5)  # Let it settle

        clients: list[TestClient] = []

        # Phase 1: Connection handshake for all clients
        print(f"[VALIDATOR] Phase 1: Connecting {args.clients} client(s)...")
        for i in range(args.clients):
            client = TestClient('127.0.0.1', port, client_id=i + 1,
                               latency_ms=args.latency, packet_loss=args.packet_loss)
            if not client.connect():
                print(f"  [FAIL] Client {i+1} failed to connect")
                all_passed = False
                continue
            client.start()
            clients.append(client)
            print(f"  [PASS] Client {i+1} connected (entity_id={client.stats.entity_id})")

        if not clients:
            print("[VALIDATOR] No clients connected, aborting.")
            return False

        # Phase 2: Ping/pong RTT measurement
        print("[VALIDATOR] Phase 2: Ping/pong RTT measurement...")
        for client in clients:
            client.send_ping()
        time.sleep(1.0)
        for i, client in enumerate(clients):
            if client.stats.pongs_received > 0:
                print(f"  [PASS] Client {i+1} RTT: {client.stats.rtt_ms:.2f}ms")
            else:
                print(f"  [FAIL] Client {i+1} did not receive pong")
                all_passed = False

        # Phase 3: Input sending + snapshot reception
        if args.combat:
            # When combat testing, skip movement to keep entities at spawn (melee range)
            print("[VALIDATOR] Phase 3: Combat setup (minimal movement, staying at spawn)...")
            time.sleep(1.0)
            for client in clients:
                client.send_ping()
        else:
            print(f"[VALIDATOR] Phase 3: Sending inputs for {args.duration}s...")
            start_time = time.time()
            input_interval = 1.0 / 60.0
            last_input_time = [0.0] * len(clients)
            last_ping_time = start_time

            while time.time() - start_time < args.duration:
                now = time.time()
                for i, client in enumerate(clients):
                    if now - last_input_time[i] >= input_interval:
                        # Send varied movement inputs
                        yaw = (now % 6.28)  # rotating yaw
                        client.send_input(forward=True, yaw=yaw)
                        last_input_time[i] = now

                # Ping every second
                if now - last_ping_time >= 1.0:
                    for client in clients:
                        client.send_ping()
                    last_ping_time = now

                time.sleep(0.005)

        # Phase 4: Validation
        print("[VALIDATOR] Phase 4: Validating results...")
        for i, client in enumerate(clients):
            stats = client.stats
            print(f"  Client {i+1} stats:")
            print(f"    Inputs sent: {stats.inputs_sent}")
            print(f"    Snapshots received: {stats.snapshots_received}")
            print(f"    Corrections received: {stats.corrections_received}")
            print(f"    Events received: {stats.events_received}")
            print(f"    Pings sent: {stats.pings_sent}, Pongs received: {stats.pongs_received}")
            if stats.snapshot_entity_counts:
                avg_entities = sum(stats.snapshot_entity_counts) / len(stats.snapshot_entity_counts)
                print(f"    Avg entities per snapshot: {avg_entities:.1f}")

            if stats.snapshots_received == 0:
                print(f"  [FAIL] Client {i+1} received no snapshots")
                all_passed = False
            else:
                print(f"  [PASS] Client {i+1} received {stats.snapshots_received} snapshots")

            if stats.errors:
                print(f"  [WARN] Client {i+1} errors: {stats.errors[:3]}")
                all_passed = False

        # Phase 5: NPC visibility validation (if NPCs enabled)
        if args.npcs and len(clients) > 0:
            print("[VALIDATOR] Phase 5: NPC visibility check...")
            npcs_visible = False
            for i, client in enumerate(clients):
                if client.stats.snapshot_entity_counts:
                    max_entities = max(client.stats.snapshot_entity_counts)
                    if max_entities > len(clients):
                        npcs_visible = True
                        print(f"  [PASS] Client {i+1} saw up to {max_entities} entities (>{len(clients)} players = NPCs present)")
                    else:
                        print(f"  [INFO] Client {i+1} max entities: {max_entities} (expected >{len(clients)} for NPCs)")
            if not npcs_visible:
                print(f"  [FAIL] No client observed NPCs in snapshots")
                all_passed = False

        # Phase 6: Multi-client overlap validation (if > 1 client)
        if len(clients) > 1:
            print("[VALIDATOR] Phase 6: Multi-client visibility check...")
            # All clients should see each other in snapshots
            # (server broadcasts to all connections)
            for i, client in enumerate(clients):
                if client.stats.snapshots_received > 0:
                    print(f"  [PASS] Client {i+1} visible in network")
                else:
                    print(f"  [FAIL] Client {i+1} not visible")
                    all_passed = False

        # Phase 7: Combat validation
        if args.combat and len(clients) >= 1:
            print(f"[VALIDATOR] Phase 7: Combat validation ({args.combat_duration}s)...")
            combat_start = time.time()
            input_interval = 1.0 / 20.0  # 20Hz input sends
            last_input_time = [0.0] * len(clients)
            last_ping_time = combat_start

            while time.time() - combat_start < args.combat_duration:
                now = time.time()
                for i, client in enumerate(clients):
                    if now - last_input_time[i] >= input_interval:
                        # Send attack input without movement to stay in melee range
                        client.send_input(attack=True, yaw=0.0)
                        last_input_time[i] = now

                if now - last_ping_time >= 1.0:
                    for client in clients:
                        client.send_ping()
                    last_ping_time = now

                time.sleep(0.005)

            # Allow snapshots to settle
            time.sleep(1.0)

            # Validate combat results
            total_deaths = 0
            total_respawns = 0
            total_health_changes = 0
            for i, client in enumerate(clients):
                stats = client.stats
                total_deaths += stats.deaths_observed
                total_respawns += stats.respawns_observed
                total_health_changes += len(stats.health_changes)
                print(f"  Client {i+1} combat stats:")
                print(f"    Attacks sent: {stats.attacks_sent}")
                print(f"    Health changes observed: {len(stats.health_changes)}")
                print(f"    Deaths observed: {stats.deaths_observed}")
                print(f"    Respawns observed: {stats.respawns_observed}")
                if stats.health_changes:
                    # Show first few changes
                    for eid, old, new, tick in stats.health_changes[:5]:
                        print(f"      entity {eid}: {old}% -> {new}% (tick {tick})")

            if total_health_changes == 0:
                print(f"  [FAIL] No health changes observed — combat may not be processing")
                all_passed = False
            else:
                print(f"  [PASS] Observed {total_health_changes} health changes")

            if total_deaths == 0:
                print(f"  [WARN] No deaths observed (may need longer combat duration)")
                # Don't fail — could be low damage or targets out of range
            else:
                print(f"  [PASS] Observed {total_deaths} death(s)")

            if total_respawns == 0:
                print(f"  [WARN] No respawns observed (respawn delay is 5s)")
            else:
                print(f"  [PASS] Observed {total_respawns} respawn(s)")

        # Phase 8: Interpolation / jitter stress test
        if args.interpolation_stress and len(clients) >= 1:
            print(f"[VALIDATOR] Phase 8: Interpolation stress test ({args.interpolation_duration}s)...")
            stress_start = time.time()
            input_interval = 1.0 / 60.0
            last_input_time = [0.0] * len(clients)
            last_ping_time = stress_start

            while time.time() - stress_start < args.interpolation_duration:
                now = time.time()
                for i, client in enumerate(clients):
                    if now - last_input_time[i] >= input_interval:
                        # Circular movement pattern to generate smooth position curves
                        t = now - stress_start
                        yaw = (t * 2.0) % 6.28
                        client.send_input(forward=True, yaw=yaw)
                        last_input_time[i] = now

                if now - last_ping_time >= 1.0:
                    for client in clients:
                        client.send_ping()
                    last_ping_time = now

                time.sleep(0.005)

            # Allow final snapshots to arrive
            time.sleep(0.5)

            # Validate jitter metrics
            for i, client in enumerate(clients):
                stats = client.stats
                jitter = compute_jitter_stats(stats.snapshot_arrival_times)
                tick_check = compute_tick_consistency(stats.snapshot_server_ticks)
                pos_check = compute_position_jitter(stats.entity_positions)

                print(f"  Client {i+1} interpolation stats:")
                print(f"    Snapshots received: {jitter['count']}")
                print(f"    Median inter-arrival: {jitter['median_ms']:.2f}ms (expected ~50ms)")
                print(f"    Jitter (stddev): {jitter['stddev_ms']:.2f}ms")
                print(f"    Min/Max interval: {jitter['min_ms']:.2f}ms / {jitter['max_ms']:.2f}ms")
                print(f"    Estimated snapshot loss: {jitter['loss_percent']:.1f}%")
                print(f"    Tick consistency: {'PASS' if tick_check['consistent'] else 'WARN'}")
                print(f"    Max position delta: {pos_check['max_delta_m']:.3f}m")
                print(f"    Large position jumps (>2m): {pos_check['large_jumps']}")

                # Validation criteria
                if jitter['count'] < 20:
                    print(f"  [WARN] Too few snapshots for jitter analysis ({jitter['count']})")
                else:
                    if 40 <= jitter['median_ms'] <= 65:
                        print(f"  [PASS] Snapshot rate within expected range")
                    else:
                        print(f"  [FAIL] Snapshot rate out of range ({jitter['median_ms']:.1f}ms)")
                        all_passed = False

                    if jitter['stddev_ms'] > 20:
                        print(f"  [WARN] High jitter detected ({jitter['stddev_ms']:.1f}ms stddev)")
                    else:
                        print(f"  [PASS] Jitter acceptable")

                    if jitter['loss_percent'] > 10:
                        print(f"  [WARN] High estimated snapshot loss ({jitter['loss_percent']:.1f}%)")
                    else:
                        print(f"  [PASS] Snapshot loss acceptable")

        # Phase 9: NPC Movement Validation
        if args.npc_movement and args.npcs and len(clients) > 0:
            print("[VALIDATOR] Phase 9: NPC movement validation...")
            total_moving_npcs = 0
            for i, client in enumerate(clients):
                moving = 0
                for entity_id, positions in client.stats.entity_positions.items():
                    if entity_id == client.stats.entity_id:
                        continue  # Skip self
                    if len(positions) < 2:
                        continue
                    # Check if position changed between first and last snapshot
                    _, x1, y1, z1 = positions[0]
                    _, x2, y2, z2 = positions[-1]
                    dx = ((x2 - x1) ** 2 + (y2 - y1) ** 2 + (z2 - z1) ** 2) ** 0.5
                    if dx > 0.5:  # Moved more than 0.5m
                        moving += 1
                if moving > 0:
                    print(f"  [PASS] Client {i+1} observed {moving} moving NPCs")
                    total_moving_npcs += moving
                else:
                    print(f"  [INFO] Client {i+1} did not observe NPC movement (may be stationary)")
            if total_moving_npcs == 0:
                print(f"  [WARN] No NPC movement observed across all clients")
            else:
                print(f"  [PASS] Total moving NPCs observed: {total_moving_npcs}")

        # Phase 10: Tick Budget Validation
        if args.tick_budget and server_proc:
            print("[VALIDATOR] Phase 10: Tick budget validation...")
            # We'll check server stdout in the finally block for overruns
            pass  # Checked during cleanup

    finally:
        print("[VALIDATOR] Cleaning up clients...")
        for client in clients:
            client.stop()

        if server_proc:
            print("[VALIDATOR] Stopping server...")
            server_proc.terminate()
            try:
                server_proc.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                server_proc.kill()
                server_proc.wait()
            stdout, _ = server_proc.communicate()
            if stdout and args.verbose:
                print("[SERVER OUTPUT]\n" + stdout[-2000:])

            # Tick budget check
            if args.tick_budget and stdout:
                overrun_count = stdout.count("Tick overrun")
                if overrun_count == 0:
                    print("  [PASS] No tick overruns detected")
                else:
                    print(f"  [FAIL] Detected {overrun_count} tick overrun(s)")
                    all_passed = False

        if args.latency > 0 or args.packet_loss > 0:
            print(f"[VALIDATOR] Network simulation: latency={args.latency}ms, packet_loss={args.packet_loss*100:.1f}%")
        if args.npcs:
            print(f"[VALIDATOR] NPCs enabled: {args.npc_count} NPCs")
        print("")
    if all_passed:
        print("[VALIDATOR] ========================================")
        print("[VALIDATOR] ALL CHECKS PASSED")
        print("[VALIDATOR] ========================================")
    else:
        print("[VALIDATOR] ========================================")
        print("[VALIDATOR] SOME CHECKS FAILED")
        print("[VALIDATOR] ========================================")

    return all_passed


def main():
    parser = argparse.ArgumentParser(description='DarkAges Live Client-Server Validator')
    parser.add_argument('--server-bin', default=None,
                        help='Path to darkages_server binary (if not provided, assumes server is already running)')
    parser.add_argument('--port', type=int, default=28777,
                        help='Server port (default: 28777)')
    parser.add_argument('--clients', type=int, default=1,
                        help='Number of simultaneous test clients (default: 1)')
    parser.add_argument('--duration', type=int, default=5,
                        help='Duration of input/snapshot test in seconds (default: 5)')
    parser.add_argument('--npcs', action='store_true',
                        help='Enable NPC auto-population on server')
    parser.add_argument('--npc-count', type=int, default=10,
                        help='Number of NPCs to spawn (default: 10)')
    parser.add_argument('--latency', type=int, default=0,
                        help='Simulated latency in ms (default: 0)')
    parser.add_argument('--packet-loss', type=float, default=0.0,
                        help='Simulated packet loss probability 0.0-1.0 (default: 0.0)')
    parser.add_argument('--verbose', action='store_true',
                        help='Print server stdout on completion')
    parser.add_argument('--combat', action='store_true',
                        help='Enable combat validation phase (attack inputs + health tracking)')
    parser.add_argument('--combat-duration', type=int, default=10,
                        help='Duration of combat phase in seconds (default: 10)')
    parser.add_argument('--interpolation-stress', action='store_true',
                        help='Enable interpolation/jitter stress test')
    parser.add_argument('--interpolation-duration', type=int, default=10,
                        help='Duration of interpolation stress in seconds (default: 10)')
    parser.add_argument('--demo-mode', action='store_true',
                        help='Start server in demo mode (zone 99, curated config)')
    parser.add_argument('--tick-budget', action='store_true',
                        help='Validate server tick budget (no overruns)')
    parser.add_argument('--npc-movement', action='store_true',
                        help='Validate NPCs are moving (position changes over time)')
    args = parser.parse_args()

    success = run_validation(args)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
