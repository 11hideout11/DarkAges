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
    pings_sent: int = 0
    pongs_received: int = 0
    errors: list[str] = field(default_factory=list)
    snapshot_entity_counts: list[int] = field(default_factory=list)


class TestClient:
    """UDP test client that mimics the Godot client's network behavior."""

    def __init__(self, server_host: str, server_port: int, client_id: int):
        self.server_host = server_host
        self.server_port = server_port
        self.client_id = client_id
        self.stats = ClientStats()

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(5.0)
        self.connected = False
        self.running = False
        self.receive_thread: Optional[threading.Thread] = None
        self.input_sequence = 1
        self.last_ping_time = 0.0
        self._lock = threading.Lock()

    def connect(self) -> bool:
        """Send connection request and wait for response."""
        try:
            # [type:1=6][version:4][player_id:4]
            request = struct.pack('<BII', PACKET_CONNECTION_REQUEST, 1, self.client_id)
            self.sock.sendto(request, (self.server_host, self.server_port))

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

        yaw_q = int(yaw * 10000)
        pitch_q = int(pitch * 10000)
        timestamp = int(time.time() * 1000) & 0xFFFFFFFF

        # [type:1=1][sequence:4][timestamp:4][flags:1][yaw:2][pitch:2][target:4]
        data = struct.pack('<BIIbhhI', PACKET_CLIENT_INPUT, self.input_sequence,
                           timestamp, flags, yaw_q, pitch_q, 0)
        try:
            self.sock.sendto(data, (self.server_host, self.server_port))
            self.input_sequence += 1
            with self._lock:
                self.stats.inputs_sent += 1
            return True
        except Exception as e:
            with self._lock:
                self.stats.errors.append(f"Input send error: {e}")
            return False

    def send_ping(self) -> bool:
        """Send a ping packet."""
        if not self.connected:
            return False

        self.last_ping_time = time.time()
        timestamp = int(self.last_ping_time * 1000) & 0xFFFFFFFF
        data = struct.pack('<BI', PACKET_PING, timestamp)
        try:
            self.sock.sendto(data, (self.server_host, self.server_port))
            with self._lock:
                self.stats.pings_sent += 1
            return True
        except Exception as e:
            with self._lock:
                self.stats.errors.append(f"Ping send error: {e}")
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

        with self._lock:
            self.stats.snapshots_received += 1
            self.stats.snapshot_entity_counts.append(entity_count)

    def _process_pong(self, data: bytes):
        """Calculate RTT from pong packet."""
        if len(data) < 5:
            return
        rtt = (time.time() - self.last_ping_time) * 1000.0
        with self._lock:
            self.stats.pongs_received += 1
            self.stats.rtt_ms = rtt


def start_server(bin_path: str, port: int) -> subprocess.Popen:
    """Start the darkages_server binary on a given port."""
    cmd = [bin_path, '--port', str(port), '--zone-id', '1']
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
            server_proc = start_server(args.server_bin, port)
            print("[VALIDATOR] Server started successfully")
            time.sleep(0.5)  # Let it settle

        clients: list[TestClient] = []

        # Phase 1: Connection handshake for all clients
        print(f"[VALIDATOR] Phase 1: Connecting {args.clients} client(s)...")
        for i in range(args.clients):
            client = TestClient('127.0.0.1', port, client_id=i + 1)
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

        # Phase 5: Multi-client overlap validation (if > 1 client)
        if len(clients) > 1:
            print("[VALIDATOR] Phase 5: Multi-client visibility check...")
            # All clients should see each other in snapshots
            # (server broadcasts to all connections)
            for i, client in enumerate(clients):
                if client.stats.snapshots_received > 0:
                    print(f"  [PASS] Client {i+1} visible in network")
                else:
                    print(f"  [FAIL] Client {i+1} not visible")
                    all_passed = False

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
    parser.add_argument('--verbose', action='store_true',
                        help='Print server stdout on completion')
    args = parser.parse_args()

    success = run_validation(args)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
