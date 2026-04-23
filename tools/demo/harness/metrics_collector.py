#!/usr/bin/env python3
"""DarkAges Metrics Collector

Polls server/client metrics and system resources, writes JSON timeseries.
"""

import json
import os
import re
import time
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import List, Optional, Dict

PROJECT_ROOT = Path("/root/projects/DarkAges")


@dataclass
class Snapshot:
    timestamp: float
    server_tick_ms: Optional[float] = None
    entity_count: Optional[int] = None
    snapshot_rate: Optional[float] = None
    server_memory_mb: Optional[float] = None
    client_fps: Optional[float] = None
    cpu_percent: Optional[float] = None
    memory_percent: Optional[float] = None


class MetricsCollector:
    def __init__(self, output_path: Optional[Path] = None, interval_sec: float = 2.0):
        self.output_path = output_path or (PROJECT_ROOT / "tools/demo/artifacts/metrics/metrics.json")
        self.output_path.parent.mkdir(parents=True, exist_ok=True)
        self.interval_sec = interval_sec
        self.snapshots: List[Snapshot] = []
        self._running = False
        self._server_pid: Optional[int] = None
        self._client_pid: Optional[int] = None

    def attach_pids(self, server_pid: Optional[int] = None, client_pid: Optional[int] = None):
        self._server_pid = server_pid
        self._client_pid = client_pid

    def _get_system_metrics(self) -> Dict[str, float]:
        try:
            import psutil
            return {
                "cpu_percent": psutil.cpu_percent(interval=0.5),
                "memory_percent": psutil.virtual_memory().percent,
            }
        except Exception:
            return {}

    def _get_process_memory_mb(self, pid: Optional[int]) -> Optional[float]:
        if pid is None:
            return None
        try:
            import psutil
            proc = psutil.Process(pid)
            return proc.memory_info().rss / (1024 * 1024)
        except Exception:
            return None

    def _parse_server_log_for_metrics(self) -> Dict[str, float]:
        # Try to parse latest server log for tick duration / entity count
        # This is heuristic: look for patterns in stdout if logged
        return {}

    def _parse_client_log_for_fps(self) -> Dict[str, float]:
        return {}

    def collect(self) -> Snapshot:
        sys_metrics = self._get_system_metrics()
        snap = Snapshot(
            timestamp=time.time(),
            server_memory_mb=self._get_process_memory_mb(self._server_pid),
            cpu_percent=sys_metrics.get("cpu_percent"),
            memory_percent=sys_metrics.get("memory_percent"),
        )
        self.snapshots.append(snap)
        return snap

    def run_loop(self, duration_sec: float):
        self._running = True
        end = time.time() + duration_sec
        while self._running and time.time() < end:
            self.collect()
            time.sleep(self.interval_sec)
        self.save()

    def stop(self):
        self._running = False

    def save(self):
        data = {
            "meta": {
                "interval_sec": self.interval_sec,
                "count": len(self.snapshots),
                "start": self.snapshots[0].timestamp if self.snapshots else None,
                "end": self.snapshots[-1].timestamp if self.snapshots else None,
            },
            "snapshots": [asdict(s) for s in self.snapshots],
        }
        self.output_path.write_text(json.dumps(data, indent=2))

    def summary(self) -> Dict:
        if not self.snapshots:
            return {}
        mems = [s.server_memory_mb for s in self.snapshots if s.server_memory_mb is not None]
        cpus = [s.cpu_percent for s in self.snapshots if s.cpu_percent is not None]
        return {
            "server_memory_p50_mb": sorted(mems)[len(mems)//2] if mems else None,
            "server_memory_max_mb": max(mems) if mems else None,
            "cpu_avg": sum(cpus)/len(cpus) if cpus else None,
            "memory_peak_percent": max([s.memory_percent for s in self.snapshots if s.memory_percent is not None] or [0]),
        }
