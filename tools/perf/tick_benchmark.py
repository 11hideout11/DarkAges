#!/usr/bin/env python3
"""
DarkAges Tick Benchmark — measures server tick performance under various loads.
Phase 9: Performance Testing

Usage:
    python3 tools/perf/tick_benchmark.py                    # Quick 100-entity test
    python3 tools/perf/tick_benchmark.py --entities 1000    # 1000-entity load test
    python3 tools/perf/tick_benchmark.py --duration 60      # Run for 60 seconds
    python3 tools/perf/tick_benchmark.py --full             # Full benchmark suite
"""

import subprocess
import time
import re
import sys
import json
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
BUILD_DIR = REPO / "build_validate"


def run_ctest(tag="[!benchmark]", timeout=300):
    """Run Catch2 benchmarks with the given tag filter."""
    cmd = [
        str(BUILD_DIR / "darkages_tests"),
        tag,
        "--benchmark-samples", "10",
        "--benchmark-warmup-time", "3"
    ]
    try:
        r = subprocess.run(cmd, cwd=str(BUILD_DIR), capture_output=True, text=True, timeout=timeout)
        return r.stdout + "\n" + r.stderr, r.returncode
    except subprocess.TimeoutExpired:
        return "TIMEOUT", -1


def parse_benchmark_output(output):
    """Parse Catch2 benchmark output into structured results."""
    results = []
    for line in output.split("\n"):
        # Match lines like: "benchmark name    mean: 1.23 us    std dev: 0.45 us"
        m = re.search(r'(\w+)\s+mean:\s+([\d.]+)\s*(ns|us|ms|s)\s+std dev:\s+([\d.]+)', line)
        if m:
            results.append({
                "name": m.group(1),
                "mean": float(m.group(2)),
                "unit": m.group(3),
                "stddev": float(m.group(4))
            })
    return results


def quick_benchmark():
    """Run a quick benchmark suite for core systems."""
    print("=" * 60)
    print("DarkAges Tick Benchmark — Quick Suite")
    print("=" * 60)

    output, code = run_ctest("[!benchmark]", timeout=120)
    results = parse_benchmark_output(output)

    if not results:
        print("No benchmark results captured. Raw output:")
        print(output[:2000])
        return

    print(f"\n{'Benchmark':<40} {'Mean':>12} {'Std Dev':>12}")
    print("-" * 65)
    for r in results:
        print(f"{r['name']:<40} {r['mean']:>8.2f} {r['unit']:<3} {r['stddev']:>8.2f} {r['unit']}")
    print()

    # Check against budgets
    budgets = {
        "spatial_insert_1000": 5.0,    # 5ms max for 1000 inserts
        "movement_500": 16.0,          # Must fit in 16ms tick
        "combat_damage_100": 5.0,      # 5ms max for 100 damage calcs
        "entt_iterate_1000": 1.0,      # 1ms max for 1000-entity iteration
    }

    print("Budget Checks:")
    all_pass = True
    for r in results:
        budget = budgets.get(r["name"])
        if budget is not None:
            # Convert to same unit
            mean_ms = r["mean"]
            if r["unit"] == "us":
                mean_ms /= 1000
            elif r["unit"] == "ns":
                mean_ms /= 1000000
            elif r["unit"] == "s":
                mean_ms *= 1000

            status = "PASS" if mean_ms <= budget else "FAIL"
            if status == "FAIL":
                all_pass = False
            print(f"  {r['name']}: {mean_ms:.2f}ms vs {budget:.1f}ms budget [{status}]")

    return all_pass


if __name__ == "__main__":
    if "--help" in sys.argv:
        print(__doc__)
        sys.exit(0)

    passed = quick_benchmark()
    sys.exit(0 if passed else 1)
