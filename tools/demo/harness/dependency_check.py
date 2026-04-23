#!/usr/bin/env python3
"""DarkAges Demo Dependency Checker

Validates all prerequisites for running the DarkAges MMO demo pipeline.
Exit code 0 = all passed, 1 = one or more failures.
"""

import json
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"

PROJECT_ROOT = Path("/root/projects/DarkAges")


@dataclass
class CheckResult:
    name: str
    passed: bool
    message: str = ""
    details: dict = field(default_factory=dict)


class DependencyChecker:
    def __init__(self):
        self.results: List[CheckResult] = []

    def _run(self, cmd: List[str], timeout: int = 10) -> tuple[int, str, str]:
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
            return r.returncode, r.stdout.strip(), r.stderr.strip()
        except FileNotFoundError:
            return -1, "", f"Command not found: {cmd[0]}"
        except subprocess.TimeoutExpired:
            return -1, "", "Timeout"

    def check(self) -> List[CheckResult]:
        self._check_godot()
        self._check_dotnet()
        self._check_cmake()
        self._check_compiler()
        self._check_python()
        self._check_ports()
        self._check_disk_space()
        self._check_godot_project()
        return self.results

    def _check_godot(self):
        path = shutil.which("godot")
        if not path:
            self.results.append(CheckResult("Godot", False, "godot not in PATH"))
            return
        rc, out, err = self._run([path, "--version"])
        version = out or err
        ok = rc == 0 and "4.2" in version
        self.results.append(CheckResult(
            "Godot", ok,
            f"{version} at {path}" if ok else f"Failed: {version}",
            {"path": path, "version": version}
        ))

    def _check_dotnet(self):
        rc, out, _ = self._run(["dotnet", "--version"])
        ok = rc == 0
        self.results.append(CheckResult(
            ".NET SDK", ok,
            f"SDK {out}" if ok else "dotnet not found",
            {"version": out}
        ))

    def _check_cmake(self):
        rc, out, _ = self._run(["cmake", "--version"])
        ok = rc == 0
        version = out.splitlines()[0] if out else ""
        self.results.append(CheckResult(
            "CMake", ok,
            version if ok else "cmake not found",
            {"version": version}
        ))

    def _check_compiler(self):
        for compiler in ["g++", "clang++"]:
            rc, out, _ = self._run([compiler, "--version"])
            if rc == 0:
                self.results.append(CheckResult(
                    "C++ Compiler", True,
                    out.splitlines()[0],
                    {"compiler": compiler}
                ))
                return
        self.results.append(CheckResult("C++ Compiler", False, "No g++ or clang++ found"))

    def _check_python(self):
        v = sys.version_info
        ok = v.major >= 3 and v.minor >= 10
        self.results.append(CheckResult(
            "Python", ok,
            f"{v.major}.{v.minor}.{v.micro}",
            {"version": f"{v.major}.{v.minor}.{v.micro}"}
        ))

    def _check_ports(self):
        busy = []
        for port in [7777, 7778]:
            try:
                with socket_guard() as s:
                    s.bind(("", port))
            except OSError:
                busy.append(port)
        ok = len(busy) == 0
        self.results.append(CheckResult(
            "Ports", ok,
            f"Available: 7777, 7778" if ok else f"Busy: {busy}",
            {"busy": busy}
        ))

    def _check_disk_space(self):
        stat = os.statvfs(PROJECT_ROOT)
        free_gb = (stat.f_bavail * stat.f_frsize) / (1024**3)
        ok = free_gb >= 2.0
        self.results.append(CheckResult(
            "Disk Space", ok,
            f"{free_gb:.1f} GB free" if ok else f"Only {free_gb:.1f} GB free (need 2GB+)",
            {"free_gb": round(free_gb, 2)}
        ))

    def _check_godot_project(self):
        proj = PROJECT_ROOT / "src/client/project.godot"
        ok = proj.exists()
        self.results.append(CheckResult(
            "Godot Project", ok,
            str(proj) if ok else "src/client/project.godot not found",
            {"path": str(proj)}
        ))

    def print_report(self):
        print(f"\n{BOLD}{CYAN}{'='*60}{RESET}")
        print(f"{BOLD}{CYAN}  DARKAGES DEMO DEPENDENCY CHECK{RESET}")
        print(f"{BOLD}{CYAN}{'='*60}{RESET}\n")
        for r in self.results:
            color = GREEN if r.passed else RED
            status = "PASS" if r.passed else "FAIL"
            print(f"  [{color}{status:4}{RESET}] {r.name:<20} {r.message}")
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        print(f"\n{BOLD}{'-'*60}{RESET}")
        color = GREEN if passed == total else RED
        print(f"  {color}{passed}/{total} checks passed{RESET}")
        if passed < total:
            print(f"\n  {YELLOW}Fix failures before running demo.{RESET}")
        print("")

    def save_json(self, path: Path):
        data = {
            "passed": all(r.passed for r in self.results),
            "checks": [
                {"name": r.name, "passed": r.passed, "message": r.message, "details": r.details}
                for r in self.results
            ]
        }
        path.write_text(json.dumps(data, indent=2))


class socket_guard:
    def __enter__(self):
        import socket
        self.s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return self.s
    def __exit__(self, *args):
        self.s.close()


def main():
    checker = DependencyChecker()
    checker.check()
    checker.print_report()
    out = PROJECT_ROOT / "tools/demo/artifacts/dependency_check.json"
    checker.save_json(out)
    sys.exit(0 if all(r.passed for r in checker.results) else 1)


if __name__ == "__main__":
    main()
