#!/usr/bin/env python3
"""Testing pipeline skill — derived from OpenHands testing.md
Selects affected tests and detects flaky tests."""

import subprocess, sys, argparse, os, json

def get_changed_files(base="main"):
    r = subprocess.run(
        ["git", "diff", "--name-only", f"origin/{base}..HEAD"],
        capture_output=True, text=True
    )
    return [f for f in r.stdout.strip().split("\n") if f]

def find_tests(root="."):
    tests = []
    for dirpath, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d not in [".git", "build", "__pycache__", ".hermes"]]
        for fn in files:
            if fn.endswith("_test.cpp") or fn.endswith(".Tests.cs"):
                tests.append(os.path.join(dirpath, fn))
    return tests

def guess_affected_tests(changed, tests):
    changed_bases = {os.path.splitext(os.path.basename(f))[0] for f in changed}
    affected = []
    for t in tests:
        base = os.path.splitext(os.path.basename(t))[0]
        core = base.replace("_test","").replace("Tests","")
        if core in changed_bases:
            affected.append(t)
    return affected

def run_ctest(test_regex, repeat=1):
    all_ok = True
    for i in range(repeat):
        r = subprocess.run(
            ["ctest", "-R", test_regex, "-j1", "--output-on-failure"],
            capture_output=True, text=True, cwd="build"
        )
        ok = r.returncode == 0
        all_ok = all_ok and ok
        if not ok and repeat == 1:
            print(r.stdout[-1000:])
    return {"passed": all_ok, "runs": repeat}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--changed", action="store_true", help="Run tests affected by recent changes")
    parser.add_argument("--bisect", help="Identify flakiness for specific test")
    parser.add_argument("--repeat", type=int, default=1, help="Number of runs for flakiness detection")
    parser.add_argument("--json", action="store_true", help="Output JSON")
    args = parser.parse_args()

    report = {}
    if args.changed:
        src = get_changed_files()
        tests = find_tests(".")
        affected_tests = guess_affected_tests(src, tests)
        if affected_tests:
            pattern = "|".join(os.path.splitext(os.path.basename(t))[0] for t in affected_tests)
            result = run_ctest(pattern, repeat=args.repeat)
            report = {"mode":"changed","changed_files":src,"tests":affected_tests,"result":result}
        else:
            report = {"mode":"changed","tests":[]}
    elif args.bisect:
        result = run_ctest(args.bisect, repeat=max(10, args.repeat))
        report = {"mode":"bisect","test":args.bisect,"result":result}
    else:
        parser.print_help()
        sys.exit(1)

    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(json.dumps(report, indent=2))

if __name__ == "__main__":
    main()
