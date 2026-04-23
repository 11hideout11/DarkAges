#!/usr/bin/env python3
"""DarkAges Build Manager

Idempotent CMake configure + build with retry logic.
"""

import json
import os
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional

PROJECT_ROOT = Path("/root/projects/DarkAges")
DEFAULT_BUILD_DIR = PROJECT_ROOT / "build_validate"

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"


@dataclass
class BuildResult:
    success: bool
    configure_ok: bool = False
    build_ok: bool = False
    binary_exists: bool = False
    logs: List[str] = field(default_factory=list)
    duration: float = 0.0


class BuildManager:
    def __init__(self, build_dir: Path = DEFAULT_BUILD_DIR, jobs: Optional[int] = None):
        self.build_dir = build_dir
        self.jobs = jobs or os.cpu_count() or 4
        self.binary = build_dir / "darkages_server"
        self.logs: List[str] = []

    def _log(self, msg: str):
        self.logs.append(msg)
        print(msg)

    def _run(self, cmd: List[str], cwd: Path, timeout: int = 300) -> subprocess.CompletedProcess:
        self._log(f"  Running: {' '.join(cmd)}")
        return subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True, timeout=timeout)

    def clean(self):
        if self.build_dir.exists():
            self._log(f"  Removing {self.build_dir}")
            shutil.rmtree(self.build_dir)

    def configure(self, clean: bool = False, gns: bool = False) -> bool:
        if clean and self.build_dir.exists():
            self.clean()
        self.build_dir.mkdir(parents=True, exist_ok=True)

        flags = [
            "-DBUILD_TESTS=ON",
            "-DFETCH_DEPENDENCIES=ON",
            "-DENABLE_GNS=OFF",
            "-DENABLE_REDIS=OFF",
            "-DENABLE_SCYLLA=OFF",
        ]
        if gns:
            flags = [f.replace("OFF", "ON") if "ENABLE_GNS" in f else f for f in flags]

        cmd = ["cmake", "-S", str(PROJECT_ROOT), "-B", str(self.build_dir)] + flags
        result = self._run(cmd, PROJECT_ROOT)
        if result.returncode != 0:
            self._log(f"{RED}  Configure failed:{RESET}\n{result.stderr[-800:]}")
            return False
        self._log(f"{GREEN}  Configure OK{RESET}")
        return True

    def build(self) -> bool:
        cmd = ["cmake", "--build", str(self.build_dir), "-j", str(self.jobs)]
        result = self._run(cmd, PROJECT_ROOT, timeout=600)
        if result.returncode != 0:
            self._log(f"{RED}  Build failed:{RESET}\n{result.stderr[-800:]}")
            return False
        self._log(f"{GREEN}  Build OK{RESET}")
        return True

    def verify_binary(self) -> bool:
        ok = self.binary.exists() and os.access(self.binary, os.X_OK)
        if ok:
            size_mb = self.binary.stat().st_size / (1024 * 1024)
            self._log(f"{GREEN}  Binary verified: {self.binary} ({size_mb:.1f} MB){RESET}")
        else:
            self._log(f"{RED}  Binary missing or not executable: {self.binary}{RESET}")
        return ok

    def run(self, clean: bool = False, retry_on_fail: bool = True) -> BuildResult:
        start = time.time()
        self._log(f"\n{BOLD}{CYAN}{'='*60}{RESET}")
        self._log(f"{BOLD}{CYAN}  BUILD MANAGER{RESET}")
        self._log(f"{BOLD}{CYAN}{'='*60}{RESET}")

        ok = self.configure(clean=clean)
        if not ok and retry_on_fail and not clean:
            self._log(f"{YELLOW}  Retrying with clean build...{RESET}")
            ok = self.configure(clean=True)

        result = BuildResult(success=False, configure_ok=ok)
        if not ok:
            result.duration = time.time() - start
            return result

        ok = self.build()
        result.build_ok = ok
        if not ok and retry_on_fail:
            self._log(f"{YELLOW}  Retrying with clean build...{RESET}")
            self.configure(clean=True)
            ok = self.build()
            result.build_ok = ok

        if ok:
            result.binary_exists = self.verify_binary()
            result.success = result.binary_exists

        result.duration = time.time() - start
        self._log(f"\n  Duration: {result.duration:.1f}s")
        return result

    def save_json(self, path: Path):
        data = {
            "success": False,  # overridden below
            "build_dir": str(self.build_dir),
            "binary": str(self.binary),
            "logs": self.logs,
        }
        path.write_text(json.dumps(data, indent=2))


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--gns", action="store_true")
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    args = parser.parse_args()

    mgr = BuildManager(build_dir=args.build_dir)
    result = mgr.run(clean=args.clean)
    mgr.save_json(PROJECT_ROOT / "tools/demo/artifacts/build_report.json")
    sys.exit(0 if result.success else 1)


if __name__ == "__main__":
    main()
