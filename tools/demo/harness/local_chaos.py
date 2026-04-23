#!/usr/bin/env python3
"""DarkAges Local Chaos Monkey

Bare-metal fault injection for demo resilience testing.
Only active when --chaos flag is passed to run_demo.py.
"""

import os
import signal
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional, List

PROJECT_ROOT = Path("/root/projects/DarkAges")

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"


class LocalChaosMonkey:
    """Inject faults into the demo pipeline to verify resilience."""

    def __init__(self, supervisor):
        self.supervisor = supervisor
        self.actions: List[dict] = []
        self._tc_available = self._check_tc()
        self._stress_ng_available = self._check_stress_ng()

    def _log(self, msg: str, level: str = "info"):
        color = GREEN if level == "ok" else (RED if level == "error" else YELLOW)
        print(f"  [{color}CHAOS{RESET}] {msg}")

    def _check_tc(self) -> bool:
        """Check if tc (traffic control) is available."""
        try:
            subprocess.run(["tc", "qdisc"], capture_output=True, check=True)
            return True
        except Exception:
            return False

    def _check_stress_ng(self) -> bool:
        """Check if stress-ng is available."""
        try:
            subprocess.run(["stress-ng", "--version"], capture_output=True, check=True)
            return True
        except Exception:
            return False

    def _require_root(self) -> bool:
        if os.geteuid() != 0:
            self._log("tc commands require root. Skipping network chaos.", "error")
            return False
        return True

    def _require_supervisor_proc(self, role: str) -> Optional[int]:
        if role == "server":
            proc = self.supervisor.server.proc
        else:
            # Use first client
            for state in self.supervisor.clients.values():
                proc = state.proc
                break
            else:
                proc = None
        if not proc:
            self._log(f"No {role} process found to chaos.", "error")
            return None
        return proc.pid

    def kill_server_graceful(self) -> dict:
        """Send SIGTERM to server, verify supervisor restarts."""
        pid = self._require_supervisor_proc("server")
        if pid is None:
            return {"action": "kill_server_graceful", "status": "skipped", "reason": "no server"}

        self._log(f"Sending SIGTERM to server (PID {pid})...")
        try:
            os.kill(pid, signal.SIGTERM)
            time.sleep(1)
            # Wait for restart
            for i in range(10):
                if self.supervisor.server.proc and self.supervisor.server.proc.poll() is None:
                    self._log(f"Server restarted (PID {self.supervisor.server.proc.pid})", "ok")
                    return {"action": "kill_server_graceful", "status": "pass", "restarted": True}
                time.sleep(1)
            return {"action": "kill_server_graceful", "status": "fail", "reason": "no restart"}
        except Exception as e:
            return {"action": "kill_server_graceful", "status": "error", "reason": str(e)}

    def kill_server_force(self) -> dict:
        """Send SIGKILL to server, verify supervisor restarts."""
        pid = self._require_supervisor_proc("server")
        if pid is None:
            return {"action": "kill_server_force", "status": "skipped", "reason": "no server"}

        self._log(f"Sending SIGKILL to server (PID {pid})...")
        try:
            os.kill(pid, signal.SIGKILL)
            time.sleep(1)
            for i in range(10):
                if self.supervisor.server.proc and self.supervisor.server.proc.poll() is None:
                    self._log(f"Server restarted after SIGKILL (PID {self.supervisor.server.proc.pid})", "ok")
                    return {"action": "kill_server_force", "status": "pass", "restarted": True}
                time.sleep(1)
            return {"action": "kill_server_force", "status": "fail", "reason": "no restart"}
        except Exception as e:
            return {"action": "kill_server_force", "status": "error", "reason": str(e)}

    def kill_client(self) -> dict:
        """Kill the first client process, verify restart."""
        for cid, state in self.supervisor.clients.items():
            if state.proc and state.proc.poll() is None:
                pid = state.proc.pid
                self._log(f"Sending SIGKILL to client {cid} (PID {pid})...")
                try:
                    os.kill(pid, signal.SIGKILL)
                    time.sleep(1)
                    for i in range(10):
                        if cid in self.supervisor.clients and self.supervisor.clients[cid].proc and self.supervisor.clients[cid].proc.poll() is None:
                            self._log(f"Client {cid} restarted (PID {self.supervisor.clients[cid].proc.pid})", "ok")
                            return {"action": "kill_client", "status": "pass", "client": cid, "restarted": True}
                        time.sleep(1)
                    return {"action": "kill_client", "status": "fail", "reason": "no restart"}
                except Exception as e:
                    return {"action": "kill_client", "status": "error", "reason": str(e)}
        return {"action": "kill_client", "status": "skipped", "reason": "no client"}

    def network_delay(self, delay_ms: int = 100) -> dict:
        """Add latency to loopback via tc netem."""
        if not self._tc_available:
            return {"action": "network_delay", "status": "skipped", "reason": "tc not available"}
        if not self._require_root():
            return {"action": "network_delay", "status": "skipped", "reason": "not root"}

        self._log(f"Adding {delay_ms}ms delay to loopback...")
        try:
            subprocess.run(
                ["tc", "qdisc", "add", "dev", "lo", "root", "netem", "delay", f"{delay_ms}ms"],
                capture_output=True, check=True,
            )
            self._log("Network delay applied", "ok")
            return {"action": "network_delay", "status": "pass", "delay_ms": delay_ms}
        except subprocess.CalledProcessError as e:
            # May already exist
            return {"action": "network_delay", "status": "warn", "reason": e.stderr.decode() if e.stderr else str(e)}

    def network_loss(self, percent: int = 5) -> dict:
        """Add packet loss to loopback via tc netem."""
        if not self._tc_available:
            return {"action": "network_loss", "status": "skipped", "reason": "tc not available"}
        if not self._require_root():
            return {"action": "network_loss", "status": "skipped", "reason": "not root"}

        self._log(f"Adding {percent}% packet loss to loopback...")
        try:
            subprocess.run(
                ["tc", "qdisc", "add", "dev", "lo", "root", "netem", "loss", f"{percent}%"],
                capture_output=True, check=True,
            )
            self._log("Packet loss applied", "ok")
            return {"action": "network_loss", "status": "pass", "loss_percent": percent}
        except subprocess.CalledProcessError as e:
            return {"action": "network_loss", "status": "warn", "reason": e.stderr.decode() if e.stderr else str(e)}

    def network_recover(self) -> dict:
        """Remove tc netem from loopback."""
        if not self._tc_available:
            return {"action": "network_recover", "status": "skipped", "reason": "tc not available"}
        if not self._require_root():
            return {"action": "network_recover", "status": "skipped", "reason": "not root"}

        self._log("Removing tc netem from loopback...")
        try:
            subprocess.run(
                ["tc", "qdisc", "del", "dev", "lo", "root"],
                capture_output=True, check=True,
            )
            self._log("Network recovered", "ok")
            return {"action": "network_recover", "status": "pass"}
        except subprocess.CalledProcessError as e:
            # May not exist
            return {"action": "network_recover", "status": "warn", "reason": e.stderr.decode() if e.stderr else str(e)}

    def cpu_spike(self, duration_sec: int = 10, workers: int = 4) -> dict:
        """Spawn stress-ng to create CPU pressure."""
        if not self._stress_ng_available:
            return {"action": "cpu_spike", "status": "skipped", "reason": "stress-ng not installed"}

        self._log(f"Spiking CPU with stress-ng ({workers} workers, {duration_sec}s)...")
        try:
            proc = subprocess.Popen(
                ["stress-ng", "--cpu", str(workers), "--timeout", f"{duration_sec}s", "--quiet"],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            )
            return {"action": "cpu_spike", "status": "pass", "pid": proc.pid, "duration_sec": duration_sec}
        except Exception as e:
            return {"action": "cpu_spike", "status": "error", "reason": str(e)}

    def memory_pressure(self, size_mb: int = 512) -> dict:
        """Allocate a large bytearray to pressure memory."""
        self._log(f"Allocating {size_mb}MB memory pressure...")
        try:
            # Hold reference for 5 seconds then release
            buf = bytearray(size_mb * 1024 * 1024)
            time.sleep(5)
            del buf
            self._log("Memory pressure released", "ok")
            return {"action": "memory_pressure", "status": "pass", "size_mb": size_mb}
        except Exception as e:
            return {"action": "memory_pressure", "status": "error", "reason": str(e)}

    def run_default_sequence(self) -> List[dict]:
        """Run a predefined chaos sequence suitable for demo."""
        results = []
        self._log("=== CHAOS SEQUENCE START ===")

        # 1. CPU spike
        results.append(self.cpu_spike(duration_sec=5, workers=2))
        time.sleep(6)

        # 2. Network delay
        results.append(self.network_delay(delay_ms=50))
        time.sleep(5)

        # 3. Kill client gracefully
        results.append(self.kill_client())
        time.sleep(5)

        # 4. Kill server forcefully
        results.append(self.kill_server_force())
        time.sleep(5)

        # 5. Recover network
        results.append(self.network_recover())

        self._log("=== CHAOS SEQUENCE END ===")
        return results


def main():
    """Standalone test: requires a running supervisor."""
    print("LocalChaosMonkey: import test OK")
    print(f"tc available: {LocalChaosMonkey(None)._tc_available}")
    print(f"stress-ng available: {LocalChaosMonkey(None)._stress_ng_available}")
    print(f"running as root: {os.geteuid() == 0}")


if __name__ == "__main__":
    main()
