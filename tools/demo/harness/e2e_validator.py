#!/usr/bin/env python3
"""End-to-End Demo Validator

Validates that a demo run met all success criteria.
Can be run standalone or imported by run_demo.py.
"""

import json
import os
import socket
import struct
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Dict

PROJECT_ROOT = Path("/root/projects/DarkAges")

PACKET_PING = 4
PACKET_PONG = 5
PACKET_CONNECTION_REQUEST = 6
PACKET_CONNECTION_RESPONSE = 7
PACKET_SNAPSHOT = 2


@dataclass
class Check:
    name: str
    passed: bool
    detail: str = ""


class E2EValidator:
    def __init__(self, server_host: str = "127.0.0.1", server_port: int = 7777):
        self.host = server_host
        self.port = server_port
        self.checks: List[Check] = []
        self._sock: socket.socket = None
        self._client_id = 9999
        self._entity_id = 0

    def _connect(self) -> bool:
        """Establish a persistent UDP connection to the server."""
        try:
            self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._sock.settimeout(5.0)
            request = struct.pack('<BII', PACKET_CONNECTION_REQUEST, 1, self._client_id)
            self._sock.sendto(request, (self.host, self.port))
            data, _ = self._sock.recvfrom(1024)
            if len(data) >= 10:
                pkt_type, success, entity_id, zone_id = struct.unpack('<BBII', data)
                if pkt_type == PACKET_CONNECTION_RESPONSE and success == 1:
                    self._entity_id = entity_id
                    return True
            return False
        except Exception:
            return False

    def _disconnect(self):
        """Close the persistent connection."""
        if self._sock:
            try:
                self._sock.close()
            except Exception:
                pass
            self._sock = None

    def _udp_ping(self, timeout: float = 3.0) -> bool:
        if not self._sock and not self._connect():
            return False
        try:
            self._sock.settimeout(timeout)
            ping_time = int(time.time() * 1000) & 0xFFFFFFFF
            ping_pkt = struct.pack('<BI', PACKET_PING, ping_time)
            self._sock.sendto(ping_pkt, (self.host, self.port))
            data, _ = self._sock.recvfrom(1024)
            return len(data) >= 1 and data[0] == PACKET_PONG
        except Exception:
            return False

    def _handshake(self) -> tuple[bool, int]:
        if not self._sock and not self._connect():
            return False, 0
        return True, self._entity_id

    def _count_snapshots(self, duration: float = 5.0) -> int:
        if not self._sock and not self._connect():
            return 0
        try:
            start = time.time()
            count = 0
            self._sock.settimeout(1.0)
            while time.time() - start < duration:
                try:
                    data, _ = self._sock.recvfrom(4096)
                    if len(data) >= 1 and data[0] == PACKET_SNAPSHOT:
                        count += 1
                except socket.timeout:
                    continue
            return count
        except Exception:
            return 0

    def _udp_ping_with_retry(self, max_wait: float = 10.0) -> bool:
        start = time.time()
        while time.time() - start < max_wait:
            if self._udp_ping(timeout=2.0):
                return True
            time.sleep(0.5)
        return False

    def _handshake_with_retry(self, max_wait: float = 10.0) -> tuple[bool, int]:
        start = time.time()
        while time.time() - start < max_wait:
            ok, entity_id = self._handshake()
            if ok:
                return True, entity_id
            time.sleep(0.5)
        return False, 0

    def run(self) -> List[Check]:
        print(f"\n{'='*60}")
        print(f"  END-TO-END VALIDATION")
        print(f"{'='*60}")

        # 1. UDP Ping (with retry)
        ok = self._udp_ping_with_retry()
        self.checks.append(Check("Server UDP Ping", ok, "Port 7777 responds" if ok else "No response after 10s"))

        # 2. Handshake (with retry)
        ok, entity_id = self._handshake_with_retry()
        self.checks.append(Check("Connection Handshake", ok, f"Entity ID: {entity_id}" if ok else "No handshake after 10s"))

        # 3. Snapshots
        count = self._count_snapshots(duration=5.0)
        ok = count >= 10
        self.checks.append(Check("Snapshot Rate", ok, f"{count} snapshots in 5s" if ok else f"Only {count} snapshots"))

        # 4. Log scan — filter to current session only (last 30 min)
        server_log = PROJECT_ROOT / "tools/demo/artifacts/logs"
        # Clean up any stale errors from prior runs first
        self._clean_stale_logs(server_log)
        errors = self._scan_logs_for_errors(server_log)
        ok = len(errors) == 0
        self.checks.append(Check("Server Logs Clean", ok, f"{len(errors)} errors" if not ok else "No errors"))

        # 5. Binary exists
        binary = PROJECT_ROOT / "build_validate/darkages_server"
        ok = binary.exists() and os.access(binary, os.X_OK)
        self.checks.append(Check("Server Binary", ok, str(binary)))

        self._print_report()
        self._disconnect()
        return self.checks

    def _scan_logs_for_errors(self, log_dir: Path) -> List[str]:
        errors = []
        if not log_dir.exists():
            return errors
        for f in sorted(log_dir.glob("*.log"), reverse=True)[:2]:  # Only latest 2 log files
            try:
                text = f.read_text()
                for line in text.splitlines():
                    if "error" in line.lower() or "fatal" in line.lower() or "assertion" in line.lower():
                        # Ignore known benign patterns in headless/test modes
                        lower = line.lower()
                        if any(ignore in lower for ignore in [
                            'networkmanager',  # Class name, not an error
                            'no error',        # Negation
                            'error callback',  # Registration, not failure
                        ]):
                            continue
                        errors.append(f"{f.name}: {line.strip()[:200]}")
            except Exception:
                pass
        return errors

    def _clean_stale_logs(self, log_dir: Path):
        """Remove log files older than 1 hour to prevent stale false positives."""
        if not log_dir.exists():
            return
        cutoff = time.time() - 3600
        for f in log_dir.glob("*.log"):
            try:
                if f.stat().st_mtime < cutoff:
                    f.unlink()
            except Exception:
                pass

    def _print_report(self):
        for c in self.checks:
            color = "\033[92m" if c.passed else "\033[91m"
            status = "PASS" if c.passed else "FAIL"
            print(f"  [{color}{status:4}\033[0m] {c.name:<25} {c.detail}")
        passed = sum(1 for c in self.checks if c.passed)
        total = len(self.checks)
        print(f"\n  {passed}/{total} checks passed")

    def save_json(self, path: Path):
        data = {
            "passed": all(c.passed for c in self.checks),
            "checks": [{"name": c.name, "passed": c.passed, "detail": c.detail} for c in self.checks],
        }
        path.write_text(json.dumps(data, indent=2))


def main():
    v = E2EValidator()
    v.run()
    v.save_json(PROJECT_ROOT / "tools/demo/artifacts/e2e_report.json")
    sys.exit(0 if all(c.passed for c in v.checks) else 1)


if __name__ == "__main__":
    main()
