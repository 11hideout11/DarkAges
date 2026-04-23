#!/usr/bin/env python3
"""
DarkAges Task Discovery Script
Scans the codebase for actionable improvement opportunities.
Used by cron jobs to find tasks for autonomous iteration.
"""
import subprocess
import os
import re
import json
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
SRC_DIR = PROJECT_ROOT / "src"
CACHE_FILE = Path(__file__).resolve().parent / ".task_cache.json"
CACHE_TTL_SECONDS = 1800  # 30 minutes


@dataclass
class Task:
    priority: str  # P0, P1, P2, P3
    category: str  # test, refactor, fix, feature
    title: str
    description: str
    files: List[str]
    estimated_hours: float


def get_git_head() -> str:
    """Get current git HEAD hash for cache invalidation."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            capture_output=True, text=True, timeout=5,
            cwd=str(PROJECT_ROOT)
        )
        return result.stdout.strip()
    except Exception:
        return "unknown"


def load_cache() -> dict:
    """Load cached task discovery results if fresh and git state matches."""
    if CACHE_FILE.exists():
        try:
            import time
            data = json.loads(CACHE_FILE.read_text())
            # Invalidate if git HEAD changed (tasks may have been completed)
            if data.get("git_head") != get_git_head():
                return {}
            if time.time() - data.get("timestamp", 0) < CACHE_TTL_SECONDS:
                return data
        except (json.JSONDecodeError, KeyError):
            pass
    return {}


def save_cache(tasks: List[dict]):
    """Save task discovery results to cache."""
    import time
    CACHE_FILE.write_text(json.dumps({
        "timestamp": time.time(),
        "git_head": get_git_head(),
        "tasks": tasks
    }, indent=2))


def find_missing_tests() -> List[Task]:
    """Find source files without corresponding test files."""
    tasks = []

    # Get all .cpp source files (excluding tests, stubs, main)
    src_files = []
    for ext in ["*.cpp"]:
        for f in SRC_DIR.rglob(ext):
            if "tests" in str(f) or "stub" in f.name.lower():
                continue
            if f.name.startswith("Test"):
                continue
            if f.name == "main.cpp":
                continue
            src_files.append(f)

    # Get all test file stems (lowercased for fuzzy matching)
    test_stems_lower = set()
    for f in SRC_DIR.rglob("Test*.cpp"):
        # "TestRedisIntegration" -> "redisintegration"
        test_stems_lower.add(f.stem.lower().replace("test", ""))

    # Source basenames to skip (covered by other test files)
    covered_by_other_tests = {
        "redismanager",  # covered by TestRedisIntegration
        "scyllamanager",  # covered by TestScyllaManager
        "redismanager_stub",  # stub, no test needed
        "scyllamanager_stub",  # stub, no test needed
        "main",  # no test needed
    }

    # Stub files that don't need tests
    stub_suffixes = {"_stub", "_stubs"}

    for src_file in src_files:
        base_lower = src_file.stem.lower()

        # Skip if covered by a different test file
        if base_lower in covered_by_other_tests:
            continue

        # Skip stub files
        if any(base_lower.endswith(s) for s in stub_suffixes):
            continue

        # Check for EXACT name match only (not fuzzy) for test stems
        # "anticheathandler" should NOT match "anticheat"
        has_exact_test = base_lower in test_stems_lower

        # Also check if the class name (without "Handler"/"Manager" suffix) has a test
        # e.g., "anticheathandler" -> check if "anticheat" has TestAntiCheat.cpp
        # But only if the test file is specifically about this component
        has_partial_test = False
        if not has_exact_test:
            # Strip common suffixes and check
            stripped = base_lower
            for suffix in ["handler", "manager", "logger", "optimizer", "validator"]:
                if stripped.endswith(suffix):
                    stripped = stripped[:-len(suffix)]
                    break
            if stripped in test_stems_lower and len(stripped) > 5:
                has_partial_test = True

        if not has_exact_test and not has_partial_test:
            # Determine priority based on file type
            if "stub" in base_lower:
                priority = "P3"
                desc = f"Stub file — add basic tests verifying stub behavior"
            elif any(k in str(src_file) for k in ["/db/", "/security/"]):
                priority = "P1"
                desc = f"Critical subsystem without tests — {src_file.relative_to(PROJECT_ROOT)}"
            else:
                priority = "P2"
                desc = f"No test file found for {src_file.relative_to(PROJECT_ROOT)}"

            tasks.append(Task(
                priority=priority,
                category="test",
                title=f"Add tests for {src_file.stem}",
                description=desc,
                files=[str(src_file.relative_to(PROJECT_ROOT))],
                estimated_hours=1.0
            ))

    return tasks


def find_missing_header_tests() -> List[Task]:
    """Find significant header files without corresponding test files."""
    tasks = []
    test_dir = SRC_DIR / "server" / "tests"

    # Get all test file stems
    test_stems = set()
    if test_dir.exists():
        for f in test_dir.glob("Test*.cpp"):
            test_stems.add(f.stem.lower().replace("test", ""))

    for h in SRC_DIR.rglob("*.hpp"):
        if "tests" in str(h) or "proto" in str(h):
            continue

        try:
            lines = len(h.read_text().splitlines())
        except (UnicodeDecodeError, PermissionError):
            continue

        # Only significant headers (50+ lines)
        if lines < 50:
            continue

        basename = h.stem.lower()

        # Check if there's a corresponding source file with tests
        has_src_test = False
        for suffix in ["handler", "manager", "logger", "optimizer", "validator"]:
            if basename.endswith(suffix):
                stripped = basename[:-len(suffix)]
                if stripped in test_stems:
                    has_src_test = True
                    break

        if has_src_test:
            continue

        # Check if any test covers this header's component
        has_test = basename in test_stems
        if not has_test:
            # Prioritize memory/safety headers
            if "/memory/" in str(h):
                priority = "P1"
            elif "/security/" in str(h):
                priority = "P1"
            else:
                priority = "P2"

            tasks.append(Task(
                priority=priority,
                category="test",
                title=f"Add tests for {h.stem}",
                description=f"Header has {lines} lines but no corresponding test file",
                files=[str(h.relative_to(PROJECT_ROOT))],
                estimated_hours=1.5
            ))

    return tasks


def find_large_files() -> List[Task]:
    """Find large files that are refactoring candidates."""
    tasks = []

    for ext in ["*.cpp", "*.hpp"]:
        for f in SRC_DIR.rglob(ext):
            if "tests" in str(f) or "proto" in str(f):
                continue
            try:
                content = f.read_text()
                lines = content.splitlines()

                # Only flag if there's actual cleanup potential:
                # 1. Has trailing whitespace
                has_trailing_ws = any(line != line.rstrip() for line in lines)
                # 2. Has 4+ consecutive blank lines
                import re as _re
                has_excessive_blanks = bool(_re.search(r'\n{4,}', content))
                # 3. Doesn't end with newline
                missing_trailing_nl = not content.endswith('\n')

                if not (has_trailing_ws or has_excessive_blanks or missing_trailing_nl):
                    continue  # File is already clean, skip

                line_count = len(lines)
                if line_count > 500:
                    tasks.append(Task(
                        priority="P3" if line_count < 1500 else "P2",
                        category="refactor",
                        title=f"Refactor {f.name} ({line_count} lines)",
                        description=f"File is {line_count} lines with cleanup potential",
                        files=[str(f.relative_to(PROJECT_ROOT))],
                        estimated_hours=3.0
                    ))
            except (UnicodeDecodeError, PermissionError):
                pass

    return tasks


def find_todos() -> List[Task]:
    """Find actionable TODO/FIXME markers."""
    tasks = []
    skip_patterns = re.compile(
        r'Phase \d|TODO:.*implement|TODO:.*later|#.*TODO|"TODO|SPEED_HACK|FLY_HACK|NO_CLIP|TODO: Add meaningful',
        re.IGNORECASE
    )

    try:
        result = subprocess.run(
            ["grep", "-rn", r"TODO\|FIXME",
             str(SRC_DIR), "--include=*.cpp", "--include=*.hpp"],
            capture_output=True, text=True, timeout=30
        )

        for line in result.stdout.strip().split("\n"):
            if not line or skip_patterns.search(line):
                continue
            # Skip TODOs in test files (may be auto-generated)
            if "/tests/" in line:
                continue
            # Extract file and line
            parts = line.split(":", 2)
            if len(parts) >= 3:
                filepath = parts[0]
                content = parts[2].strip()
                tasks.append(Task(
                    priority="P2",
                    category="fix",
                    title=f"Address TODO in {Path(filepath).name}",
                    description=content[:200],
                    files=[str(Path(filepath).relative_to(PROJECT_ROOT))],
                    estimated_hours=0.5
                ))
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    return tasks


def find_shallow_tests() -> List[Task]:
    """Find test files with too few test cases for the size/complexity of the source."""
    tasks = []
    test_dir = SRC_DIR / "server" / "tests"

    if not test_dir.exists():
        return tasks

    for test_file in test_dir.glob("Test*.cpp"):
        content = test_file.read_text()
        test_count = len(re.findall(r'TEST_CASE\s*\(', content))
        base = test_file.stem.replace("Test", "")

        # Find corresponding source file
        src_file = None
        for ext in ["*.cpp", "*.hpp"]:
            for f in SRC_DIR.rglob(ext):
                if "tests" in str(f):
                    continue
                if f.stem == base:
                    src_file = f
                    break
            if src_file:
                break

        if not src_file:
            continue

        src_lines = len(src_file.read_text().splitlines())

        # Skip stubs
        if "stub" in src_file.name.lower():
            continue

        # Tightened thresholds: ratio-based, not just count
        # Large files (>500 lines) need at least 1 test per 40 source lines
        # Medium files (>300 lines) need at least 1 test per 50 source lines
        if src_lines > 500:
            min_tests = max(8, src_lines // 40)
        elif src_lines > 300:
            min_tests = max(5, src_lines // 50)
        else:
            continue

        if test_count < min_tests:
            # Prioritize: db and security are P1, others P2
            if any(k in str(src_file) for k in ["/db/", "/security/"]):
                priority = "P1"
            elif src_lines > 800:
                priority = "P1"
            else:
                priority = "P2"

            tasks.append(Task(
                priority=priority,
                category="test-depth",
                title=f"Expand tests for {base} ({test_count} tests for {src_lines} lines)",
                description=f"Source file has {src_lines} lines but only {test_count} test cases — need {min_tests}+",
                files=[str(src_file.relative_to(PROJECT_ROOT)), str(test_file.relative_to(PROJECT_ROOT))],
                estimated_hours=2.0
            ))

    return tasks


def find_only_shallow_tests() -> List[Task]:
    """Find test files that only have sizeof/type checks, no behavioral testing."""
    tasks = []
    test_dir = SRC_DIR / "server" / "tests"

    if not test_dir.exists():
        return tasks

    sizeof_only_patterns = [
        r'sizeof\s*\(',
        r'SECTION\s*\(\s*"sizeof',
        r'SECTION\s*\(\s*"members',
        r'SECTION\s*\(\s*"defaults',
        r'CHECK\s*\(\s*sizeof',
    ]
    behavioral_patterns = [
        r'CHECK\s*\(.+\s*==\s*[^0-9]',  # CHECK with non-constant
        r'CHECK\s*\(.+\s*[<>]',  # CHECK with comparison
        r'REQUIRE\s*\(.+\s*==\s*[^0-9]',
        r'registry\.',
        r'\.update\s*\(',
        r'\.process\s*\(',
        r'\.validate\s*\(',
        r'\.calculate\s*\(',
    ]

    for test_file in test_dir.glob("Test*.cpp"):
        content = test_file.read_text()
        test_count = len(re.findall(r'TEST_CASE\s*\(', content))

        if test_count < 3:
            continue

        # Count sizeof-only patterns
        sizeof_count = sum(len(re.findall(p, content)) for p in sizeof_only_patterns)
        behavioral_count = sum(len(re.findall(p, content)) for p in behavioral_patterns)

        total_checks = sizeof_count + behavioral_count
        if total_checks == 0:
            continue

        # If >60% of assertions are sizeof/type checks, flag it
        if sizeof_count > 0 and sizeof_count / (sizeof_count + behavioral_count) > 0.6 and test_count > 5:
            base = test_file.stem.replace("Test", "")
            tasks.append(Task(
                priority="P2",
                category="test-depth",
                title=f"Add behavioral tests for {base} ({sizeof_count} shallow / {behavioral_count} behavioral)",
                description=f"Test file has {test_count} tests but most are sizeof/type checks — need functional tests",
                files=[str(test_file.relative_to(PROJECT_ROOT))],
                estimated_hours=3.0
            ))

    return tasks


def find_stale_branches() -> List[Task]:
    """Find autonomous branches that are fully merged and can be cleaned up."""
    tasks = []
    try:
        # Get local autonomous branches
        result = subprocess.run(
            ["git", "branch", "--merged", "main", "--list", "autonomous/*"],
            capture_output=True, text=True, timeout=10,
            cwd=str(PROJECT_ROOT)
        )
        merged_local = [b.strip() for b in result.stdout.strip().split("\n") if b.strip()]

        # Get remote autonomous branches (already merged)
        result2 = subprocess.run(
            ["git", "branch", "-r", "--merged", "main", "--list", "origin/autonomous/*"],
            capture_output=True, text=True, timeout=10,
            cwd=str(PROJECT_ROOT)
        )
        merged_remote = [b.strip() for b in result2.stdout.strip().split("\n") if b.strip()]

        total = len(merged_local) + len(merged_remote)
        if total > 0:
            tasks.append(Task(
                priority="P3",
                category="cleanup",
                title=f"Clean up {total} merged autonomous branches",
                description=f"{len(merged_local)} local + {len(merged_remote)} remote branches already merged to main",
                files=[],
                estimated_hours=0.25
            ))
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    return tasks


def find_doc_drift() -> List[Task]:
    """Find documentation files that may be out of date."""
    tasks = []
    docs_dir = PROJECT_ROOT / "docs"
    root_dir = PROJECT_ROOT

    # Check for docs that reference Phase 6/7 planning as future work
    stale_phrases = [
        (r'Phase 8.*Week 1', "Phase 8 planning docs reference Week 1 as current"),
        (r'IN PROGRESS.*Day 1/14', "Status docs still show Day 1/14 progress"),
        (r'Stub.*operational', "Integration status still shows stubs"),
    ]

    stale_files = set()
    for phrase, desc in stale_phrases:
        for md_file in list(docs_dir.glob("*.md")) + list(root_dir.glob("*.md")):
            try:
                content = md_file.read_text()
                if re.search(phrase, content, re.IGNORECASE):
                    stale_files.add(str(md_file.relative_to(PROJECT_ROOT)))
            except (UnicodeDecodeError, PermissionError):
                continue

    if stale_files:
        tasks.append(Task(
            priority="P2",
            category="docs",
            title=f"Update {len(stale_files)} stale documentation files",
            description="Documentation references old project state — needs alignment with current implementation",
            files=list(stale_files)[:10],
            estimated_hours=2.0
        ))

    return tasks


def find_include_deps() -> List[Task]:
    """Find headers with excessive includes that could be forward-declared."""
    tasks = []
    for f in SRC_DIR.rglob("*.hpp"):
        content = f.read_text()
        includes = len(re.findall(r'^\s*#include\s', content, re.MULTILINE))
        lines = len(content.splitlines())

        if includes > 15 and lines > 100:
            tasks.append(Task(
                priority="P3",
                category="refactor",
                title=f"Reduce includes in {f.name} ({includes} includes)",
                description=f"Header has {includes} #include directives — consider forward declarations",
                files=[str(f.relative_to(PROJECT_ROOT))],
                estimated_hours=1.0
            ))

    return tasks


def find_missing_include_guards() -> List[Task]:
    """Find header files missing include guards."""
    tasks = []
    for f in SRC_DIR.rglob("*.hpp"):
        if "/tests/" in str(f) or "stub" in f.name.lower():
            continue
        try:
            content = f.read_text()
        except (UnicodeDecodeError, PermissionError):
            continue

        # Check for #ifndef or #pragma once
        has_guard = bool(re.search(r'#ifndef\s+\w+_HPP_|#pragma\s+once', content))
        if not has_guard:
            tasks.append(Task(
                priority="P2",
                category="refactor",
                title=f"Add include guard to {f.name}",
                description="Header file missing #ifndef guard or #pragma once",
                files=[str(f.relative_to(PROJECT_ROOT))],
                estimated_hours=0.25
            ))
    return tasks


def find_godot_client_gaps() -> List[Task]:
    """Find Godot client integration gaps."""
    tasks = []
    client_dir = PROJECT_ROOT / "src" / "client"

    if not client_dir.exists():
        return tasks

    # Check if Godot imports exist but have errors
    godot_dir = client_dir / ".godot"
    if godot_dir.exists():
        # Check for import errors in the project
        project_file = client_dir / "project.godot"
        if project_file.exists():
            content = project_file.read_text()
            # Look for common Godot build issues
            if "!editor" in content or "!runnable" in content:
                tasks.append(Task(
                    priority="P1",
                    category="fix",
                    title="Fix Godot client project configuration",
                    description="project.godot may have configuration errors",
                    files=[str(project_file.relative_to(PROJECT_ROOT))],
                    estimated_hours=1.0
                ))

    # Check C# scripts for common issues
    for cs_file in client_dir.rglob("*.cs"):
        try:
            content = cs_file.read_text()
            # Check for common Godot C# issues
            if "void Update()" in content and "extends Node" in content:
                # Check if using old GDScript-style Update
                if "delta" not in content:
                    tasks.append(Task(
                        priority="P2",
                        category="fix",
                        title=f"Fix {cs_file.name} - Update method should have delta parameter",
                        description="Godot 4.x requires delta parameter in _Process",
                        files=[str(cs_file.relative_to(PROJECT_ROOT))],
                        estimated_hours=0.5
                    ))
        except (UnicodeDecodeError, PermissionError):
            continue

    return tasks


def find_performance_hotspots() -> List[Task]:
    """Find potential performance hotspots in hot paths."""
    tasks = []

    # Hot path files (called every tick/frame)
    hot_paths = ["ZoneServer.cpp", "NPCAISystem.cpp", "CombatSystem.cpp",
                "PhysicsSystem.cpp", "NetworkManager.cpp"]

    for name in hot_paths:
        for f in SRC_DIR.rglob(name):
            try:
                content = f.read_text()
                lines = len(content.splitlines())

                # Find large functions (>100 lines)
                functions = re.findall(r'(?:void|bool|int|float|entity_t|void\*)\s+(\w+)\s*\([^)]*\)\s*\{[^}]*\{[^}]{100,}[^}][^}]*\}', content)
                if len(functions) > 0:
                    tasks.append(Task(
                        priority="P2",
                        category="perf",
                        title=f"Profile {f.name} - {len(functions)} large functions",
                        description=f"Hot path file with {lines} lines — consider profiling",
                        files=[str(f.relative_to(PROJECT_ROOT))],
                        estimated_hours=2.0
                    ))
            except (UnicodeDecodeError, PermissionError):
                continue

    return tasks


def find_api_unused_in_header() -> List[Task]:
    """Find public methods in headers not called anywhere in the codebase."""
    tasks = []

    for h in SRC_DIR.rglob("*.hpp"):
        if "/tests/" in str(h):
            continue
        try:
            content = h.read_text()
        except (UnicodeDecodeError, PermissionError):
            continue

        # Find public methods (non-static, non-private)
        methods = re.findall(r'^\s*(?!private|protected|static)(\w+(?:<[^>]+>)?)\s+(\w+)\s*\([^)]*\)', content, re.MULTILINE)
        if not methods:
            continue

        # Check if each method is used anywhere in src/
        for ret_type, method_name in methods:
            if method_name in ("init", "update", "process", "tick", "cleanup", "shutdown"):
                continue  # Common names, skip

            # Grep for method usage (quick check, limit to 10 matches)
            result = subprocess.run(
                ["grep", "-r", f"{method_name}\\(", str(SRC_DIR), "--include=*.cpp", "--include=*.hpp", "-l"],
                capture_output=True, text=True, timeout=10
            )
            if result.returncode != 0 or not result.stdout.strip():
                tasks.append(Task(
                    priority="P2",
                    category="fix",
                    title=f"Check if {method_name} in {h.name} is used",
                    description=f"Method '{method_name}' may be unused — verify or remove",
                    files=[str(h.relative_to(PROJECT_ROOT))],
                    estimated_hours=0.5
                ))
                if len(tasks) >= 5:  # Limit results
                    break

    return tasks


def main():
    import sys

    # Check cache first
    cache = load_cache()
    if cache.get("tasks"):
        if "--json" in sys.argv:
            print(json.dumps(cache["tasks"], indent=2))
        else:
            tasks = cache["tasks"]
            print(f"=== DarkAges Task Queue ({len(tasks)} tasks) [cached] ===\n")
            for i, task in enumerate(tasks, 1):
                print(f"{i}. [{task['priority']}] [~{task['estimated_hours']}h] [{task['category']}]")
                print(f"   {task['title']}")
                print(f"   {', '.join(task['files'])}")
                print(f"   {task['description']}")
                print()
        return

    # Discover tasks
    all_tasks = []
    all_tasks.extend(find_missing_tests())
    all_tasks.extend(find_missing_header_tests())
    all_tasks.extend(find_shallow_tests())
    all_tasks.extend(find_only_shallow_tests())
    all_tasks.extend(find_large_files())
    all_tasks.extend(find_include_deps())
    all_tasks.extend(find_todos())
    all_tasks.extend(find_stale_branches())
    all_tasks.extend(find_doc_drift())
    # NEW: Extended discovery patterns
    all_tasks.extend(find_missing_include_guards())
    all_tasks.extend(find_godot_client_gaps())
    all_tasks.extend(find_performance_hotspots())
    all_tasks.extend(find_api_unused_in_header())

    # Deduplicate by title
    seen = set()
    unique_tasks = []
    for task in all_tasks:
        if task.title not in seen:
            seen.add(task.title)
            unique_tasks.append(task)

    # Sort by priority
    priority_order = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}
    unique_tasks.sort(key=lambda t: priority_order.get(t.priority, 99))

    # Filter out tasks from build/_deps directories (third-party code)
    DEP_PATTERNS = ('_deps', 'build_tmp', 'build_psm', 'build_verify',
                    'build_validate', 'build_test', 'build_autonomous')
    unique_tasks = [t for t in unique_tasks
                    if not any(p in str(t.files) for p in DEP_PATTERNS)]

    task_dicts = [asdict(t) for t in unique_tasks]
    save_cache(task_dicts)

    if "--json" in sys.argv:
        print(json.dumps(task_dicts, indent=2))
    else:
        print(f"=== DarkAges Task Queue ({len(task_dicts)} tasks) ===\n")
        for i, task in enumerate(task_dicts, 1):
            print(f"{i}. [{task['priority']}] [~{task['estimated_hours']}h] [{task['category']}]")
            print(f"   {task['title']}")
            print(f"   {', '.join(task['files'])}")
            print(f"   {task['description']}")
            print()


if __name__ == "__main__":
    main()
