#!/usr/bin/env python3
"""Godot Client Integration Test for DarkAges MMO

Starts the server, runs the Godot client headless with auto-connect,
validates connection and snapshot reception, then reports results.
"""

import subprocess
import socket
import struct
import sys
import time
import os

SERVER_BIN = "/root/projects/DarkAges/build_validate/darkages_server"
GODOT = "/usr/local/bin/godot"
CLIENT_DIR = "/root/projects/DarkAges/src/client"
PORT = 7777

def wait_for_server(port, timeout=15):
    """Wait for server to accept connections."""
    start = time.time()
    while time.time() - start < timeout:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(1.0)
            req = struct.pack('<BII', 6, 1, 99999)
            sock.sendto(req, ('127.0.0.1', port))
            data, _ = sock.recvfrom(1024)
            sock.close()
            if len(data) >= 10 and data[0] == 7:
                return True
        except Exception:
            pass
        time.sleep(0.3)
    return False

def main():
    print("[TEST] Starting DarkAges Godot Integration Test")
    print("[TEST] ==========================================")

    # Start server
    server_cmd = [
        SERVER_BIN, '--port', str(PORT),
        '--demo-mode',
        '--zone-config', '/root/projects/DarkAges/tools/demo/content/demo_zone.json',
        '--npcs', '--npc-count', '10'
    ]
    print(f"[TEST] Starting server: {' '.join(server_cmd)}")
    server_proc = subprocess.Popen(
        server_cmd,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True
    )

    # Wait for server readiness
    print("[TEST] Waiting for server to be ready...")
    if not wait_for_server(PORT, timeout=15):
        print("[FAIL] Server failed to start within 15s")
        server_proc.terminate()
        server_proc.wait(timeout=5)
        return 1
    print("[TEST] Server is ready")

    # Start Godot client headless (use xvfb-run, not --headless — Godot 4.2 headless rendering has known issues)
    godot_cmd = [
        'xvfb-run', '-a', '--server-args=-screen 0 1280x720x24',
        GODOT, '--path', CLIENT_DIR,
        '--', '--server', '127.0.0.1', '--port', str(PORT),
        '--auto-connect', '--demo-duration', '5'
    ]
    print(f"[TEST] Starting Godot client: {' '.join(godot_cmd)}")
    client_proc = subprocess.Popen(
        godot_cmd,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True
    )

    # Capture and filter output
    start_time = time.time()
    connected = False
    snapshots = 0
    entities = 0
    errors = []
    connection_timeout = False

    while time.time() - start_time < 20 and client_proc.poll() is None:
        line = client_proc.stdout.readline()
        if not line:
            time.sleep(0.1)
            continue
        line = line.strip()
        if not line:
            continue

        # Filter relevant lines
        if any(k in line for k in ['[Main]', '[GameUI]', '[NetworkManager]', '[GameState]', 'Connected', 'snapshot', 'entity', 'ERROR', 'FATAL', 'quit']):
            print(f"  {line}")

        if 'Connection state: Connected' in line:
            connected = True
        if 'Demo duration reached' in line:
            # Parse entity/snapshot counts
            if 'Entities seen:' in line:
                parts = line.split('Entities seen:')
                if len(parts) > 1:
                    rest = parts[1].split(',')[0].strip()
                    try:
                        entities = int(rest)
                    except ValueError:
                        pass
                if 'Snapshots:' in line:
                    sparts = line.split('Snapshots:')
                    if len(sparts) > 1:
                        try:
                            snapshots = int(sparts[1].strip())
                        except ValueError:
                            pass
        if 'Connection timeout' in line:
            connection_timeout = True
        if 'ERROR:' in line or 'FATAL:' in line:
            # Ignore non-fatal Godot scene-tree/headless errors
            if any(ignore in line for ignore in [
                'is_inside_tree', 'add_child', 'mesh_get_surface_count',
                'ObjectDB instances leaked', 'Mouse is not supported',
                'Quaternion is not normalized'
            ]):
                pass  # Headless rendering/math artifact, not a network error
            else:
                errors.append(line)

    # Wait for client to finish if not already done
    try:
        client_proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        client_proc.terminate()
        client_proc.wait(timeout=3)

    # Kill server
    server_proc.send_signal(2)  # SIGINT
    try:
        server_proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        server_proc.kill()
        server_proc.wait(timeout=2)

    # Report
    print("\n[TEST] ==========================================")
    print("[TEST] RESULTS")
    print("[TEST] ==========================================")
    print(f"  Connected:     {'PASS' if connected else 'FAIL'}")
    print(f"  Snapshots:     {snapshots}")
    print(f"  Entities seen: {entities}")
    print(f"  Errors:        {len(errors)}")
    if errors:
        for e in errors[:5]:
            print(f"    - {e}")
    if connection_timeout:
        print("  Connection timed out (server may not have responded)")

    success = connected and snapshots > 5 and len(errors) == 0
    print(f"\n[TEST] OVERALL: {'PASS' if success else 'FAIL'}")
    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
