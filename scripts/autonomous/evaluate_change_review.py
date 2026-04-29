#!/usr/bin/env python3
"""
DarkAges Evaluator Review Agent — skeptical code review via OpenCode CLI.

This is the "subjective" evaluator layer in the GAN-style architecture.
It does NOT build or test — that is handled by evaluate_change.py (objective).
Instead, it reads the diff, ingests AGENTS.md, and acts as a skeptical reviewer.

Principles:
1. NEVER generates or modifies code.
2. Focuses on: bug patterns, AGENTS.md rule violations, test quality, regressions.
3. Returns structured JSON for programmatic gating.

Usage:
    python3 evaluate_change_review.py <branch_name> [--base main]
    python3 evaluate_change_review.py --current [--base main]

Exit codes:
    0 = PASS (review OK, no critical issues)
    1 = FAIL (critical issues found)
    2 = ERROR (review infrastructure failure)
"""
import subprocess, json, os, sys, re, time
from datetime import datetime, timezone
from pathlib import Path
from typing import Tuple, Optional

REPO = Path("/root/projects/DarkAges")
AGENTS_MD = REPO / "AGENTS.md"


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


def get_diff(branch: Optional[str] = None, base_ref: str = "main") -> str:
    """Get the diff between branch and base_ref, excluding runtime state files."""
    if branch:
        code, out, err = git("diff", f"{base_ref}...{branch}")
    else:
        # --current: review staged changes (index) vs base
        code, out, err = git("diff", "--cached", base_ref)
    if code != 0:
        return f"# Error getting diff: {err}"
    # Filter out runtime/state files that are not code changes
    filtered_lines = []
    skip_file = False
    runtime_patterns = [
        ".task_cache.json",
        ".cron_state.json",
        ".cron.lock",
        ".edit_history.json",
        "AUTONOMOUS_LOG.md",
        "TASK_QUEUE.md",
    ]
    for line in out.split("\n"):
        if line.startswith("diff --git "):
            skip_file = any(p in line for p in runtime_patterns)
        if not skip_file:
            filtered_lines.append(line)
    return "\n".join(filtered_lines)


def get_agents_md() -> str:
    """Read AGENTS.md if present."""
    if AGENTS_MD.exists():
        try:
            return AGENTS_MD.read_text()
        except Exception:
            pass
    return "# AGENTS.md not found"


def run_opencode_review(diff_text: str, agents_text: str) -> dict:
    """
    Delegate skeptical review to OpenCode CLI agent.
    The agent is given the diff and AGENTS.md and asked to critique.
    """
    # Check if opencode is available
    code, _, _ = run(["which", "opencode"], timeout=5)
    if code != 0:
        return {
            "overall": "UNAVAILABLE",
            "reason": "opencode CLI not found in PATH",
        }

    # Build a focused prompt for OpenCode
    prompt = f"""You are an independent, skeptical QA reviewer. You do NOT write code. You ONLY critique.

PROJECT CONTEXT (from AGENTS.md):
```
{agents_text[:4000]}
```

CODE DIFF TO REVIEW:
```diff
{diff_text[:8000]}
```

INSTRUCTIONS:
1. Look for bugs, logic errors, missing error handling, and violations of the project rules in AGENTS.md.
2. Check if tests were added/modified appropriately for the changes.
3. Flag any EnTT API misuse, namespace violations, or forward-decl sizeof issues.
4. Be SKEPTICAL — if something looks questionable, flag it.
5. Do NOT approve just because the diff looks clean. Default to cautious.

Respond with ONLY a JSON object in this exact format (no markdown, no prose):
{{
  "overall": "PASS" or "FAIL",
  "issues": [
    {{
      "severity": "critical" or "warning" or "info",
      "file": "filename",
      "line": "line number or range",
      "message": "detailed description of the issue"
    }}
  ],
  "notes": "any additional skeptical observations"
}}

If there are no issues, return an empty issues array and overall: "PASS".
"""

    # Run OpenCode in non-interactive mode with inline prompt
    code, out, err = run(
        ["opencode", "run", prompt],
        cwd=str(REPO),
        timeout=60,
    )

    # Try to extract JSON from OpenCode's output
    json_match = re.search(r'\{.*\}', out, re.DOTALL)
    if json_match:
        try:
            return json.loads(json_match.group())
        except json.JSONDecodeError:
            pass

    # Fallback: try to parse from the entire output
    try:
        return json.loads(out)
    except json.JSONDecodeError:
        # CRITICAL FALLBACK: if OpenCode fails, do lightweight heuristic scan
        return heuristic_review(diff_text, agents_text)


def heuristic_review(diff_text: str, agents_text: str) -> dict:
    """
    Lightweight pattern-based code review as fallback when OpenCode is unavailable
    or returns non-JSON output. Scans for known critical violation patterns.
    """
    issues = []
    lines = diff_text.splitlines()
    in_hunk = False
    current_file = None

    # Known violation patterns
    forbidden_namespace = re.compile(r'\bnamespace\s+darkages\b')  # case-sensitive: DarkAges (correct)
    entitle_has = re.compile(r'registry\.has<')
    entitle_view_size = re.compile(r'\.view\.size\s*\(')
    forward_decl_sizeof = re.compile(r'sizeof\s*\(\s*\w+\s*\)')
    nested_type_qual = re.compile(r'\w+::\w+\s*{')
    missing_include = re.compile(r'^[+-]#include\s+"')

    for i, line in enumerate(lines, 1):
        if line.startswith("+++ b/") or line.startswith("--- a/"):
            current_file = line.split("/")[-1]
            in_hunk = False
        elif line.startswith("@@ "):
            in_hunk = True
        elif not in_hunk:
            continue
        elif line.startswith("-") and not line.startswith("---"):
            pass  # We only inspect added lines for new violations
        elif line.startswith("+") and not line.startswith("+++"):
            code_line = line[1:]  # strip '+'

            # Determine if this is a C++ source file (allocation mutations only)
            is_cpp = current_file and any(
                current_file.endswith(ext) for ext in (".cpp", ".hpp", ".h", ".cc", ".cxx")
            )

            # Check namespace (C++ files only)
            if is_cpp and forbidden_namespace.search(code_line):
                issues.append({
                    "severity": "critical",
                    "file": current_file or "unknown",
                    "line": i,
                    "message": f"Namespace violation: found 'namespace darkages' (must be DarkAges::)"
                })
            # EnTT API misuse (C++ files only)
            if is_cpp and entitle_has.search(code_line):
                issues.append({
                    "severity": "critical",
                    "file": current_file or "unknown",
                    "line": i,
                    "message": "EnTT API misuse: registry.has<T>() forbidden; use registry.all_of<T>()"
                })
            if is_cpp and entitle_view_size.search(code_line):
                issues.append({
                    "severity": "critical",
                    "file": current_file or "unknown",
                    "line": i,
                    "message": "EnTT API misuse: view.size() forbidden; iterate or use registry.size() on entity storage"
                })
            # Forward-decl sizeof (C++ files only)
            # Ignore sizeof on known complete fundamental types to avoid false positives
            if is_cpp and forward_decl_sizeof.search(code_line):
                # Extract the type name inside sizeof(...)
                m = re.search(r'sizeof\s*\(\s*(\w+)\s*\)', code_line)
                if m:
                    typename = m.group(1)
                    # Whitelist of types that are always complete
                    complete_types = {
                        'float', 'double', 'int', 'unsigned', 'char', 'bool',
                        'uint32_t', 'int32_t', 'uint16_t', 'int16_t',
                        'uint8_t', 'int8_t', 'size_t', 'void', 'long', 'short',
                        'long long', 'unsigned long', 'unsigned char',
                    }
                    if typename in complete_types:
                        # Skip false positive
                        pass
                    else:
                        issues.append({
                            "severity": "critical",
                            "file": current_file or "unknown",
                            "line": i,
                            "message": f"sizeof() applied to forward-declared type '{typename}' — requires complete definition"
                        })
                else:
                    # Could not extract type; flag conservatively
                    issues.append({
                        "severity": "critical",
                        "file": current_file or "unknown",
                        "line": i,
                        "message": "sizeof() applied to forward-declared type — requires complete definition"
                    })
            # Nested type qualification (C++ files only)
            if is_cpp and nested_type_qual.search(code_line) and "::" in code_line:
                issues.append({
                    "severity": "warning",
                    "file": current_file or "unknown",
                    "line": i,
                    "message": "Nested type usage — ensure full qualification (e.g., DarkAges::RedisInternal::PendingCallback)"
                })

    # Determine overall
    critical_count = sum(1 for iss in issues if iss["severity"] == "critical")
    overall = "FAIL" if critical_count > 0 else "PASS"

    return {
        "overall": overall,
        "issues": issues,
        "notes": f"Heuristic review fallback: {len(issues)} issues flagged ({critical_count} critical). OpenCode unavailable or failed to parse."
    }


def evaluate_review(branch: Optional[str] = None, base_ref: str = "main") -> dict:
    """Run the subjective review evaluation. Returns structured report."""
    result = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "branch": branch or get_current_branch(),
        "base_ref": base_ref,
        "diff_lines": 0,
        "review": {"overall": "PENDING", "issues": [], "notes": ""},
    }

    diff_text = get_diff(branch, base_ref)
    result["diff_lines"] = len(diff_text.splitlines())

    if not diff_text.strip():
        result["review"]["overall"] = "PASS"
        result["review"]["notes"] = "No changes to review"
        return result

    agents_text = get_agents_md()
    review = run_opencode_review(diff_text, agents_text)

    # Normalize review output
    if "overall" in review:
        result["review"]["overall"] = review["overall"]
    if "issues" in review:
        result["review"]["issues"] = review["issues"]
    if "notes" in review:
        result["review"]["notes"] = review["notes"]
    if "reason" in review:
        result["review"]["notes"] = review["reason"]
        result["review"]["overall"] = review.get("overall", "ERROR")

    # Critical issues auto-FAIL
    critical_count = sum(1 for i in result["review"]["issues"] if i.get("severity") == "critical")
    if critical_count > 0 and result["review"]["overall"] == "PASS":
        result["review"]["overall"] = "FAIL"
        result["review"]["notes"] = f"Auto-FAIL: {critical_count} critical issue(s) found. " + result["review"]["notes"]

    return result


def main():
    if len(sys.argv) < 2:
        print("Usage: evaluate_change_review.py <branch_name> [--base main]", file=sys.stderr)
        print("       evaluate_change_review.py --current [--base main]", file=sys.stderr)
        sys.exit(2)

    branch = sys.argv[1] if sys.argv[1] != "--current" else None
    base_ref = "main"
    if "--base" in sys.argv:
        idx = sys.argv.index("--base")
        if idx + 1 < len(sys.argv):
            base_ref = sys.argv[idx + 1]

    report = evaluate_review(branch, base_ref)
    print(json.dumps(report, indent=2))

    if report["review"]["overall"] == "PASS":
        sys.exit(0)
    elif report["review"]["overall"] == "FAIL":
        sys.exit(1)
    else:
        sys.exit(2)


if __name__ == "__main__":
    main()
