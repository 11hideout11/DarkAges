#!/usr/bin/env python3
"""DarkAges MMO Demo Installer

One-time setup script. Validates dependencies, builds server, runs tests,
and prepares the demo environment.

Usage:
    python3 tools/demo/install.py
    python3 tools/demo/install.py --clean
"""

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "harness"))

from dependency_check import DependencyChecker
from build_manager import BuildManager
from test_runner import TestRunner
from godot_controller import GodotController

PROJECT_ROOT = Path("/root/projects/DarkAges")
ARTIFACTS = PROJECT_ROOT / "tools/demo/artifacts"

GREEN = "\033[92m"
RED = "\033[91m"
BOLD = "\033[1m"
RESET = "\033[0m"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--clean", action="store_true", help="Clean build before compiling")
    parser.add_argument("--skip-tests", action="store_true", help="Skip test phase")
    args = parser.parse_args()

    print(f"\n{BOLD}DARKAGES DEMO INSTALLER{RESET}\n")

    # Phase 0: Dependencies
    checker = DependencyChecker()
    results = checker.check()
    checker.print_report()
    checker.save_json(ARTIFACTS / "dependency_check.json")
    if not all(r.passed for r in results):
        print(f"{RED}Install halted: dependency check failed.{RESET}")
        sys.exit(1)

    # Phase 1: Build
    mgr = BuildManager()
    result = mgr.run(clean=args.clean)
    mgr.save_json(ARTIFACTS / "build_report.json")
    if not result.success:
        print(f"{RED}Install halted: build failed.{RESET}")
        sys.exit(1)

    # Phase 2: Tests
    if not args.skip_tests:
        runner = TestRunner()
        tresult = runner.run()
        runner.save_json(ARTIFACTS / "test_report.json")
        if not tresult.passed:
            print(f"{RED}Install halted: tests failed.{RESET}")
            sys.exit(1)

    # Phase 3: Godot import/build (best-effort, non-blocking)
    print("\n  Verifying Godot client...")
    ctrl = GodotController()
    if not ctrl.verify_installation():
        print(f"{RED}Godot verification failed.{RESET}")
        sys.exit(1)
    print(f"{GREEN}  Godot installation verified{RESET}")
    # Import can be slow; run it but don't block demo on it
    print(f"  Importing Godot project (this may take a moment)...")
    ctrl.import_project()
    print(f"  Godot client ready.{RESET}")

    # State file
    state = {
        "installed": True,
        "paths": {
            "server_bin": str(PROJECT_ROOT / "build_validate/darkages_server"),
            "godot": "/usr/local/bin/godot",
            "client_project": str(PROJECT_ROOT / "src/client"),
        }
    }
    (PROJECT_ROOT / ".demo_state.json").write_text(json.dumps(state, indent=2))

    print(f"\n{BOLD}{GREEN}Installation complete. Ready for demo.{RESET}")
    print(f"  Run: python3 tools/demo/run_demo.py --smoke")
    print(f"  Run: python3 tools/demo/run_demo.py --full --duration 60\n")


if __name__ == "__main__":
    main()
