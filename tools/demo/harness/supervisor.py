#!/usr/bin/env python3
"""DarkAges Demo Supervisor

Manages server and client processes with health checks, auto-restart,
port escalation, memory guard, and circuit breaker.
"""

import json
import os
import signal
import socket
import struct
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Callable

PROJECT_ROOT = Path("/root/projects/DarkAges")
SERVER_BIN = PROJECT_ROOT / "build_validate/darkages_server"
GODOT_PATH = "/usr/local/bin/godot"
CLIENT_PATH = PROJECT_ROOT / "src/client"

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"


@dataclass
class ProcessState:
    proc: Optional[subprocess.Popen] = None
    role: str = ""  # "server" or "client"
    restart_count: int = 0
    last_start: float = 0.0
    healthy: bool = False
    log_path: Optional[Path] = None
    log_file = None
    last_log_size: int = 0
    last_log_time: float = 0.0


class Supervisor:
    def __init__(
        self,
        server_bin: Path = SERVER_BIN,
        godot_path: str = GODOT_PATH,
        client_path: Path = CLIENT_PATH,
        max_restarts: int = 3,
        restart_window: float = 60.0,
        base_port: int = 7777,
    ):
        self.server_bin = server_bin
        self.godot_path = godot_path
        self.client_path = client_path
        self.max_restarts = max_restarts
        self.restart_window = restart_window
        self.base_port = base_port
        self.server = ProcessState(role="server")
        self.clients: Dict[str, ProcessState] = {}
        self.running = False
        self.health_interval = 2.0
        self._thread: Optional[threading.Thread] = None
        self._callbacks: List[Callable] = []
        self.server_port = base_port
        self.log_dir = PROJECT_ROOT / "tools/demo/artifacts/logs"
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self._circuit_tripped = False
        self._restart_times: List[float] = []
        self._consecutive_udp_failures = 0
        self._udp_failure_threshold = 5  # Allow 5 consecutive failures (~10s) before zombie kill
        self._startup_grace_sec = 5.0    # Don't declare zombie within 5s of startup
        self._memory_guard_enabled = True
        self._memory_warn_threshold = 85.0
        self._memory_kill_threshold = 92.0

    def _log(self, msg: str):
        print(msg)

    def _open_log(self, name: str) -> Path:
        ts = time.strftime("%Y%m%d_%H%M%S")
        path = self.log_dir / f"{name}_{ts}.log"
        return path

    def _check_circuit_breaker(self) -> bool:
        """Return True if OK, False if circuit is tripped."""
        now = time.time()
        # Keep only restarts within the window
        self._restart_times = [t for t in self._restart_times if now - t < self.restart_window]
        if len(self._restart_times) >= self.max_restarts:
            if not self._circuit_tripped:
                self._log(f"{RED}  CIRCUIT BREAKER TRIPPED: {len(self._restart_times)} restarts in {int(self.restart_window)}s. Halting demo.{RESET}")
                self._circuit_tripped = True
            return False
        return True

    def _record_restart(self):
        self._restart_times.append(time.time())

    def _find_available_port(self) -> int:
        """Find next available port starting from base_port."""
        for port in range(self.base_port, self.base_port + 100):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                sock.bind(("127.0.0.1", port))
                sock.close()
                return port
            except OSError:
                continue
        raise RuntimeError("No available UDP ports in range")

    def _write_active_port(self):
        port_file = PROJECT_ROOT / "tools/demo/artifacts/active_port.txt"
        port_file.write_text(str(self.server_port))

    def start_server(self, args: List[str] = None) -> bool:
        if self._circuit_tripped:
            self._log(f"{RED}  Cannot start server: circuit breaker tripped{RESET}")
            return False

        if self.server.proc and self.server.proc.poll() is None:
            self._log(f"{YELLOW}  Server already running (PID {self.server.proc.pid}){RESET}")
            return True

        # Port escalation if needed
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.bind(("127.0.0.1", self.server_port))
            sock.close()
        except OSError:
            old_port = self.server_port
            self.server_port = self._find_available_port()
            self._log(f"{YELLOW}  Port {old_port} in use. Escalated to {self.server_port}{RESET}")

        cmd = [str(self.server_bin), "--port", str(self.server_port)]
        if args:
            cmd += args

        log_path = self._open_log("server")
        self.server.log_path = log_path
        self.server.log_file = open(log_path, "w")

        self._log(f"  Starting server: {' '.join(cmd)}")
        try:
            self.server.proc = subprocess.Popen(
                cmd, stdout=self.server.log_file, stderr=subprocess.STDOUT,
                cwd=str(self.server_bin.parent)
            )
            self.server.last_start = time.time()
            self.server.restart_count += 1
            self._record_restart()
            self._write_active_port()
            self._log(f"{GREEN}  Server started (PID {self.server.proc.pid}), port {self.server_port}, log: {log_path}{RESET}")
            return True
        except Exception as e:
            self._log(f"{RED}  Failed to start server: {e}{RESET}")
            return False

    def start_client(self, client_id: str = "main", headless: bool = False, extra_args: List[str] = None) -> bool:
        if client_id in self.clients and self.clients[client_id].proc and self.clients[client_id].proc.poll() is None:
            self._log(f"{YELLOW}  Client {client_id} already running{RESET}")
            return True

        cmd = [self.godot_path, "--path", str(self.client_path)]
        if headless:
            cmd += ["--headless", "--display-driver", "headless", "--rendering-driver", "dummy"]
        cmd += ["--", "--server", "127.0.0.1", "--port", str(self.server_port)]
        if extra_args:
            cmd += extra_args

        log_path = self._open_log(f"client_{client_id}")
        log_file = open(log_path, "w")

        self._log(f"  Starting client {client_id}: {' '.join(cmd)}")
        try:
            proc = subprocess.Popen(cmd, stdout=log_file, stderr=subprocess.STDOUT)
            state = ProcessState(proc=proc, role="client", log_path=log_path, log_file=log_file)
            state.last_start = time.time()
            state.restart_count += 1
            state.last_log_time = time.time()
            self.clients[client_id] = state
            self._log(f"{GREEN}  Client {client_id} started (PID {proc.pid}){RESET}")
            return True
        except Exception as e:
            self._log(f"{RED}  Failed to start client {client_id}: {e}{RESET}")
            return False

    def stop_server(self, graceful: bool = True):
        if not self.server.proc:
            return
        self._log(f"  Stopping server (PID {self.server.proc.pid})...")
        if graceful:
            self.server.proc.send_signal(signal.SIGINT)
            try:
                self.server.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.server.proc.kill()
                self.server.proc.wait(timeout=2)
        else:
            self.server.proc.kill()
        if self.server.log_file:
            self.server.log_file.close()
            self.server.log_file = None
        self.server.proc = None
        self.server.healthy = False
        self._log(f"{GREEN}  Server stopped{RESET}")

    def stop_client(self, client_id: str = "main", graceful: bool = True):
        state = self.clients.get(client_id)
        if not state or not state.proc:
            return
        self._log(f"  Stopping client {client_id} (PID {state.proc.pid})...")
        if graceful:
            state.proc.terminate()
            try:
                state.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                state.proc.kill()
                state.proc.wait(timeout=2)
        else:
            state.proc.kill()
        if state.log_file:
            state.log_file.close()
            state.log_file = None
        state.proc = None
        state.healthy = False
        self._log(f"{GREEN}  Client {client_id} stopped{RESET}")

    def stop_all(self):
        self.running = False
        if self._thread:
            self._thread.join(timeout=3)
        for cid in list(self.clients.keys()):
            self.stop_client(cid)
        self.stop_server()

    def _probe_server_udp(self) -> bool:
        """Probe server UDP with proper handshake + ping."""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(2.0)
            # First establish a connection
            client_id = 99999
            request = struct.pack('<BII', 6, 1, client_id)  # PACKET_CONNECTION_REQUEST
            sock.sendto(request, ("127.0.0.1", self.server_port))
            data, _ = sock.recvfrom(1024)
            if len(data) < 10:
                sock.close()
                return False
            pkt_type, success = struct.unpack('<BB', data[:2])
            if pkt_type != 7 or success != 1:  # PACKET_CONNECTION_RESPONSE
                sock.close()
                return False
            # Now send ping
            import time as time_mod
            ping_time = int(time_mod.time() * 1000) & 0xFFFFFFFF
            ping_pkt = struct.pack('<BI', 4, ping_time)  # PACKET_PING
            sock.sendto(ping_pkt, ("127.0.0.1", self.server_port))
            data, _ = sock.recvfrom(1024)
            sock.close()
            return len(data) >= 1 and data[0] == 5  # PACKET_PONG
        except Exception:
            return False

    def _probe_client_log(self, state: ProcessState) -> bool:
        """Check if client log file is growing (client is active)."""
        if not state.log_path or not state.log_path.exists():
            return False
        try:
            size = state.log_path.stat().st_size
            now = time.time()
            if size > state.last_log_size:
                state.last_log_size = size
                state.last_log_time = now
                return True
            # Log hasn't grown in 15s → consider stale
            if now - state.last_log_time > 15.0:
                return False
            return True
        except Exception:
            return False

    def _check_memory(self):
        """Monitor system memory and warn/kill if thresholds exceeded."""
        if not self._memory_guard_enabled:
            return
        try:
            import psutil
            mem = psutil.virtual_memory()
            if mem.percent >= self._memory_kill_threshold:
                self._log(f"{RED}  MEMORY GUARD: {mem.percent}% RAM used. Killing client, then server.{RESET}")
                for cid in list(self.clients.keys()):
                    self.stop_client(cid)
                self.stop_server()
                self.running = False
            elif mem.percent >= self._memory_warn_threshold:
                self._log(f"{YELLOW}  MEMORY WARNING: {mem.percent}% RAM used{RESET}")
        except ImportError:
            pass
        except Exception as e:
            self._log(f"{YELLOW}  Memory check error: {e}{RESET}")

    def _health_check(self):
        # Memory guard first
        self._check_memory()

        # Server health
        if self.server.proc:
            if self.server.proc.poll() is not None:
                self._log(f"{RED}  Server exited with code {self.server.proc.poll()}{RESET}")
                self.server.healthy = False
                if self.running and self._check_circuit_breaker():
                    self._log(f"{YELLOW}  Restarting server (attempt {self.server.restart_count + 1}){RESET}")
                    self.start_server()
                else:
                    self._log(f"{RED}  Server restart limit reached or circuit tripped. Halting.{RESET}")
                    self.running = False
            else:
                uptime = time.time() - self.server.last_start
                udp_ok = self._probe_server_udp()
                if not udp_ok:
                    self._consecutive_udp_failures += 1
                    if uptime > self._startup_grace_sec and self._consecutive_udp_failures >= self._udp_failure_threshold:
                        # Zombie detection: PID alive but port dead for multiple checks after grace period
                        self._log(f"{YELLOW}  Server PID {self.server.proc.pid} alive but UDP port {self.server_port} unresponsive for {self._consecutive_udp_failures} checks. Killing zombie...{RESET}")
                        self.server.proc.kill()
                        self.server.proc.wait(timeout=2)
                        self.server.healthy = False
                        self._consecutive_udp_failures = 0
                        if self.running and self._check_circuit_breaker():
                            self.start_server()
                    else:
                        self._log(f"{YELLOW}  Server UDP probe failed ({self._consecutive_udp_failures}/{self._udp_failure_threshold}){RESET}")
                else:
                    self._consecutive_udp_failures = 0
                    self.server.healthy = True
        else:
            self.server.healthy = False

        # Client health
        for cid, state in list(self.clients.items()):
            if state.proc:
                if state.proc.poll() is not None:
                    self._log(f"{YELLOW}  Client {cid} exited (code {state.proc.poll()}){RESET}")
                    state.healthy = False
                    # Restart client once, preserve server
                    if self.running and state.restart_count < self.max_restarts:
                        self._log(f"{YELLOW}  Restarting client {cid}{RESET}")
                        self.start_client(cid)
                else:
                    state.healthy = self._probe_client_log(state)
                    if not state.healthy:
                        self._log(f"{YELLOW}  Client {cid} log stale (>15s). Marking unhealthy.{RESET}")
            else:
                state.healthy = False

    def _loop(self):
        while self.running:
            self._health_check()
            for cb in self._callbacks:
                try:
                    cb(self)
                except Exception:
                    pass
            time.sleep(self.health_interval)

    def start_monitoring(self):
        self.running = True
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def on_health_tick(self, callback: Callable):
        self._callbacks.append(callback)

    def status(self) -> dict:
        return {
            "running": self.running,
            "circuit_tripped": self._circuit_tripped,
            "server_port": self.server_port,
            "server": {
                "pid": self.server.proc.pid if self.server.proc else None,
                "healthy": self.server.healthy,
                "restarts": self.server.restart_count,
            },
            "clients": {
                cid: {
                    "pid": s.proc.pid if s.proc else None,
                    "healthy": s.healthy,
                    "restarts": s.restart_count,
                }
                for cid, s in self.clients.items()
            },
        }


def main():
    sup = Supervisor()
    sup.start_server(["--npcs", "--npc-count", "5"])
    sup.start_monitoring()
    try:
        for i in range(30):
            print(json.dumps(sup.status(), indent=2))
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        sup.stop_all()


if __name__ == "__main__":
    main()
