#!/usr/bin/env python3
"""DarkAges Log Aggregator

Real-time colorized log tail with critical pattern detection.
"""

import os
import re
import sys
import threading
import time
from pathlib import Path
from typing import Dict, List, Optional, Callable

PROJECT_ROOT = Path("/root/projects/DarkAges")

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"
MAGENTA = "\033[95m"


CRITICAL_PATTERNS = [
    (re.compile(r"segmentation fault|sigsegv|segfault", re.IGNORECASE), RED, "SEGFAULT"),
    (re.compile(r"assertion failed|assert\s*\(|ASSERT", re.IGNORECASE), RED, "ASSERT"),
    (re.compile(r"bind\(\).*address already in use|eaddrinuse", re.IGNORECASE), YELLOW, "PORT_CONFLICT"),
    (re.compile(r"server tick exceeded budget|tick budget exceeded", re.IGNORECASE), YELLOW, "TICK_BUDGET"),
    (re.compile(r"exception|throw|std::|runtime_error", re.IGNORECASE), RED, "EXCEPTION"),
]

SEVERITY_COLORS = {
    "ERROR": RED,
    "WARN": YELLOW,
    "WARNING": YELLOW,
    "INFO": GREEN,
    "DEBUG": CYAN,
    "METRIC": MAGENTA,
}


class LogAggregator:
    def __init__(self, output_path: Optional[Path] = None):
        self.output_path = output_path or (PROJECT_ROOT / "tools/demo/artifacts/logs/combined.log")
        self.output_path.parent.mkdir(parents=True, exist_ok=True)
        self._running = False
        self._threads: List[threading.Thread] = []
        self._callbacks: List[Callable] = []
        self._outfile = None
        self._alerted_patterns: set = set()

    def _colorize(self, line: str, source: str) -> str:
        # Detect severity keyword
        upper = line.upper()
        color = RESET
        for keyword, c in SEVERITY_COLORS.items():
            if keyword in upper:
                color = c
                break

        # Check critical patterns
        for pattern, pat_color, label in CRITICAL_PATTERNS:
            if pattern.search(line):
                key = (label, line[:80])
                if key not in self._alerted_patterns:
                    self._alerted_patterns.add(key)
                    self._alert(f"[{source}] {label}: {line.strip()}", pat_color)
                break

        return f"{color}{line}{RESET}"

    def _alert(self, msg: str, color: str):
        print(f"\n{color}{BOLD}!!! ALERT: {msg}{RESET}\n")
        for cb in self._callbacks:
            try:
                cb("alert", msg)
            except Exception:
                pass

    def _tail(self, filepath: Path, source: str):
        """Tail a log file and print/process lines."""
        while self._running:
            try:
                if not filepath.exists():
                    time.sleep(0.5)
                    continue
                with open(filepath, "r") as f:
                    # Seek to end
                    f.seek(0, 2)
                    while self._running:
                        line = f.readline()
                        if not line:
                            time.sleep(0.2)
                            continue
                        ts = time.strftime("%H:%M:%S")
                        out = f"[{ts}] [{source}] {line.rstrip()}"
                        colored = self._colorize(out, source)
                        # Print to terminal (but suppress in non-interactive to avoid spam)
                        # Instead write to combined log
                        if self._outfile:
                            self._outfile.write(out + "\n")
                            self._outfile.flush()
            except Exception as e:
                if self._running:
                    print(f"{YELLOW}LogAggregator tail error for {source}: {e}{RESET}")
                time.sleep(1)

    def start(self, log_paths: Dict[str, Path]):
        """Start tailing the given log files."""
        self._running = True
        self._outfile = open(self.output_path, "a")
        for source, path in log_paths.items():
            t = threading.Thread(target=self._tail, args=(path, source), daemon=True)
            t.start()
            self._threads.append(t)

    def stop(self):
        self._running = False
        for t in self._threads:
            t.join(timeout=1)
        if self._outfile:
            self._outfile.close()
            self._outfile = None

    def on_alert(self, callback: Callable):
        self._callbacks.append(callback)
