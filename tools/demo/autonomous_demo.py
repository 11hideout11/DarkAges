#!/usr/bin/env python3
"""DarkAges MMO — Autonomous Demo Orchestrator

One command to bring the entire demo environment online:
  1. Validates all dependencies
  2. Builds server (with retry)
  3. Builds Godot client (with retry)
  4. Starts server → waits for UDP health probe
  5. Starts Godot client → waits for connection + snapshots
  6. Runs live validators
  7. Captures screenshots
  8. Generates report
  9. Graceful shutdown

Usage:
    python3 autonomous_demo.py              # Full demo (~90s)
    python3 autonomous_demo.py --quick      # Quick demo (~45s)
    python3 autonomous_demo.py --extended   # Extended + chaos (~180s)
    python3 autonomous_demo.py --no-build   # Skip builds (fastest)
    python3 autonomous_demo.py --headed     # Launch Godot with GUI
    python3 autonomous_demo.py --dashboard  # Live status dashboard

Exit codes:
    0  = Demo completed successfully
    1  = Dependency/build failure
    2  = Server failed to start after retries
    3  = Client failed to start after retries
    4  = Validation failed
"""

import argparse
import json
import os
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import traceback
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Callable

# ── Paths ──────────────────────────────────────────────────────────────
PROJECT_ROOT = Path("/root/projects/DarkAges")
SERVER_BIN = PROJECT_ROOT / "build_validate/darkages_server"
GODOT_PATH = "/usr/local/bin/godot"
CLIENT_PATH = PROJECT_ROOT / "src/client"
ARTIFACTS = PROJECT_ROOT / "tools/demo/artifacts"
SCREENSHOTS = ARTIFACTS / "screenshots"
LOGS = ARTIFACTS / "logs"
REPORTS = ARTIFACTS / "reports"

# Add harness to path
sys.path.insert(0, str(PROJECT_ROOT / "tools/demo/harness"))

# ── Colors ─────────────────────────────────────────────────────────────
RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"
MAGENTA = "\033[95m"

# ── Logging ────────────────────────────────────────────────────────────
def _ts() -> str:
    return datetime.now().strftime("%H:%M:%S")

def log_info(msg: str):
    print(f"  [{CYAN}{_ts()}{RESET}] {msg}")

def log_ok(msg: str):
    print(f"  [{GREEN}{_ts()}{RESET}] {GREEN}✓{RESET} {msg}")

def log_warn(msg: str):
    print(f"  [{YELLOW}{_ts()}{RESET}] {YELLOW}⚠{RESET} {msg}")

def log_err(msg: str):
    print(f"  [{RED}{_ts()}{RESET}] {RED}✗{RESET} {msg}")

def print_header(text: str):
    print(f"\n{BOLD}{CYAN}{'='*70}{RESET}")
    print(f"{BOLD}{CYAN}{text}{RESET}")
    print(f"{BOLD}{CYAN}{'='*70}{RESET}\n")

def print_phase(phase: int, name: str):
    print(f"\n{BOLD}{MAGENTA}[PHASE {phase}]{RESET} {BOLD}{name}{RESET}")

# ── Data structures ────────────────────────────────────────────────────
@dataclass
class ComponentStatus:
    name: str
    healthy: bool = False
    pid: Optional[int] = None
    proc: Optional[subprocess.Popen] = None
    restarts: int = 0
    last_error: str = ""
    start_time: float = 0.0
    log_path: Optional[Path] = None

@dataclass
class DemoResult:
    success: bool = False
    phases_completed: List[str] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)
    screenshots: List[Path] = field(default_factory=list)
    server_metrics: Dict = field(default_factory=dict)
    client_metrics: Dict = field(default_factory=dict)
    validator_metrics: Dict = field(default_factory=dict)
    duration_sec: float = 0.0

# ── UDP Probes ─────────────────────────────────────────────────────────
def probe_server_udp(port: int, timeout: float = 10.0) -> bool:
    """Probe server UDP with handshake + ping."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(2.0)
            # Handshake
            req = struct.pack('<BII', 6, 1, 99998)
            sock.sendto(req, ('127.0.0.1', port))
            data, _ = sock.recvfrom(1024)
            if len(data) < 10 or data[0] != 7:
                sock.close()
                time.sleep(0.5)
                continue
            # Ping
            ping_time = int(time.time() * 1000) & 0xFFFFFFFF
            sock.sendto(struct.pack('<BI', 4, ping_time), ('127.0.0.1', port))
            data, _ = sock.recvfrom(1024)
            sock.close()
            if len(data) >= 1 and data[0] == 5:
                return True
        except Exception:
            pass
        time.sleep(0.5)
    return False

def is_port_free(port: int) -> bool:
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind(('127.0.0.1', port))
        s.close()
        return True
    except OSError:
        return False

def find_free_port(start: int = 7777, end: int = 7877) -> int:
    for p in range(start, end):
        if is_port_free(p):
            return p
    raise RuntimeError(f"No free UDP ports in range {start}-{end}")

# ── Process management ─────────────────────────────────────────────────
def kill_process_tree(pid: int):
    """Kill a process and any children."""
    try:
        subprocess.run(["pkill", "-P", str(pid)], capture_output=True, timeout=3)
        os.kill(pid, signal.SIGKILL)
    except Exception:
        pass

def graceful_stop(proc: subprocess.Popen, timeout: float = 5.0):
    if not proc or proc.poll() is not None:
        return
    try:
        proc.send_signal(signal.SIGTERM)
        proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait(timeout=2)

# ── Screenshot ─────────────────────────────────────────────────────────
def take_screenshot(output_path: Path, xauth_path: Optional[Path] = None) -> bool:
    env = os.environ.copy()
    display = os.environ.get("DISPLAY", ":99")
    if xauth_path and xauth_path.exists():
        env["XAUTHORITY"] = str(xauth_path)
        # Extract display from xauth file (e.g., "hostname/unix:100")
        try:
            r = subprocess.run(["xauth", "-f", str(xauth_path), "list"],
                               capture_output=True, text=True, timeout=2)
            if r.returncode == 0 and r.stdout:
                for line in r.stdout.strip().splitlines():
                    parts = line.split()
                    if parts and "/unix:" in parts[0]:
                        display_num = parts[0].split(":")[-1]
                        display = f":{display_num}"
                        break
        except Exception:
            pass
    env["DISPLAY"] = display
    # Try ffmpeg x11grab (most reliable for xvfb)
    if shutil.which("ffmpeg"):
        try:
            r = subprocess.run(
                ["ffmpeg", "-f", "x11grab", "-s", "1280x720", "-i", display,
                 "-vframes", "1", "-update", "1", "-y", str(output_path)],
                capture_output=True, env=env, timeout=5)
            if r.returncode == 0 and output_path.exists() and output_path.stat().st_size > 5000:
                return True
        except Exception:
            pass
    # Try scrot
    if shutil.which("scrot"):
        try:
            r = subprocess.run(["scrot", "-z", str(output_path)],
                               capture_output=True, env=env, timeout=5)
            if r.returncode == 0 and output_path.exists() and output_path.stat().st_size > 100:
                return True
        except Exception:
            pass
    # Try ImageMagick
    if shutil.which("import"):
        try:
            r = subprocess.run(["import", "-window", "root", str(output_path)],
                               capture_output=True, env=env, timeout=5)
            if r.returncode == 0 and output_path.exists() and output_path.stat().st_size > 100:
                return True
        except Exception:
            pass
    return False

# ── Main Orchestrator ──────────────────────────────────────────────────
class AutonomousOrchestrator:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.server = ComponentStatus(name="Server")
        self.client = ComponentStatus(name="GodotClient")
        self.result = DemoResult()
        self.start_time = 0.0
        self._shutdown_event = threading.Event()
        self._dashboard_thread: Optional[threading.Thread] = None
        self._xauth_path: Optional[Path] = None
        self._port = args.port
        self._screenshot_paths: List[Path] = []
        self._server_log_file = None
        self._client_log_file = None
        self._video_proc: Optional[subprocess.Popen] = None
        self._video_path: Optional[Path] = None
        self._client_connected_time: Optional[float] = None

    # ── Phase 0: Dependencies ──────────────────────────────────────────
    def phase_dependencies(self) -> bool:
        print_phase(0, "Environment Validation")
        checks = []

        # Godot
        if Path(GODOT_PATH).exists():
            try:
                r = subprocess.run([GODOT_PATH, "--version"], capture_output=True, text=True, timeout=5)
                ver = (r.stdout + r.stderr).strip()
                log_ok(f"Godot: {ver}")
                checks.append(("godot", True))
            except Exception as e:
                log_err(f"Godot version check failed: {e}")
                checks.append(("godot", False))
        else:
            log_err(f"Godot not found at {GODOT_PATH}")
            checks.append(("godot", False))

        # .NET
        if shutil.which("dotnet"):
            try:
                r = subprocess.run(["dotnet", "--version"], capture_output=True, text=True, timeout=5)
                log_ok(f".NET SDK: {r.stdout.strip()}")
                checks.append(("dotnet", True))
            except Exception:
                log_err(".NET SDK check failed")
                checks.append(("dotnet", False))
        else:
            log_err(".NET SDK not found")
            checks.append(("dotnet", False))

        # CMake
        if shutil.which("cmake"):
            log_ok("CMake available")
            checks.append(("cmake", True))
        else:
            log_err("CMake not found")
            checks.append(("cmake", False))

        # xvfb-run (optional but recommended)
        if shutil.which("xvfb-run"):
            log_ok("xvfb-run available (headless display)")
            checks.append(("xvfb", True))
        else:
            log_warn("xvfb-run not found — headless mode may fail")
            checks.append(("xvfb", False))

        # ffmpeg (optional — screenshots/video)
        if shutil.which("ffmpeg"):
            log_ok("ffmpeg available (screenshots/video)")
        else:
            log_warn("ffmpeg not found — screenshots may fail")

        # scrot (optional — fallback)
        if shutil.which("scrot"):
            log_ok("scrot available (fallback screenshots)")
        else:
            log_info("scrot not found — using ffmpeg for screenshots")

        # Server binary exists?
        if SERVER_BIN.exists():
            log_ok(f"Server binary exists ({SERVER_BIN.stat().st_size // 1024} KB)")
            checks.append(("server_bin", True))
        else:
            log_warn("Server binary not found — will build")
            checks.append(("server_bin", False))

        # Godot project
        if (CLIENT_PATH / "project.godot").exists():
            log_ok("Godot project found")
            checks.append(("godot_project", True))
        else:
            log_err("Godot project not found")
            checks.append(("godot_project", False))

        # Port check
        if is_port_free(self._port):
            log_ok(f"Port {self._port} is free")
        else:
            old = self._port
            self._port = find_free_port(self._port + 1)
            log_warn(f"Port {old} in use — escalated to {self._port}")

        all_ok = all(v for _, v in checks)
        if not all_ok:
            failed = [n for n, v in checks if not v]
            log_err(f"Missing critical dependencies: {', '.join(failed)}")
            return False

        log_ok("All dependencies validated")
        self.result.phases_completed.append("dependencies")
        return True

    # ── Phase 1: Build Server ──────────────────────────────────────────
    def phase_build_server(self) -> bool:
        if self.args.no_build and SERVER_BIN.exists():
            log_info("Skipping server build (--no-build)")
            self.result.phases_completed.append("build_server")
            return True

        print_phase(1, "Build Server")
        build_dir = PROJECT_ROOT / "build_validate"
        build_dir.mkdir(parents=True, exist_ok=True)

        # Configure
        log_info("Configuring with CMake...")
        cmake_cmd = [
            "cmake", "-S", str(PROJECT_ROOT), "-B", str(build_dir),
            "-DBUILD_TESTS=ON", "-DFETCH_DEPENDENCIES=ON",
            "-DENABLE_GNS=OFF", "-DENABLE_REDIS=OFF", "-DENABLE_SCYLLA=OFF"
        ]
        r = subprocess.run(cmake_cmd, capture_output=True, text=True)
        if r.returncode != 0:
            log_err("CMake configure failed")
            print(r.stderr[-800:])
            return False
        log_ok("CMake configure")

        # Build
        log_info("Building server...")
        jobs = os.cpu_count() or 2
        build_cmd = ["cmake", "--build", str(build_dir), "-j", str(jobs)]
        r = subprocess.run(build_cmd, capture_output=True, text=True)
        if r.returncode != 0:
            log_err("Build failed")
            print(r.stderr[-800:])
            return False

        if not SERVER_BIN.exists():
            log_err("Server binary not found after build")
            return False

        log_ok(f"Server built ({SERVER_BIN.stat().st_size // 1024} KB)")
        self.result.phases_completed.append("build_server")
        return True

    # ── Phase 2: Build Godot Client ────────────────────────────────────
    def phase_build_godot(self) -> bool:
        if self.args.no_build:
            log_info("Skipping Godot build (--no-build)")
            self.result.phases_completed.append("build_godot")
            return True

        print_phase(2, "Build Godot C# Client")

        # Import
        log_info("Importing Godot project...")
        r = subprocess.run(
            [GODOT_PATH, "--headless", "--path", str(CLIENT_PATH), "--import"],
            capture_output=True, text=True, timeout=120
        )
        if r.returncode != 0:
            log_warn("Godot import had issues (non-fatal)")
        else:
            log_ok("Godot import")

        # Build solutions
        log_info("Building C# solutions...")
        r = subprocess.run(
            [GODOT_PATH, "--headless", "--path", str(CLIENT_PATH), "--build-solutions", "--quit"],
            capture_output=True, text=True, timeout=180
        )
        # Godot 4.2 Mono headless often exits -15 (SIGTERM) after successful build
        dll_path = CLIENT_PATH / ".godot/mono/temp/bin/Debug/DarkAgesClient.dll"
        if dll_path.exists() and dll_path.stat().st_size > 10000:
            log_ok(f"C# build complete ({dll_path.stat().st_size // 1024} KB)")
            self.result.phases_completed.append("build_godot")
            return True
        else:
            log_err("C# build failed — DLL not found or too small")
            print(r.stderr[-500:] if r.stderr else "")
            return False

    # ── Phase 3: Start Server ──────────────────────────────────────────
    def phase_start_server(self) -> bool:
        print_phase(3, "Start Server")

        if not SERVER_BIN.exists():
            log_err("Server binary missing — build required")
            return False

        LOGS.mkdir(parents=True, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_path = LOGS / f"server_{ts}.log"
        self._server_log_file = open(log_path, "w")
        self.server.log_path = log_path

        npcs = self.args.npcs
        cmd = [str(SERVER_BIN), "--port", str(self._port), "--npcs", "--npc-count", str(npcs)]
        if self.args.demo_mode:
            cmd.append("--demo-mode")
            zone_cfg = PROJECT_ROOT / "tools/demo/content/demo_zone.json"
            if zone_cfg.exists():
                cmd.extend(["--zone-config", str(zone_cfg)])

        log_info(f"Starting: {' '.join(cmd)}")

        for attempt in range(1, 4):
            proc = subprocess.Popen(
                cmd, stdout=self._server_log_file, stderr=subprocess.STDOUT,
                cwd=str(SERVER_BIN.parent)
            )
            self.server.proc = proc
            self.server.pid = proc.pid
            self.server.start_time = time.time()
            log_info(f"Attempt {attempt}: Server PID {proc.pid}, port {self._port}")

            # Wait for UDP probe
            log_info("Waiting for UDP health probe...")
            if probe_server_udp(self._port, timeout=15.0):
                self.server.healthy = True
                log_ok(f"Server healthy on port {self._port}")
                self.result.phases_completed.append("start_server")
                return True

            # Probe failed — check if process died
            if proc.poll() is not None:
                log_warn(f"Server exited early (code {proc.poll()})")
                self._server_log_file.flush()
                if attempt < 3:
                    time.sleep(1)
                    continue
            else:
                # Process alive but unresponsive — kill and retry
                log_warn("Server unresponsive — killing and retrying")
                graceful_stop(proc, timeout=3)
                if attempt < 3:
                    time.sleep(1)
                    continue

        log_err("Server failed to start after 3 attempts")
        self.server.last_error = "Failed UDP health probe after 3 attempts"
        return False

    # ── Phase 4: Start Godot Client ────────────────────────────────────
    def phase_start_client(self) -> bool:
        print_phase(4, "Start Godot Client")

        if self.args.no_client:
            log_info("Skipping client (--no-client)")
            self.result.phases_completed.append("start_client")
            return True

        duration = self.args.duration
        LOGS.mkdir(parents=True, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_path = LOGS / f"godot_{ts}.log"
        self._client_log_file = open(log_path, "w")
        self.client.log_path = log_path

        # Build Godot command
        if self.args.headed:
            cmd = [
                GODOT_PATH, "--path", str(CLIENT_PATH),
                "--", "--server", "127.0.0.1", "--port", str(self._port),
                "--auto-connect", "--demo-duration", str(duration)
            ]
            log_info("Launching in HEADED mode (GUI)")
        else:
            # Headless via xvfb-run for screenshot support
            if not shutil.which("xvfb-run"):
                log_err("xvfb-run required for headless mode")
                return False
            # Create xauth file for screenshot tools
            self._xauth_path = Path(tempfile.mktemp(suffix=".xauth"))
            cmd = [
                "xvfb-run", "-a", "-f", str(self._xauth_path),
                "--server-args=-screen 0 1280x720x24",
                GODOT_PATH, "--path", str(CLIENT_PATH),
                "--display-driver", "x11", "--rendering-driver", "opengl3",
                "--", "--server", "127.0.0.1", "--port", str(self._port),
                "--auto-connect", "--demo-duration", str(duration)
            ]
            log_info("Launching in HEADLESS mode (xvfb-run + opengl3)")

        log_info(f"Command: {' '.join(cmd[:5])} ...")

        for attempt in range(1, 3):
            env = os.environ.copy()
            if not self.args.headed:
                env["LIBGL_ALWAYS_SOFTWARE"] = "1"
            proc = subprocess.Popen(
                cmd, stdout=self._client_log_file, stderr=subprocess.STDOUT,
                start_new_session=True, env=env
            )
            self.client.proc = proc
            self.client.pid = proc.pid
            self.client.start_time = time.time()
            log_info(f"Attempt {attempt}: Client PID {proc.pid}")

            # Wait for client to establish connection
            log_info("Waiting for client connection...")
            connected = self._wait_for_client_connection(timeout=45.0)
            if connected:
                self.client.healthy = True
                self._client_connected_time = time.time()
                log_ok("Client connected and receiving snapshots")
                self._start_video_recording()
                self.result.phases_completed.append("start_client")
                return True

            if proc.poll() is not None:
                log_warn(f"Client exited early (code {proc.poll()})")
                self._client_log_file.flush()
                if attempt < 2:
                    time.sleep(1)
                    continue
            else:
                log_warn("Client not connecting — killing and retrying")
                graceful_stop(proc, timeout=3)
                if attempt < 2:
                    time.sleep(1)
                    continue

        log_err("Client failed to start after retries")
        self.client.last_error = "Failed to connect after retries"
        return False

    def _wait_for_client_connection(self, timeout: float) -> bool:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.client.log_path and self.client.log_path.exists():
                # Flush file buffer so read_text sees latest data
                if self._client_log_file:
                    self._client_log_file.flush()
                    os.fsync(self._client_log_file.fileno())
                text = self.client.log_path.read_text()
                if "Connection state: Connected" in text:
                    return True
                if "Connection timeout" in text:
                    return False
            if self.client.proc and self.client.proc.poll() is not None:
                return False
            time.sleep(0.5)
        return False

    def _get_display(self) -> str:
        """Extract DISPLAY from xauth file or environment."""
        if self._xauth_path and self._xauth_path.exists():
            try:
                r = subprocess.run(["xauth", "-f", str(self._xauth_path), "list"],
                                   capture_output=True, text=True, timeout=2)
                if r.returncode == 0 and r.stdout:
                    for line in r.stdout.strip().splitlines():
                        parts = line.split()
                        if parts and "/unix:" in parts[0]:
                            display = parts[0].split(":")[-1]
                            return f":{display}"
            except Exception:
                pass
        return os.environ.get("DISPLAY", ":99")

    def _start_video_recording(self) -> None:
        """Start ffmpeg x11grab video recording in the background."""
        if self.args.no_screenshots or not shutil.which("ffmpeg"):
            return
        display = self._get_display()
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        VIDEO_DIR = ARTIFACTS / "videos"
        VIDEO_DIR.mkdir(parents=True, exist_ok=True)
        self._video_path = VIDEO_DIR / f"demo_{ts}.mp4"
        env = os.environ.copy()
        if self._xauth_path:
            env["XAUTHORITY"] = str(self._xauth_path)
        env["DISPLAY"] = display
        cmd = [
            "ffmpeg", "-f", "x11grab", "-r", "30", "-s", "1280x720",
            "-i", display, "-c:v", "libx264", "-preset", "ultrafast",
            "-crf", "28", "-pix_fmt", "yuv420p", "-y", str(self._video_path)
        ]
        log_info(f"Starting video recording on {display} → {self._video_path.name}")
        try:
            self._video_proc = subprocess.Popen(
                cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                env=env, start_new_session=True
            )
        except Exception as e:
            log_warn(f"Failed to start video recording: {e}")
            self._video_proc = None

    def _stop_video_recording(self) -> None:
        """Gracefully stop ffmpeg video recording."""
        if self._video_proc and self._video_proc.poll() is None:
            log_info("Stopping video recording...")
            self._video_proc.send_signal(signal.SIGINT)
            try:
                self._video_proc.wait(timeout=5)
                log_ok(f"Video saved: {self._video_path}")
            except subprocess.TimeoutExpired:
                graceful_stop(self._video_proc, timeout=3)

    # ── Phase 5: Validation ────────────────────────────────────────────
    def phase_validate(self) -> bool:
        print_phase(5, "Live Validation")

        if self.args.no_validation:
            log_info("Skipping validation (--no-validation)")
            self.result.phases_completed.append("validate")
            return True

        # Run Python validator
        validator = PROJECT_ROOT / "tools/validation/live_client_validator.py"
        if validator.exists():
            log_info("Running live_client_validator.py...")
            cmd = [
                sys.executable, str(validator),
                "--port", str(self._port),
                "--clients", str(self.args.validator_clients),
                "--duration", str(min(10, self.args.duration)),
                "--npcs", "--npc-count", str(self.args.npcs),
            ]
            if self.args.validator_deep:
                cmd.extend(["--npc-movement", "--tick-budget"])

            r = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            output = r.stdout
            print(output[-1500:] if len(output) > 1500 else output)

            self.result.validator_metrics = {
                "passed": r.returncode == 0,
                "output": output,
                "error": r.stderr,
            }
            if r.returncode == 0:
                log_ok("Live validator passed")
            else:
                log_warn("Live validator reported issues (non-fatal)")
        else:
            log_warn("live_client_validator.py not found — skipping")

        # Parse client log for metrics
        if self.client.log_path and self.client.log_path.exists():
            self.result.client_metrics = self._parse_client_log(self.client.log_path)
            m = self.result.client_metrics
            log_info(f"Client metrics: {m.get('snapshots', 0)} snapshots, "
                     f"{m.get('entities_seen', 0)} entities, "
                     f"{len(m.get('errors', []))} errors")

        self.result.phases_completed.append("validate")
        return True

    def _parse_client_log(self, log_path: Path) -> dict:
        result = {"connected": False, "snapshots": 0, "entities_seen": 0, "errors": []}
        if not log_path.exists():
            return result
        text = log_path.read_text()
        seen = set()
        for line in text.splitlines():
            if "Connection state: Connected" in line:
                result["connected"] = True
            if "Snapshot received tick=" in line:
                result["snapshots"] += 1
            if "Entity spawned event:" in line:
                parts = line.split("Entity spawned event:")
                if len(parts) > 1:
                    try:
                        eid = int(parts[1].strip().split()[0])
                        seen.add(eid)
                    except Exception:
                        pass
            if "ERROR:" in line or "FATAL:" in line:
                if any(ign in line for ign in [
                    "is_inside_tree", "add_child", "mesh_get_surface_count",
                    "ObjectDB instances leaked", "Mouse is not supported",
                    "Quaternion is not normalized", "BUG: Unreferenced static string",
                    "Pages in use exist at exit", "X connection to",
                    "Custom cursor shape not supported", "Blend file import",
                    "Started the engine as", "Physical screen size"
                ]):
                    continue
                result["errors"].append(line.strip())
        result["entities_seen"] = len(seen)
        return result

    # ── Phase 6: Screenshots ───────────────────────────────────────────
    def phase_screenshots(self) -> bool:
        print_phase(6, "Screenshot Capture")
        self.result.phases_completed.append("screenshots")

        if self.args.no_screenshots:
            log_info("Skipping screenshots (--no-screenshots)")
            return True

        SCREENSHOTS.mkdir(parents=True, exist_ok=True)
        count = self.args.screenshot_count
        interval = self.args.duration / max(count, 1)

        log_info(f"Capturing {count} screenshots over {self.args.duration}s")

        for i in range(count):
            if self._shutdown_event.is_set():
                break
            ts = datetime.now().strftime("%Y%m%d_%H%M%S")
            path = SCREENSHOTS / f"demo_{ts}_{i+1:02d}.png"
            if take_screenshot(path, self._xauth_path):
                self._screenshot_paths.append(path)
                log_ok(f"Screenshot {i+1}/{count}: {path.name}")
            else:
                log_warn(f"Screenshot {i+1}/{count} failed")
            if i < count - 1:
                # Sleep in small chunks so we can respond to shutdown quickly
                slept = 0.0
                while slept < interval and not self._shutdown_event.is_set():
                    time.sleep(0.5)
                    slept += 0.5
        return True

    # ── Phase 7: Instrumentation Validation ──────────────────────────────
    def phase_instrumentation(self) -> bool:
        print_phase(7, "Client Instrumentation Validation")
        validator = PROJECT_ROOT / "tools/demo/client_instrumentation_validator.py"
        if validator.exists():
            log_info("Running client_instrumentation_validator.py...")
            report_path = ARTIFACTS / "instrumentation_report.json"
            cmd = [
                sys.executable, str(validator),
                "--json", "/tmp/darkages_instrumentation.json",
                "--report", str(report_path)
            ]
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            output = r.stdout
            print(output[-1200:] if len(output) > 1200 else output)
            
            self.result.client_metrics["instrumentation_passed"] = (r.returncode == 0)
            if r.returncode == 0:
                log_ok("Instrumentation validation passed")
            else:
                log_warn("Instrumentation validation reported issues")
        else:
            log_warn("client_instrumentation_validator.py not found — skipping")
        
        self.result.phases_completed.append("instrumentation")
        return True

    # ── Phase 8: Report ──────────────────────────────────────────────────
    def phase_report(self) -> bool:
        print_phase(8, "Generate Report")

        REPORTS.mkdir(parents=True, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_path = REPORTS / f"Autonomous_Demo_Report_{ts}.md"
        json_path = ARTIFACTS / f"autonomous_demo_summary_{ts}.json"

        duration = time.time() - self.start_time
        self.result.duration_sec = duration

        # Git commit
        git_commit = "unknown"
        try:
            r = subprocess.run(["git", "-C", str(PROJECT_ROOT), "rev-parse", "--short", "HEAD"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                git_commit = r.stdout.strip()
        except Exception:
            pass

        # Write Markdown report
        md = f"""# DarkAges MMO — Autonomous Demo Report
**Generated:** {ts}
**Git Commit:** `{git_commit}`
**Duration:** {duration:.1f}s
**Overall:** {'✅ PASS' if self.result.success else '❌ FAIL'}

## Phases Completed
"""
        for ph in self.result.phases_completed:
            md += f"- ✅ {ph}\n"

        md += f"\n## Server Status\n"
        md += f"- Port: {self._port}\n"
        md += f"- PID: {self.server.pid}\n"
        md += f"- Healthy: {self.server.healthy}\n"
        md += f"- Restarts: {self.server.restarts}\n"

        md += f"\n## Client Status\n"
        md += f"- PID: {self.client.pid}\n"
        md += f"- Healthy: {self.client.healthy}\n"
        md += f"- Restarts: {self.client.restarts}\n"

        md += f"\n## Client Metrics\n"
        cm = self.result.client_metrics
        md += f"- Connected: {cm.get('connected', False)}\n"
        md += f"- Snapshots: {cm.get('snapshots', 0)}\n"
        md += f"- Entities Seen: {cm.get('entities_seen', 0)}\n"
        md += f"- Errors: {len(cm.get('errors', []))}\n"

        md += f"\n## Screenshots\n"
        for p in self._screenshot_paths:
            md += f"- `{p.name}`\n"

        md += f"\n## Errors\n"
        if self.result.errors:
            for e in self.result.errors:
                md += f"- {e}\n"
        else:
            md += "None\n"

        md += f"\n## Artifacts\n"
        md += f"- JSON Summary: `{json_path}`\n"
        md += f"- Report: `{report_path}`\n"
        if self._video_path and self._video_path.exists():
            md += f"- Video Recording: `{self._video_path}`\n"
        if self.server.log_path:
            md += f"- Server Log: `{self.server.log_path}`\n"
        if self.client.log_path:
            md += f"- Client Log: `{self.client.log_path}`\n"

        report_path.write_text(md)

        # Write JSON summary (after report so phases_completed is complete)
        self.result.phases_completed.append("report")
        summary = {
            "timestamp": ts,
            "git_commit": git_commit,
            "duration_sec": round(duration, 1),
            "success": self.result.success,
            "phases_completed": self.result.phases_completed,
            "errors": self.result.errors,
            "port": self._port,
            "screenshots": [str(p) for p in self._screenshot_paths],
            "video": str(self._video_path) if (self._video_path and self._video_path.exists()) else None,
            "server_metrics": self.result.server_metrics,
            "client_metrics": self.result.client_metrics,
            "validator_metrics": self.result.validator_metrics,
        }
        json_path.write_text(json.dumps(summary, indent=2))

        log_ok(f"Report: {report_path}")
        log_ok(f"JSON:   {json_path}")
        return True

    # ── Dashboard ──────────────────────────────────────────────────────
    def _dashboard_loop(self):
        while not self._shutdown_event.is_set():
            os.system("clear" if os.name != "nt" else "cls")
            print(f"{BOLD}{CYAN}DarkAges Autonomous Demo Dashboard{RESET}")
            print(f"{'='*60}")
            uptime = time.time() - self.start_time
            print(f"Uptime: {uptime:.1f}s  Port: {self._port}")
            print(f"\n{'Component':<15} {'PID':<8} {'Health':<8} {'Restarts':<8}")
            print("-" * 45)

            srv = self.server
            s_health = f"{GREEN}OK{RESET}" if srv.healthy else f"{RED}FAIL{RESET}"
            print(f"{'Server':<15} {srv.pid or '-':<8} {s_health:<20} {srv.restarts:<8}")

            cli = self.client
            c_health = f"{GREEN}OK{RESET}" if cli.healthy else f"{RED}FAIL{RESET}"
            print(f"{'Client':<15} {cli.pid or '-':<8} {c_health:<20} {cli.restarts:<8}")

            print(f"\n{'Phase':<30} {'Status':<10}")
            print("-" * 45)
            all_phases = ["dependencies", "build_server", "build_godot", "start_server",
                          "start_client", "validate", "screenshots", "instrumentation", "report"]
            for ph in all_phases:
                status = f"{GREEN}DONE{RESET}" if ph in self.result.phases_completed else f"{YELLOW}PENDING{RESET}"
                print(f"{ph:<30} {status:<10}")

            print(f"\nScreenshots: {len(self._screenshot_paths)}")
            print(f"Errors: {len(self.result.errors)}")
            print(f"\nPress Ctrl+C to stop")
            time.sleep(1)

    def start_dashboard(self):
        if self.args.dashboard:
            self._dashboard_thread = threading.Thread(target=self._dashboard_loop, daemon=True)
            self._dashboard_thread.start()

    # ── Shutdown ───────────────────────────────────────────────────────
    def shutdown(self):
        print_header("SHUTDOWN")
        self._shutdown_event.set()

        if self._dashboard_thread:
            self._dashboard_thread.join(timeout=2)

        # Stop video recording
        self._stop_video_recording()

        # Stop client first
        if self.client.proc and self.client.proc.poll() is None:
            log_info("Stopping Godot client...")
            graceful_stop(self.client.proc)
            log_ok("Client stopped")

        # Stop server
        if self.server.proc and self.server.proc.poll() is None:
            log_info("Stopping server...")
            graceful_stop(self.server.proc)
            log_ok("Server stopped")

        # Close log files
        if self._server_log_file:
            self._server_log_file.close()
        if self._client_log_file:
            self._client_log_file.close()

        # Cleanup xauth
        if self._xauth_path and self._xauth_path.exists():
            self._xauth_path.unlink()

    # ── Main Run ───────────────────────────────────────────────────────
    def run(self) -> int:
        self.start_time = time.time()
        try:
            self.start_dashboard()

            # Phase 0
            if not self.phase_dependencies():
                self.result.errors.append("Dependency check failed")
                return 1

            # Phase 1
            if not self.phase_build_server():
                self.result.errors.append("Server build failed")
                return 1

            # Phase 2
            if not self.phase_build_godot():
                self.result.errors.append("Godot build failed")
                return 1

            # Phase 3
            if not self.phase_start_server():
                self.result.errors.append("Server start failed")
                return 2

            # Phase 4
            if not self.phase_start_client():
                self.result.errors.append("Client start failed")
                # Don't fail — server is still running for validation

            # Phase 5
            self.phase_validate()

            # Phase 6 (run in background while client runs)
            if self.client.proc and self.client.proc.poll() is None:
                # Wait for client to finish its demo duration (measured from connection, not spawn)
                if self._client_connected_time:
                    elapsed_since_connect = time.time() - self._client_connected_time
                    remaining = max(0, self.args.duration - elapsed_since_connect)
                else:
                    remaining = self.args.duration
                log_info(f"Letting client run for {remaining:.0f}s...")

                # Screenshot loop concurrent with client running
                screenshot_thread = threading.Thread(target=self.phase_screenshots, daemon=True)
                screenshot_thread.start()

                # Wait for client to finish
                try:
                    self.client.proc.wait(timeout=remaining + 15)
                except subprocess.TimeoutExpired:
                    log_warn("Client did not exit in time — terminating")
                    graceful_stop(self.client.proc)
                screenshot_thread.join(timeout=5)
            else:
                self.phase_screenshots()

            # Phase 7
            self.phase_instrumentation()

            # Phase 8
            self.result.success = (
                self.server.healthy and
                (self.args.no_client or self.client.healthy) and
                len(self.result.errors) == 0
            )
            self.phase_report()

            print_header("DEMO COMPLETE")
            status = f"{GREEN}SUCCESS{RESET}" if self.result.success else f"{RED}PARTIAL/FAIL{RESET}"
            print(f"  Status: {status}")
            print(f"  Duration: {time.time() - self.start_time:.1f}s")
            print(f"  Phases: {len(self.result.phases_completed)}/9")
            print(f"  Screenshots: {len(self._screenshot_paths)}")
            print(f"  Artifacts: {ARTIFACTS}")
            return 0 if self.result.success else 4

        except KeyboardInterrupt:
            print("\n\nInterrupted by user")
            self.result.errors.append("Interrupted by user")
            return 130
        except Exception as e:
            log_err(f"Unexpected error: {e}")
            self.result.errors.append(str(e))
            traceback.print_exc()
            return 1
        finally:
            self.shutdown()


# ── CLI ────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="DarkAges MMO Autonomous Demo Orchestrator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 autonomous_demo.py                    # Full demo
  python3 autonomous_demo.py --quick            # ~45s quick demo
  python3 autonomous_demo.py --extended         # ~180s extended demo
  python3 autonomous_demo.py --no-build         # Skip builds (fastest)
  python3 autonomous_demo.py --headed           # GUI mode
  python3 autonomous_demo.py --dashboard        # Live dashboard
  python3 autonomous_demo.py --no-client        # Server + validator only
        """
    )
    parser.add_argument("--quick", action="store_true", help="Quick demo: 30s, 5 NPCs, 3 screenshots")
    parser.add_argument("--extended", action="store_true", help="Extended demo: 120s, 20 NPCs, 12 screenshots")
    parser.add_argument("--no-build", action="store_true", help="Skip server and Godot builds")
    parser.add_argument("--no-tests", action="store_true", help="Skip CTest suite (deprecated, always skipped)")
    parser.add_argument("--no-client", action="store_true", help="Skip Godot client (server + validator only)")
    parser.add_argument("--no-validation", action="store_true", help="Skip live validation")
    parser.add_argument("--no-screenshots", action="store_true", help="Skip screenshot capture")
    parser.add_argument("--headed", action="store_true", help="Launch Godot with GUI (requires display)")
    parser.add_argument("--dashboard", action="store_true", help="Show live status dashboard")
    parser.add_argument("--duration", type=int, default=60, help="Demo duration in seconds")
    parser.add_argument("--npcs", type=int, default=10, help="Number of NPCs")
    parser.add_argument("--port", type=int, default=7777, help="Server base port")
    parser.add_argument("--screenshot-count", type=int, default=5, help="Number of screenshots")
    parser.add_argument("--validator-clients", type=int, default=2, help="Validator client count")
    parser.add_argument("--validator-deep", action="store_true", help="Run deep validation")
    parser.add_argument("--demo-mode", action="store_true", default=True, help="Use demo zone config")
    args = parser.parse_args()

    # Apply presets
    if args.quick:
        args.duration = 30
        args.npcs = 5
        args.screenshot_count = 3
    elif args.extended:
        args.duration = 120
        args.npcs = 20
        args.screenshot_count = 12

    orch = AutonomousOrchestrator(args)
    sys.exit(orch.run())


if __name__ == "__main__":
    main()
