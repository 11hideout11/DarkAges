#!/usr/bin/env python3
"""
Coverage report skill — runs tests with coverage, produces HTML + summary.
Requires lcov installed (apt install lcov) or uses gcov directly.
"""

import subprocess, sys, argparse, os, re, json

def run(cmd, capture=True, check=False):
    r = subprocess.run(cmd, shell=True, capture_output=capture, text=True)
    if check and r.returncode != 0:
        print(f"ERROR: {r.stderr}", file=sys.stderr)
        sys.exit(r.returncode)
    return r

def main():
    parser = argparse.ArgumentParser(description="Generate test coverage report")
    parser.add_argument("--output-dir", default="coverage_html", help="HTML output directory")
    parser.add_argument("--json", action="store_true", help="Output JSON summary")
    parser.add_argument("--skip-html", action="store_true", help="Skip HTML generation")
    parser.add_argument("--rate", type=float, default=0.0, help="Minimum coverage threshold (0-100)")
    args = parser.parse_args()

    # Determine build dir
    build_dir = "build_validate" if os.path.isdir("build_validate") else "build"
    if not os.path.isdir(build_dir):
        print(f"ERROR: No build directory found", file=sys.stderr)
        sys.exit(1)

    # Clean previous coverage data
    run("rm -rf coverage.info coverage_html")

    # Run tests with lcov if available
    lcov_available = subprocess.run(["which", "lcov"], capture_output=True).returncode == 0

    if lcov_available:
        # Initialize
        run("lcov --initial --directory . --output-file coverage.info --rc lcov_branch_coverage=1", check=True)
        # Execute tests
        result = run(f"cd {build_dir} && ctest --output-on-failure -j1", check=False)
        if result.returncode != 0:
            print("WARNING: Some tests failed; coverage may be partial")
        # Capture
        run("lcov --capture --directory . --output-file coverage.info --rc lcov_branch_coverage=1", check=True)
        # Filter
        run("lcov --remove coverage.info '/usr/*' '*/deps/*' '*_test.cpp' '*/tests/*' --output-file coverage.info", check=False)
        # Generate HTML
        if not args.skip_html:
            os.makedirs(args.output_dir, exist_ok=True)
            run(f"lcov --html --directory . --output-file {args.output_dir}/index.html --title 'DarkAges Coverage'", check=True)
            print(f"HTML report: {args.output_dir}/index.html")
    else:
        # Fallback: gcov + gcovr
        gcovr_available = subprocess.run(["which", "gcovr"], capture_output=True).returncode == 0
        if not gcovr_available:
            print("ERROR: Install lcov or gcovr for coverage", file=sys.stderr)
            sys.exit(1)
        result = run(f"cd {build_dir} && ctest -j1", check=False)
        if result.returncode != 0:
            print("WARNING: tests had failures")
        # gcovr generates both HTML + summary
        run(f"gcovr --root . --html-details {args.output_dir}/coverage.html --html-title 'DarkAges Coverage'", check=True)
        print(f"HTML: {args.output_dir}/coverage.html")

    # Parse summary
    summary = {}
    # Try to find coverage percentage in recent output
    cov_match = None
    if lcov_available:
        # lcov --summary coverage.info
        r = run("lcov --summary coverage.info")
        for line in r.stdout.split('\n'):
            if 'lines' in line and 'version' not in line:
                # Example: "Lines executed:85.32% (1245 of 1458 lines)"
                m = re.search(r'(\d+\.\d+)%\s+\((\d+)\s+of\s+(\d+)', line)
                if m:
                    summary = {"lines_percent": float(m.group(1)), "lines_covered": int(m.group(2)), "lines_total": int(m.group(3))}
                    break
    else:
        # gcovr --json-summary
        r = run("gcovr --json-summary coverage.json")
        if os.path.exists("coverage.json"):
            with open("coverage.json") as f:
                data = json.load(f)
            summary = {"lines_percent": data.get('line_percent',0), "lines_covered": data.get('lines_covered',0), "lines_total": data.get('lines_total',0)}

    if args.json:
        print(json.dumps(summary))
    else:
        if summary:
            print(f"Coverage: {summary['lines_percent']:.2f}%  ({summary['lines_covered']}/{summary['lines_total']} lines)")
            if args.rate > 0 and summary['lines_percent'] < args.rate:
                print(f"FAIL: below threshold {args.rate}%", file=sys.stderr)
                sys.exit(1)
        else:
            print("Coverage summary not available")

if __name__ == "__main__":
    main()
