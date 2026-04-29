#!/usr/bin/env python3
"""
Test flakiness detector — repeated test execution to identify unstable tests.
Reports: test name, pass rate, failure patterns.
"""

import subprocess, sys, argparse, os, json, collections, re

def run_ctest(pattern, repeat=10):
    results = []
    for i in range(repeat):
        r = subprocess.run(
            ["ctest", "--test-dir", "build_validate", "--output-on-failure", "-j1", "-R", pattern],
            capture_output=True, text=True
        )
        results.append(r.returncode == 0)
    return results

def main():
    parser = argparse.ArgumentParser(description="Detect flaky tests via repeated runs")
    parser.add_argument("--test", help="Specific test regex to check (default: all)")
    parser.add_argument("--repeat", type=int, default=10, help="Number of runs")
    parser.add_argument("--threshold", type=float, default=0.9, help="Pass rate threshold (0-1)")
    parser.add_argument("--json", action="store_true", help="JSON output")
    args = parser.parse_args()

    build_dir = "build_validate" if os.path.isdir("build_validate") else "build"
    if not os.path.isdir(build_dir):
        print("ERROR: build directory not found", file=sys.stderr)
        sys.exit(1)

    # Determine which tests to run
    if args.test:
        patterns = [args.test]
    else:
        # Discover all tests from CTest
        r = subprocess.run(["ctest", "--test-dir", build_dir, "-N"], capture_output=True, text=True)
        tests = []
        for line in r.stdout.split('\n'):
            m = re.match(r'\s*\d+:\s*(.*)', line)
            if m:
                tests.append(m.group(1))
        patterns = list(set(t[:30] for t in tests if t))  # rough pattern

    results_summary = {}
    flaky = []

    for pattern in patterns:
        runs = run_ctest(pattern, args.repeat)
        passes = sum(runs)
        rate = passes / args.repeat
        results_summary[pattern] = {"runs": args.repeat, "passed": passes, "rate": rate}
        if rate < args.threshold:
            flaky.append({"test": pattern, "rate": rate})

    if args.json:
        print(json.dumps({"threshold": args.threshold, "results": results_summary, "flaky": flaky}))
    else:
        print(f"Flakiness scan (threshold={args.threshold:.0%}, repeat={args.repeat})")
        for t, info in sorted(results_summary.items()):
            status = "FLaky" if info['rate'] < args.threshold else "OK"
            print(f"  {t[:50]:50s}  {info['rate']*100:5.1f}%  {status}")
        if flaky:
            print(f"\nFlaky tests detected: {len(flaky)}")
            sys.exit(1)
        else:
            print("\nAll tests stable.")

if __name__ == "__main__":
    main()
