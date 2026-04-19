#!/usr/bin/env python3
"""
DarkAges Phase 9 Performance Report
Runs benchmarks + load tests, produces structured report with budget checks.

Usage:
    python3 tools/perf/phase9_report.py              # Full report
    python3 tools/perf/phase9_report.py --quick       # Skip benchmarks
    python3 tools/perf/phase9_report.py --json        # JSON output
"""

import subprocess
import re
import json
import sys
import time
from pathlib import Path
from datetime import datetime, timezone

REPO = Path(__file__).resolve().parent.parent.parent
BUILD_DIR = REPO / "build_validate"
REPORT_DIR = REPO / "docs" / "performance" / "reports"


def run_ctest(tag_filter, timeout=300):
    """Run tests matching tag filter, return (output, exit_code)."""
    cmd = [
        str(BUILD_DIR / "darkages_tests"),
        tag_filter,
        "--success",  # Show successful tests too
    ]
    try:
        r = subprocess.run(cmd, cwd=str(BUILD_DIR), capture_output=True, text=True, timeout=timeout)
        return r.stdout + "\n" + r.stderr, r.returncode
    except subprocess.TimeoutExpired:
        return "TIMEOUT", -1


def run_benchmarks():
    """Run Catch2 benchmarks and parse results."""
    output, code = run_ctest("[.benchmark]", timeout=120)
    results = []

    lines = output.split("\n")
    current_name = None
    for i, line in enumerate(lines):
        # Catch2 v3: benchmark name line followed by "mean" line
        # Look for lines that have "us" or "ms" in the mean position
        m = re.search(r'^(\w+)\s+\d+\s+\d+', line)
        if m:
            current_name = m.group(1)
            # Next line has: mean X.XX unit ...
            if i + 1 < len(lines):
                next_line = lines[i + 1]
                mean_match = re.search(r'([\d.]+)\s*(ns|us|ms|s)\s*$', next_line.strip())
                if mean_match and current_name:
                    results.append({
                        "name": current_name,
                        "mean": float(mean_match.group(1)),
                        "unit": mean_match.group(2),
                        "stddev": 0.0,
                    })
                    current_name = None

    return results


def run_load_tests():
    """Run load test suite and parse results."""
    output, code = run_ctest("[load]", timeout=120)
    results = []

    for line in output.split("\n"):
        # Our INFO output: "Tick time: X.XXms for N entities"
        m = re.search(r'Tick time:\s+([\d.]+)ms\s+for\s+(\d+)\s+entit', line)
        if m:
            results.append({
                "tick_ms": float(m.group(1)),
                "entities": int(m.group(2)),
            })

    # Also check for test pass/fail
    passed = "Failed" not in output and code == 0

    return results, passed


def check_budgets(bench_results, load_results):
    """Check all results against performance budgets."""
    checks = []

    # Benchmark budgets
    benchmark_budgets = {
        "spatial_insert_1000": (5.0, "ms"),
        "movement_500": (16.0, "ms"),
        "combat_damage_100": (5.0, "ms"),
        "entt_iterate_1000": (1.0, "ms"),
    }

    for r in bench_results:
        budget = benchmark_budgets.get(r["name"])
        if budget:
            mean_ms = r["mean"]
            if r["unit"] == "us":
                mean_ms /= 1000
            elif r["unit"] == "ns":
                mean_ms /= 1_000_000
            elif r["unit"] == "s":
                mean_ms *= 1000

            passed = mean_ms <= budget[0]
            checks.append({
                "name": r["name"],
                "type": "benchmark",
                "measured": round(mean_ms, 3),
                "budget": budget[0],
                "unit": "ms",
                "passed": passed,
            })

    # Load test budgets
    load_budgets = {
        50: (16.0, "light"),
        100: (16.0, "medium"),
        200: (16.0, "medium"),
        400: (20.0, "heavy"),
        800: (50.0, "extreme"),
    }

    for r in load_results:
        budget = load_budgets.get(r["entities"])
        if budget:
            passed = r["tick_ms"] <= budget[0]
            checks.append({
                "name": f"{r['entities']}_entities_tick",
                "type": "load_test",
                "measured": round(r["tick_ms"], 3),
                "budget": budget[0],
                "unit": "ms",
                "category": budget[1],
                "passed": passed,
            })

    return checks


def generate_report(bench_results, load_results, load_passed, checks, as_json=False):
    """Generate performance report."""
    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")

    total_checks = len(checks)
    passed_checks = sum(1 for c in checks if c["passed"])

    report = {
        "timestamp": now,
        "summary": {
            "total_checks": total_checks,
            "passed": passed_checks,
            "failed": total_checks - passed_checks,
            "load_tests_passed": load_passed,
        },
        "benchmarks": bench_results,
        "load_tests": load_results,
        "budget_checks": checks,
    }

    if as_json:
        print(json.dumps(report, indent=2))
        return report

    # Text report
    print("=" * 70)
    print(f"DARKAGES PHASE 9 PERFORMANCE REPORT — {now}")
    print("=" * 70)

    print(f"\nSummary: {passed_checks}/{total_checks} budget checks passed")
    if load_passed:
        print("Load tests: ALL PASSED")
    else:
        print("Load tests: FAILURES DETECTED")

    if bench_results:
        print(f"\n{'BENCHMARKS':=^70}")
        print(f"{'Name':<40} {'Mean':>12} {'Std Dev':>12}")
        print("-" * 65)
        for r in bench_results:
            print(f"{r['name']:<40} {r['mean']:>8.2f} {r['unit']:<3} {r['stddev']:>8.2f} {r['unit']}")

    if load_results:
        print(f"\n{'LOAD TESTS':=^70}")
        print(f"{'Entities':<15} {'Tick Time':>15}")
        print("-" * 35)
        for r in load_results:
            print(f"{r['entities']:<15} {r['tick_ms']:>10.2f} ms")

    if checks:
        print(f"\n{'BUDGET CHECKS':=^70}")
        for c in checks:
            status = "PASS" if c["passed"] else "FAIL"
            print(f"  [{status}] {c['name']}: {c['measured']}ms vs {c['budget']}ms budget")

    print("\n" + "=" * 70)

    return report


def save_report(report):
    """Save report to docs/performance/reports/."""
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    date_str = datetime.now().strftime("%Y-%m-%d")
    report_path = REPORT_DIR / f"phase9_report_{date_str}.json"
    report_path.write_text(json.dumps(report, indent=2))
    print(f"\nReport saved to: {report_path.relative_to(REPO)}")
    return report_path


def main():
    as_json = "--json" in sys.argv
    quick = "--quick" in sys.argv

    if not as_json:
        print("Phase 9 Performance Report")
        print("=" * 40)

    bench_results = []
    if not quick:
        if not as_json:
            print("\n[1/3] Running benchmarks...")
        bench_results = run_benchmarks()
        if not as_json:
            print(f"  Captured {len(bench_results)} benchmark results")

    if not as_json:
        print(f"\n[2/3] Running load tests...")
    load_results, load_passed = run_load_tests()
    if not as_json:
        print(f"  Captured {len(load_results)} data points, tests {'PASSED' if load_passed else 'FAILED'}")

    if not as_json:
        print(f"\n[3/3] Checking budgets...")
    checks = check_budgets(bench_results, load_results)

    report = generate_report(bench_results, load_results, load_passed, checks, as_json)

    if not as_json:
        save_report(report)

    # Exit code based on results
    all_passed = load_passed and all(c["passed"] for c in checks)
    sys.exit(0 if all_passed else 1)


if __name__ == "__main__":
    main()
