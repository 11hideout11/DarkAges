#!/usr/bin/env python3
"""DarkAges Test Runner

Runs CTest with retry logic for flaky tests.
"""

import json
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import List

PROJECT_ROOT = Path("/root/projects/DarkAges")
BUILD_DIR = PROJECT_ROOT / "build_validate"

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"


@dataclass
class TestResult:
    passed: bool
    total: int = 0
    failed: int = 0
    failed_tests: List[str] = field(default_factory=list)
    logs: List[str] = field(default_factory=list)


class TestRunner:
    def __init__(self, build_dir: Path = BUILD_DIR, jobs: int = 8):
        self.build_dir = build_dir
        self.jobs = jobs
        self.logs: List[str] = []

    def _log(self, msg: str):
        self.logs.append(msg)
        print(msg)

    def run(self, retry: bool = True) -> TestResult:
        self._log(f"\n{BOLD}{CYAN}{'='*60}{RESET}")
        self._log(f"{BOLD}{CYAN}  TEST RUNNER{RESET}")
        self._log(f"{BOLD}{CYAN}{'='*60}{RESET}")

        cmd = ["ctest", "--output-on-failure", "-j", str(self.jobs)]
        r = subprocess.run(cmd, cwd=str(self.build_dir), capture_output=True, text=True)
        stdout = r.stdout
        stderr = r.stderr

        # Parse summary
        passed = r.returncode == 0
        failed_tests: List[str] = []

        if not passed:
            for line in (stdout + stderr).splitlines():
                if "Failed" in line and "Test #" in line:
                    # Extract test name
                    parts = line.split()
                    if parts:
                        failed_tests.append(parts[-1])

        # Attempt retry of failed tests in isolation
        if not passed and retry and failed_tests:
            self._log(f"{YELLOW}  Retrying {len(failed_tests)} failed test(s)...{RESET}")
            for test in failed_tests:
                rc = subprocess.run(
                    ["ctest", "--output-on-failure", "-R", test],
                    cwd=str(self.build_dir),
                    capture_output=True,
                    text=True,
                )
                if rc.returncode == 0:
                    self._log(f"{GREEN}    {test} passed on retry{RESET}")
                else:
                    self._log(f"{RED}    {test} failed on retry{RESET}")

            # Re-run full suite to get final numbers
            r = subprocess.run(cmd, cwd=str(self.build_dir), capture_output=True, text=True)
            passed = r.returncode == 0
            stdout = r.stdout

        # Extract counts
        total = 0
        failed = 0
        for line in stdout.splitlines():
            if "tests passed" in line.lower():
                # e.g. "100% tests passed, 0 tests failed out of 1212"
                try:
                    parts = line.split(",")
                    for p in parts:
                        if "out of" in p:
                            total = int(p.split("out of")[-1].strip())
                        if "failed" in p:
                            failed = int(p.split()[0].strip())
                except Exception:
                    pass

        if passed:
            self._log(f"{GREEN}  All tests passed ({total} tests){RESET}")
        else:
            self._log(f"{RED}  {failed} test(s) failed out of {total}{RESET}")

        return TestResult(passed=passed, total=total, failed=failed, logs=self.logs)

    def save_json(self, path: Path):
        data = {"passed": False, "logs": self.logs}
        path.write_text(json.dumps(data, indent=2))


def main():
    runner = TestRunner()
    result = runner.run()
    runner.save_json(PROJECT_ROOT / "tools/demo/artifacts/test_report.json")
    sys.exit(0 if result.passed else 1)


if __name__ == "__main__":
    main()
