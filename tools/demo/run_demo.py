#!/usr/bin/env python3
"""DarkAges MMO Demo Master Runner

One-command entry point for the full demo pipeline.

Usage:
    python3 run_demo.py --smoke              # Quick validation (server + validator)
    python3 run_demo.py --headless --duration 30 --npcs 10
    python3 run_demo.py --full --dashboard   # Server + headed Godot + live UI
    python3 run_demo.py --full --chaos       # Resilience demo with fault injection
"""

import argparse
import json
import os
import signal
import sys
import time
from pathlib import Path

# Add harness to path
sys.path.insert(0, str(Path(__file__).parent / "harness"))

from dependency_check import DependencyChecker
from build_manager import BuildManager
from test_runner import TestRunner
from supervisor import Supervisor
from godot_controller import GodotController
from e2e_validator import E2EValidator
from local_chaos import LocalChaosMonkey
from report_generator import ReportGenerator
from metrics_collector import MetricsCollector

PROJECT_ROOT = Path("/root/projects/DarkAges")
ARTIFACTS = PROJECT_ROOT / "tools/demo/artifacts"

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"


def print_header(text: str):
    print(f"\n{BOLD}{CYAN}{'='*60}{RESET}")
    print(f"{BOLD}{CYAN}{text}{RESET}")
    print(f"{BOLD}{CYAN}{'='*60}{RESET}\n")


def print_step(step: str, status: str = "..."):
    color = GREEN if status == "OK" else (RED if status == "FAIL" else YELLOW)
    print(f"  [{color}{status:4}{RESET}] {step}")


class DemoPipeline:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.supervisor: Supervisor | None = None
        self.godot: GodotController | None = None
        self.chaos: LocalChaosMonkey | None = None
        self.metrics: MetricsCollector | None = None
        self.failed = False
        self.start_time = 0.0

    def run(self) -> int:
        self.start_time = time.time()
        try:
            if not self.phase_dependencies():
                return 1
            if not self.phase_build():
                return 1
            if not self.phase_tests():
                return 1
            if not self.phase_deploy():
                return 1
            if not self.phase_validate():
                return 1
            self.phase_report()
            return 0
        except KeyboardInterrupt:
            print(f"\n{YELLOW}Interrupted by user{RESET}")
            return 130
        finally:
            self.teardown()

    def phase_dependencies(self) -> bool:
        print_header("PHASE 0: Dependencies")
        checker = DependencyChecker()
        results = checker.check()
        checker.print_report()
        checker.save_json(ARTIFACTS / "dependency_check.json")
        ok = all(r.passed for r in results)
        if not ok:
            print_step("Dependency check", "FAIL")
            return False
        print_step("Dependency check", "OK")
        return True

    def phase_build(self) -> bool:
        if self.args.no_build:
            print_header("PHASE 1: Build (skipped)")
            binary = PROJECT_ROOT / "build_validate/darkages_server"
            if not binary.exists():
                print_step("Build skipped but binary missing", "FAIL")
                return False
            print_step("Build skipped, using existing binary", "OK")
            return True

        print_header("PHASE 1: Build")
        mgr = BuildManager()
        result = mgr.run(clean=self.args.clean_build)
        mgr.save_json(ARTIFACTS / "build_report.json")
        if not result.success:
            print_step("Build", "FAIL")
            return False
        print_step("Build", "OK")
        return True

    def phase_tests(self) -> bool:
        if self.args.no_tests:
            print_header("PHASE 2: Tests (skipped)")
            print_step("Tests skipped", "OK")
            return True

        print_header("PHASE 2: Tests")
        runner = TestRunner()
        result = runner.run()
        runner.save_json(ARTIFACTS / "test_report.json")
        if not result.passed:
            print_step("Tests", "FAIL")
            return False
        print_step(f"Tests ({result.total} cases)", "OK")
        return True

    def phase_deploy(self) -> bool:
        print_header("PHASE 3: Deploy")
        self.supervisor = Supervisor()

        # Start server
        server_args = ["--npcs", "--npc-count", str(self.args.npcs)]
        if self.args.demo_content:
            # If server supports zone config, pass it
            # For now, just use NPC count
            pass
        if not self.supervisor.start_server(server_args):
            print_step("Server start", "FAIL")
            return False
        print_step("Server start", "OK")

        # Wait for server to be ready
        time.sleep(2)

        # Start client if requested
        if self.args.full or self.args.headless_client:
            self.godot = GodotController()
            if self.args.headless_client:
                self.godot.launch_headless(server_port=self.supervisor.server_port)
            else:
                self.godot.launch_headed(server_port=self.supervisor.server_port)
            time.sleep(3)
            if not self.godot.is_running():
                print_step("Godot client start", "FAIL")
                return False
            print_step("Godot client start", "OK")

        # Start supervisor health monitoring
        self.supervisor.start_monitoring()
        return True

    def phase_validate(self) -> bool:
        print_header("PHASE 4: Validation")
        duration = self.args.duration
        print(f"  Running demo for {duration} seconds...")

        # Setup metrics collection
        self.metrics = MetricsCollector(interval_sec=2.0)
        if self.supervisor and self.supervisor.server.proc:
            client_pid = None
            for state in self.supervisor.clients.values():
                if state.proc:
                    client_pid = state.proc.pid
                    break
            self.metrics.attach_pids(
                server_pid=self.supervisor.server.proc.pid,
                client_pid=client_pid,
            )

        # Setup chaos if requested
        if self.args.chaos and self.supervisor:
            self.chaos = LocalChaosMonkey(self.supervisor)
            chaos_time = duration // 2
            print(f"  {YELLOW}Chaos enabled: fault injection at t={chaos_time}s{RESET}")

        # Setup dashboard if requested
        if self.args.dashboard:
            from dashboard import DemoDashboard
            dashboard = DemoDashboard(self.supervisor)
            import threading
            dash_thread = threading.Thread(target=dashboard.run, args=(duration,), daemon=True)
            dash_thread.start()

        # Live status prints
        end = time.time() + duration
        chaos_done = False
        while time.time() < end:
            if self.supervisor and not self.supervisor.running:
                if self.args.dashboard:
                    dashboard.stop()
                print_step("Supervisor halted", "FAIL")
                return False
            remaining = int(end - time.time())
            elapsed = duration - remaining
            if self.chaos and elapsed >= chaos_time and not chaos_done:
                chaos_done = True
                if self.args.dashboard:
                    dashboard.stop()
                print(f"\n  {YELLOW}Injecting chaos...{RESET}")
                self.chaos.run_default_sequence()
                print(f"  {YELLOW}Chaos complete. Continuing demo...{RESET}\n")
                if self.args.dashboard:
                    dash_thread = threading.Thread(target=dashboard.run, args=(remaining,), daemon=True)
                    dash_thread.start()
            if not self.args.dashboard:
                print(f"  {CYAN}Demo running... {remaining}s remaining{RESET}", end="\r")
            time.sleep(1)
        if self.args.dashboard:
            dashboard.stop()
        print(f"  {GREEN}Demo runtime complete{' '*20}{RESET}")

        # Stop metrics and save
        if self.metrics:
            self.metrics.save()

        # E2E validation
        validator = E2EValidator(server_port=self.supervisor.server_port if self.supervisor else 7777)
        validator.run()
        validator.save_json(ARTIFACTS / "e2e_report.json")
        ok = all(c.passed for c in validator.checks)
        print_step("E2E validation", "OK" if ok else "FAIL")
        return ok

    def phase_report(self):
        print_header("PHASE 5: Report")
        duration = time.time() - self.start_time
        git_commit = self._git_commit()

        # Load E2E checks
        e2e_path = ARTIFACTS / "e2e_report.json"
        checks = []
        if e2e_path.exists():
            data = json.loads(e2e_path.read_text())
            checks = data.get("checks", [])

        # Metrics summary
        metrics_summary = None
        if self.metrics:
            metrics_summary = self.metrics.summary()

        # Chaos results
        chaos_results = None
        if self.chaos:
            chaos_results = self.chaos.actions

        # Generate report
        gen = ReportGenerator()
        md_path = gen.generate(
            duration_sec=duration,
            mode="full" if self.args.full else ("headless-client" if self.args.headless_client else "server-only"),
            npcs=self.args.npcs,
            git_commit=git_commit,
            checks=checks,
            metrics_summary=metrics_summary,
            chaos_results=chaos_results,
        )

        # Also save simple JSON
        report = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime()),
            "duration_seconds": round(duration, 1),
            "mode": "full" if self.args.full else ("headless-client" if self.args.headless_client else "server-only"),
            "npcs": self.args.npcs,
            "git_commit": git_commit,
            "artifacts_dir": str(ARTIFACTS),
            "markdown_report": str(md_path),
        }
        path = ARTIFACTS / f"demo_report_{time.strftime('%Y%m%d_%H%M%S')}.json"
        path.write_text(json.dumps(report, indent=2))
        print(f"  Markdown report: {md_path}")
        print(f"  JSON report: {path}")
        print(f"\\n{BOLD}{GREEN}Demo completed successfully in {duration:.1f}s{RESET}")

    def teardown(self):
        print_header("Teardown")
        if self.chaos:
            self.chaos.network_recover()
            print_step("Network recovered", "OK")
        if self.godot:
            self.godot.stop(graceful=True)
            print_step("Godot stopped", "OK")
        if self.supervisor:
            self.supervisor.stop_all()
            print_step("Supervisor stopped", "OK")

    def _git_commit(self) -> str:
        try:
            import subprocess
            r = subprocess.run(["git", "-C", str(PROJECT_ROOT), "rev-parse", "--short", "HEAD"],
                               capture_output=True, text=True)
            return r.stdout.strip()
        except Exception:
            return "unknown"


def main():
    parser = argparse.ArgumentParser(description="DarkAges MMO Demo Runner")
    parser.add_argument("--smoke", action="store_true", help="Quick smoke test (server + validator)")
    parser.add_argument("--full", action="store_true", help="Full demo with headed Godot client")
    parser.add_argument("--headless-client", action="store_true", help="Launch Godot in headless mode")
    parser.add_argument("--server-only", action="store_true", help="Only start server + validator")
    parser.add_argument("--no-build", action="store_true", help="Skip build phase")
    parser.add_argument("--no-tests", action="store_true", help="Skip test phase")
    parser.add_argument("--clean-build", action="store_true", help="Clean build directory before building")
    parser.add_argument("--npcs", type=int, default=10, help="Number of NPCs to spawn")
    parser.add_argument("--duration", type=int, default=15, help="Demo duration in seconds")
    parser.add_argument("--demo-content", type=str, help="Path to demo zone JSON config")
    parser.add_argument("--chaos", action="store_true", help="Enable fault injection (CPU spike, network delay, process kills)")
    parser.add_argument("--record", action="store_true", help="Record video with ffmpeg (not yet implemented)")
    parser.add_argument("--dashboard", action="store_true", help="Show live terminal dashboard (not yet implemented)")
    args = parser.parse_args()

    if args.smoke:
        args.no_build = False
        args.no_tests = False
        args.server_only = True
        args.duration = 10
        args.npcs = 5

    pipeline = DemoPipeline(args)
    sys.exit(pipeline.run())


if __name__ == "__main__":
    main()
