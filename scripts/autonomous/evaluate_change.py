#!/usr/bin/env python3
"""
DarkAges Evaluator Agent — standalone QA harness.

Principles (from harness engineering research):
1. NEVER generates or modifies code — evaluation only.
2. NEVER grades its own work — this is a separate process from generation.
3. Actually RUNS the build and tests — not a code-reading review.
4. Returns structured JSON for programmatic gating.

Usage:
    python3 evaluate_change.py <branch_name> [--base main]
    python3 evaluate_change.py --current  # evaluate current working tree

Exit codes:
    0 = PASS (build + tests OK, no regression)
    1 = FAIL (build or test failure, or test count regression)
    2 = ERROR (evaluator internal error)
"""
import subprocess, json, os, sys, re, shutil, time
from datetime import datetime, timezone
from pathlib import Path
from typing import Tuple, Optional

REPO = Path("/root/projects/DarkAges")
BUILD_DIR = REPO / "build_validate"
ORIGINAL_BRANCH: Optional[str] = None

CMAKE_CMD = [
    "cmake", "-S", str(REPO), "-B", str(BUILD_DIR),
    "-DBUILD_TESTS=ON", "-DFETCH_DEPENDENCIES=ON",
    "-DENABLE_GNS=OFF", "-DENABLE_REDIS=OFF", "-DENABLE_SCYLLA=OFF"
]
BUILD_CMD = ["cmake", "--build", str(BUILD_DIR), "-j", str(os.cpu_count() or 4)]
TEST_CMD = ["ctest", "--output-on-failure", "-j8"]


def run(cmd, cwd=None, timeout=300, capture=True) -> Tuple[int, str, str]:
    try:
        r = subprocess.run(cmd, cwd=cwd, timeout=timeout,
                           capture_output=capture, text=True)
        return r.returncode, r.stdout or "", r.stderr or ""
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except Exception as e:
        return -1, "", str(e)


def git(*args, **kwargs):
    kwargs.setdefault("timeout", 30)
    return run(["git", "-C", str(REPO)] + list(args), **kwargs)


def get_current_branch() -> str:
    code, out, _ = git("rev-parse", "--abbrev-ref", "HEAD")
    return out.strip() if code == 0 else "unknown"


def get_baseline_test_counts(ref: str) -> Tuple[int, int]:
    """Get test counts from base ref without modifying working tree."""
    # Try reading AUTONOMOUS_LOG.md from the base ref via git show
    code, out, _ = run(
        ["git", "-C", str(REPO), "show", f"{ref}:AUTONOMOUS_LOG.md"],
        timeout=10
    )
    if code == 0 and out:
        matches = re.findall(r'(\d+) test cases?, (\d+) assertions?', out)
        if matches:
            return int(matches[-1][0]), int(matches[-1][1])

    # Fallback to current log (optimistic baseline)
    log_path = REPO / "AUTONOMOUS_LOG.md"
    if log_path.exists():
        content = log_path.read_text()
        matches = re.findall(r'(\d+) test cases?, (\d+) assertions?', content)
        if matches:
            return int(matches[-1][0]), int(matches[-1][1])
    return 0, 0


def parse_warnings(build_stdout: str, build_stderr: str) -> list:
    """Extract compiler warnings from build output."""
    combined = (build_stdout or "") + "\n" + (build_stderr or "")
    warnings = []
    for line in combined.split("\n"):
        # Match typical GCC/Clang/MSVC warning patterns
        if re.search(r':\s*warning\s*:', line, re.IGNORECASE):
            warnings.append(line.strip())
        elif re.search(r'warning\s*#', line, re.IGNORECASE):
            warnings.append(line.strip())
    return warnings


def build() -> Tuple[bool, str, float, list]:
    """Configure + build. Returns (ok, error_snippet, duration_sec, warnings)."""
    start = time.time()
    cache_file = BUILD_DIR / "CMakeCache.txt"
    makefile = BUILD_DIR / "Makefile"
    needs_configure = not cache_file.exists() or not makefile.exists()
    if needs_configure:
        code, out, err = run(CMAKE_CMD, timeout=120)
        if code != 0:
            return False, f"cmake configure failed: {err[-500:]}", time.time() - start, []
        if not (BUILD_DIR / "Makefile").exists():
            cache_file.unlink(missing_ok=True)
            code, out, err = run(CMAKE_CMD, timeout=120)
            if code != 0:
                return False, f"cmake reconfigure failed: {err[-500:]}", time.time() - start, []
    code, out, err = run(BUILD_CMD, timeout=600)
    duration = time.time() - start
    warnings = parse_warnings(out, err)
    if code != 0:
        lines = (err or out or "").strip().split("\n")
        return False, "\n".join(lines[-30:]), duration, warnings
    return True, "", duration, warnings


def test() -> Tuple[bool, str, str, float, str]:
    """Run ctest. Returns (ok, summary, error_snippet, duration_sec, full_output)."""
    start = time.time()
    # Single ctest run with both --output-on-failure and --verbose to capture
    # summary for pass/fail and verbose output for assertion counts.
    code, out, err = run(TEST_CMD + ["--verbose"], cwd=str(BUILD_DIR), timeout=600)
    duration = time.time() - start
    output = out + "\n" + err
    if code != 0:
        lines = output.strip().split("\n")
        return False, "", "\n".join(lines[-20:]), duration, output
    # Extract summary line — prioritize ctest summary, then Catch2 summary
    summary = "tests passed"
    for line in output.split("\n"):
        line_lower = line.lower()
        # ctest summary: "100% tests passed, 0 tests failed out of 11"
        if "% tests passed" in line_lower and "tests failed" in line_lower:
            summary = line.strip()
            break
        # Catch2 summary: "All tests passed (1234 assertions in 567 test cases)"
        if "all tests passed" in line_lower and "assertions" in line_lower:
            summary = line.strip()
            break
        # Fallback: "Total Test time"
        if line_lower.startswith("total test time"):
            summary = line.strip()
    return True, summary, "", duration, output


def get_test_counts(output: str) -> Tuple[int, int]:
    """Extract test case and assertion counts from ctest verbose output."""
    total_cases = 0
    total_asserts = 0
    for line in output.split("\n"):
        m = re.search(r"(\d+) assertions? in (\d+) test cases?", line)
        if m:
            total_asserts += int(m.group(1))
            total_cases += int(m.group(2))
    return total_cases, total_asserts


def evaluate(branch: Optional[str] = None, base_ref: str = "main") -> dict:
    """
    Run full evaluation on a branch or current working tree.
    Returns structured report dict.
    """
    global ORIGINAL_BRANCH
    result = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "branch": branch or get_current_branch(),
        "base_ref": base_ref,
        "build": {"pass": False, "error": "", "duration_sec": 0.0, "warnings": []},
        "tests": {"pass": False, "summary": "", "error": "", "duration_sec": 0.0},
        "counts": {"cases": 0, "assertions": 0},
        "baseline": {"cases": 0, "assertions": 0},
        "regression": False,
        "overall": "PENDING",
    }

    # Save original branch if we need to switch
    if branch:
        ORIGINAL_BRANCH = get_current_branch()
        # Only stash if there are local changes
        code, _, _ = git("diff", "--quiet")
        has_local_changes = code != 0
        if has_local_changes:
            git("stash", "push", "-m", "evaluator-auto-stash")
        code, _, err = git("checkout", branch)
        if code != 0:
            result["overall"] = "ERROR"
            result["build"]["error"] = f"Failed to checkout branch {branch}: {err}"
            return result

    try:
        # Build
        build_ok, build_err, build_dur, warnings = build()
        result["build"]["pass"] = build_ok
        result["build"]["error"] = build_err
        result["build"]["duration_sec"] = round(build_dur, 2)
        result["build"]["warnings"] = warnings[:20]  # cap to avoid JSON bloat
        if not build_ok:
            result["overall"] = "FAIL"
            return result

        # Tests
        test_ok, test_summary, test_err, test_dur, test_output = test()
        result["tests"]["pass"] = test_ok
        result["tests"]["summary"] = test_summary
        result["tests"]["error"] = test_err
        result["tests"]["duration_sec"] = round(test_dur, 2)
        if not test_ok:
            result["overall"] = "FAIL"
            return result

        # Counts (parsed from the same test output to avoid running ctest twice)
        cases, asserts = get_test_counts(test_output)
        result["counts"]["cases"] = cases
        result["counts"]["assertions"] = asserts

        # Baseline comparison
        base_cases, base_asserts = get_baseline_test_counts(base_ref)
        result["baseline"]["cases"] = base_cases
        result["baseline"]["assertions"] = base_asserts

        # Regression gate: if we had a baseline and counts dropped, flag it
        if base_cases > 0 and cases < base_cases:
            result["regression"] = True
            result["overall"] = "FAIL"
            result["tests"]["error"] = (
                f"TEST REGRESSION: cases dropped from {base_cases} to {cases}"
            )
            return result

        if base_asserts > 0 and asserts < base_asserts:
            result["regression"] = True
            result["overall"] = "FAIL"
            result["tests"]["error"] = (
                f"TEST REGRESSION: assertions dropped from {base_asserts} to {asserts}"
            )
            return result

        # Skeptical gate: if tests don't explicitly claim 100% pass, scrutinize
        # Accept ctest "100%" / "0 tests failed" or Catch2 "All tests passed"
        summary_lower = test_summary.lower()
        is_explicit_pass = (
            "100%" in test_summary
            or "0 tests failed" in test_summary
            or "all tests passed" in summary_lower
        )
        if test_summary and not is_explicit_pass:
            result["tests"]["error"] = f"TEST SUMMARY AMBIGUOUS: {test_summary}"
            result["overall"] = "FAIL"
            return result

        result["overall"] = "PASS"
        return result

    finally:
        # Restore original branch if we switched
        if branch and ORIGINAL_BRANCH:
            git("checkout", ORIGINAL_BRANCH)
            if has_local_changes:
                git("stash", "pop")


def main():
    if len(sys.argv) < 2:
        print("Usage: evaluate_change.py <branch_name> [--base main]", file=sys.stderr)
        print("       evaluate_change.py --current [--base main]", file=sys.stderr)
        sys.exit(2)

    branch = sys.argv[1] if sys.argv[1] != "--current" else None
    base_ref = "main"
    if "--base" in sys.argv:
        idx = sys.argv.index("--base")
        if idx + 1 < len(sys.argv):
            base_ref = sys.argv[idx + 1]

    report = evaluate(branch, base_ref)
    print(json.dumps(report, indent=2))

    if report["overall"] == "PASS":
        sys.exit(0)
    elif report["overall"] == "FAIL":
        sys.exit(1)
    else:
        sys.exit(2)


if __name__ == "__main__":
    main()
