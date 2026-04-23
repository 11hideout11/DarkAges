#!/usr/bin/env python3
"""
DarkAges MMO Demo Launcher

Comprehensive demo that:
1. Builds the server if needed
2. Starts the server with NPCs
3. Launches the Godot client
4. Runs automated validation
5. Demonstrates all core features

Usage:
    python demo_launcher.py --full-demo
    python demo_launcher.py --server-only
    python demo_launcher.py --client-only
"""

import argparse
import os
import signal
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

# Colors
GREEN = '\033[92m'
YELLOW = '\033[93m'
RED = '\033[91m'
BLUE = '\033[94m'
CYAN = '\033[96m'
RESET = '\033[0m'
BOLD = '\033[1m'

# Paths
PROJECT_ROOT = Path("/root/projects/DarkAges")
BUILD_DIR = PROJECT_ROOT / "build_validate"  # Use existing build
SERVER_BIN = BUILD_DIR / "darkages_server"
GODOT_PATH = "/usr/local/bin/godot"
CLIENT_PATH = PROJECT_ROOT / "src/client"
VALIDATOR_PY = PROJECT_ROOT / "tools/validation/live_client_validator.py"


@dataclass
class DemoState:
    server_process: Optional[subprocess.Popen] = None
    client_process: Optional[subprocess.Popen] = None
    start_time: float = 0.0


def print_header(text: str) -> None:
    print(f"\n{BOLD}{CYAN}{'='*60}{RESET}")
    print(f"{BOLD}{CYAN}{text}{RESET}")
    print(f"{BOLD}{CYAN}{'='*60}{RESET}\n")


def print_step(step: str, status: str = "...") -> None:
    color = GREEN if status == "OK" else (RED if status == "FAIL" else YELLOW)
    print(f"  [{color}{status:4s}{RESET}] {step}")


def check_prerequisites() -> bool:
    """Check all prerequisites are met."""
    print_header("Checking Prerequisites")
    
    # Check Godot
    if Path(GODOT_PATH).exists():
        print_step(f"Godot 4.2 found at {GODOT_PATH}", "OK")
    else:
        print_step(f"Godot not found at {GODOT_PATH}", "FAIL")
        print(f"    Install with: curl -L 'https://...Godot_v4.2.2-stable_mono_linux.x86_64.zip' -o /tmp/godot.zip")
        return False
    
    # Check .NET
    try:
        result = subprocess.run(["dotnet", "--version"], capture_output=True, text=True)
        print_step(f".NET SDK {result.stdout.strip()} installed", "OK")
    except:
        print_step(".NET SDK not found", "FAIL")
        return False
    
    # Check Godot project
    if (CLIENT_PATH / "project.godot").exists():
        print_step("Godot project found", "OK")
    else:
        print_step("Godot project not found", "FAIL")
        return False
    
    return True


def build_server() -> bool:
    """Build the server."""
    print_header("Building Server")
    
    print("  Running CMake configuration...")
    cmake_cmd = [
        "cmake", "-S", str(PROJECT_ROOT), "-B", str(BUILD_DIR),
        "-DBUILD_TESTS=ON", "-DFETCH_DEPENDENCIES=ON",
        "-DENABLE_GNS=OFF", "-DENABLE_REDIS=OFF", "-DENABLE_SCYLLA=OFF"
    ]
    
    result = subprocess.run(cmake_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print_step("CMake configuration failed", "FAIL")
        print(result.stderr[-500:])
        return False
    print_step("CMake configuration", "OK")
    
    print("  Building server...")
    build_cmd = ["cmake", "--build", str(BUILD_DIR), "-j$(nproc)"]
    result = subprocess.run(" ".join(build_cmd), shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        print_step("Build failed", "FAIL")
        print(result.stderr[-500:])
        return False
    
    if SERVER_BIN.exists():
        print_step(f"Server binary built: {SERVER_BIN}", "OK")
    else:
        print_step("Server binary not found", "FAIL")
        return False
    
    return True


def start_server(state: DemoState, npcs: int = 10, port: int = 7777) -> bool:
    """Start the game server."""
    print_header("Starting Server")
    
    cmd = [str(SERVER_BIN), "--port", str(port), "--npcs", "--npc-count", str(npcs)]
    print(f"  Command: {' '.join(cmd)}")
    
    try:
        log_file = open("/tmp/darkages_server.log", "w")
        state.server_process = subprocess.Popen(
            cmd,
            stdout=log_file,
            stderr=subprocess.STDOUT,
            cwd=str(BUILD_DIR)
        )
    except Exception as e:
        print_step(f"Failed to start server: {e}", "FAIL")
        return False
    
    # Wait for server to start
    print("  Waiting for server to initialize...")
    time.sleep(2)
    
    if state.server_process.poll() is not None:
        print_step("Server crashed on startup", "FAIL")
        return False
    
    print_step(f"Server started on port {port} with {npcs} NPCs", "OK")
    return True


def connect_validator(state: DemoState, clients: int = 2, duration: int = 10) -> bool:
    """Run the live client validator."""
    print_header("Running Validator")
    
    cmd = [
        "python3", str(VALIDATOR_PY),
        "--server-bin", str(SERVER_BIN),
        "--port", "7777",
        "--clients", str(clients),
        "--duration", str(duration),
        "--npcs"
    ]
    
    print(f"  Command: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        print_step("Validator passed", "OK")
        # Print summary
        lines = result.stdout.split('\n')
        for line in lines[-20:]:
            if line.strip():
                print(f"    {line}")
        return True
    else:
        print_step("Validator reported issues", "WARN")
        print(result.stdout[-1000:])
        return True  # Don't fail demo for validator issues


def launch_godot_client(state: DemoState) -> bool:
    """Launch the Godot client."""
    print_header("Launching Godot Client")
    
    if not Path(GODOT_PATH).exists():
        print_step("Godot not installed", "FAIL")
        return False
    
    cmd = [
        GODOT_PATH,
        "--path", str(CLIENT_PATH),
        "--",  # Pass remaining args to game
        "--server", "127.0.0.1",
        "--port", "7777"
    ]
    
    print(f"  Command: {' '.join(cmd)}")
    print("  Note: Godot client runs in headed mode (GUI)")
    
    try:
        state.client_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        print_step("Godot client launched", "OK")
        print(f"  Process ID: {state.client_process.pid}")
    except Exception as e:
        print_step(f"Failed to launch: {e}", "FAIL")
        return False
    
    return True


def run_demo_sequence(state: DemoState) -> None:
    """Run the automated demo sequence."""
    print_header("Demo Sequence")
    
    print("""
    {BOLD}DarkAges MMO Demo - Welcome!{RESET}
    
    This demo showcases:
    
    1. {BOLD}Server Startup{RESET}
       - UDP networking with 60Hz tick rate
       - Entity replication to all clients
       - NPC spawning and AI
    
    2. {BOLD}Client Connection{RESET}
       - Entity interpolation
       - Client-side prediction
       - Input handling (WASD + mouse)
    
    3. {BOLD}Combat System{RESET}
       - Melee/ranged attacks
       - Lag compensation
       - NPC aggro and chase behavior
    
    4. {BOLD}Social Features{RESET}
       - Chat system
       - Party invites
       - Guild chat
    
    Press Ctrl+C to stop the demo.
    """)
    
    # Give user time to see the demo info
    time.sleep(5)


def stop_demo(state: DemoState) -> None:
    """Stop all demo processes."""
    print_header("Stopping Demo")
    
    if state.client_process:
        print_step("Stopping Godot client...")
        state.client_process.terminate()
        try:
            state.client_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            state.client_process.kill()
        print_step("Client stopped", "OK")
    
    if state.server_process:
        print_step("Stopping server...")
        state.server_process.send_signal(signal.SIGINT)
        try:
            state.server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            state.server_process.kill()
        print_step("Server stopped", "OK")
    
    duration = time.time() - state.start_time
    print(f"\n  Demo ran for {duration:.1f} seconds")


def main():
    parser = argparse.ArgumentParser(description="DarkAges MMO Demo Launcher")
    parser.add_argument("--build", action="store_true", help="Build server before demo")
    parser.add_argument("--server-only", action="store_true", help="Only start server")
    parser.add_argument("--client-only", action="store_true", help="Only start client")
    parser.add_argument("--npcs", type=int, default=10, help="Number of NPCs to spawn")
    parser.add_argument("--port", type=int, default=7777, help="Server port")
    parser.add_argument("--clients", type=int, default=2, help="Validator clients")
    parser.add_argument("--duration", type=int, default=15, help="Validator duration")
    args = parser.parse_args()
    
    state = DemoState()
    state.start_time = time.time()
    
    try:
        # Check prerequisites
        if not check_prerequisites():
            return 1
        
        # Build if requested
        if args.build or not SERVER_BIN.exists():
            if not build_server():
                return 1
        
        # Start server
        if not args.client_only:
            if not start_server(state, args.npcs, args.port):
                return 1
        
        # Run validator
        if not args.client_only:
            connect_validator(state, args.clients, args.duration)
        
        # Launch client
        if not args.server_only:
            launch_godot_client(state)
        
        # Run demo sequence
        if not args.server_only:
            run_demo_sequence(state)
        
        # Wait for user interrupt or client exit
        if state.client_process:
            state.client_process.wait()
        
    except KeyboardInterrupt:
        print("\n\nDemo interrupted by user")
    finally:
        stop_demo(state)
    
    print_header("Demo Complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
