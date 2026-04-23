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
    def _udp_ping(self, timeout: float = 3.0) -> bool:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(timeout)
            sock.sendto(b'\x04', (self.host, self.port))
            data, _ = sock.recvfrom(1024)
            sock.close()
            return len(data) >= 1 and data[0] == PACKET_PONG
        except Exception:
            return False

    def _handshake(self) -> tuple[bool, int]:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(5.0)
            client_id = 9999
            request = struct.pack('<BII', PACKET_CONNECTION_REQUEST, 1, client_id)
            sock.sendto(request, (self.host, self.port))
            data, _ = sock.recvfrom(1024)
            if len(data) >= 10:
                pkt_type, success, entity_id, zone_id = struct.unpack('<BBII', data)
                return pkt_type == PACKET_CONNECTION_RESPONSE and success == 1, entity_id
            return False, 0
        except Exception:
            return False, 0

    def _count_snapshots(self, duration: float = 5.0) -> int:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(5.0)
            client_id = 9999
            request = struct.pack('<BII', PACKET_CONNECTION_REQUEST, 1, client_id)
            sock.sendto(request, (self.host, self.port))
            data, _ = sock.recvfrom(1024)  # consume response

            start = time.time()
            count = 0
            sock.settimeout(1.0)
            while time.time() - start < duration:
                try:
                    data, _ = sock.recvfrom(4096)
                    if len(data) >= 1 and data[0] == PACKET_SNAPSHOT:
                        count += 1
                except socket.timeout:
                    continue
            sock.close()
            return count
        except Exception:
            return 0

    def _udp_ping_with_retry(self, max_wait: float = 10.0) -> bool:
        start = time.time()
        while time.time() - start < max_wait:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                sock.settimeout(2.0)
                # First handshake to establish connection
                client_id = 9998
                request = struct.pack('<BII', PACKET_CONNECTION_REQUEST, 1, client_id)
                sock.sendto(request, (self.host, self.port))
                data, _ = sock.recvfrom(1024)
                if len(data) < 10:
                    time.sleep(0.5)
                    continue
                pkt_type, success, entity_id, zone_id = struct.unpack('<BBII', data)
                if pkt_type != PACKET_CONNECTION_RESPONSE or success != 1:
                    time.sleep(0.5)
                    continue
                # Now send ping (type 4 + uint32 timestamp)
                import time as time_mod
                ping_time = int(time_mod.time() * 1000) & 0xFFFFFFFF
                ping_pkt = struct.pack('<BI', PACKET_PING, ping_time)
                sock.sendto(ping_pkt, (self.host, self.port))
                data, _ = sock.recvfrom(1024)
                sock.close()
                if len(data) >= 5 and data[0] == PACKET_PONG:
                    return True
            except Exception:
                pass
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

        # 4. Log scan
        server_log = PROJECT_ROOT / "tools/demo/artifacts/logs"
        errors = self._scan_logs_for_errors(server_log)
        ok = len(errors) == 0
        self.checks.append(Check("Server Logs Clean", ok, f"{len(errors)} errors" if not ok else "No errors"))

        # 5. Binary exists
        binary = PROJECT_ROOT / "build_validate/darkages_server"
        ok = binary.exists() and os.access(binary, os.X_OK)
        self.checks.append(Check("Server Binary", ok, str(binary)))

        self._print_report()
        return self.checks

    def _scan_logs_for_errors(self, log_dir: Path) -> List[str]:
        errors = []
        if not log_dir.exists():
            return errors
        for f in log_dir.glob("*.log"):
            try:
                text = f.read_text()
                for line in text.splitlines():
                    if "error" in line.lower() or "fatal" in line.lower() or "assertion" in line.lower():
                        errors.append(f"{f.name}: {line.strip()}")
            except Exception:
                pass
        return errors

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
