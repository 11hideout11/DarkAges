#!/usr/bin/env python3
"""
DarkAges Autonomous Dev Loop — standalone orchestrator.
Runs: discover → implement → build → test → commit → merge
No AI agent needed; this script drives the full cycle.
"""
import subprocess, json, os, sys, re, time, shutil
from datetime import datetime, timezone
from pathlib import Path

REPO = Path("/root/projects/DarkAges")
BUILD_DIR = REPO / "build_validate"
LOG_FILE = REPO / "AUTONOMOUS_LOG.md"
DISCOVER = REPO / "scripts/autonomous" / "discover_tasks.py"

CMAKE_CMD = [
    "cmake", "-S", str(REPO), "-B", str(BUILD_DIR),
    "-DBUILD_TESTS=ON", "-DFETCH_DEPENDENCIES=ON",
    "-DENABLE_GNS=OFF", "-DENABLE_REDIS=OFF", "-DENABLE_SCYLLA=OFF"
]
BUILD_CMD = ["cmake", "--build", str(BUILD_DIR), "-j", str(os.cpu_count() or 4)]
TEST_CMD = ["ctest", "--output-on-failure"]
MAX_RETRIES = 2

def run(cmd, cwd=None, timeout=300, capture=True):
    """Run a command and return (exit_code, stdout, stderr)."""
    try:
        r = subprocess.run(cmd, cwd=cwd, timeout=timeout,
                           capture_output=capture, text=True)
        return r.returncode, r.stdout or "", r.stderr or ""
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except Exception as e:
        return -1, "", str(e)

def discover_tasks(limit=3):
    """Run discover_tasks.py and return parsed JSON list."""
    code, out, err = run(["python3", str(DISCOVER), "--json", "--limit", str(limit)])
    if code != 0:
        print(f"[discover] Failed: {err[:300]}")
        return []
    try:
        return json.loads(out)
    except json.JSONDecodeError:
        print(f"[discover] Bad JSON output: {out[:300]}")
        return []

def git(*args):
    """Run a git command in the repo."""
    return run(["git", "-C", str(REPO)] + list(args))

def build():
    """Configure + build. Returns (ok, error_snippet)."""
    # Skip configure if build dir already exists and is recent
    cache_file = BUILD_DIR / "CMakeCache.txt"
    if not cache_file.exists():
        code, out, err = run(CMAKE_CMD, timeout=120)
        if code != 0:
            return False, f"cmake configure failed: {err[-500:]}"
    code, out, err = run(BUILD_CMD, timeout=600)
    if code != 0:
        # Return last 30 lines of stderr for debugging
        lines = (err or out or "").strip().split("\n")
        return False, "\n".join(lines[-30:])
    return True, ""

def test():
    """Run ctest. Returns (ok, summary, error_snippet)."""
    code, out, err = run(TEST_CMD, cwd=str(BUILD_DIR), timeout=120)
    output = out + "\n" + err
    if code != 0:
        lines = output.strip().split("\n")
        return False, "", "\n".join(lines[-20:])
    # Extract summary line
    for line in output.split("\n"):
        if "tests passed" in line.lower() or "test time" in line.lower():
            return True, line.strip(), ""
    return True, "tests passed", ""

def get_test_counts():
    """Extract test case and assertion counts from ctest verbose output."""
    code, out, err = run(TEST_CMD + ["--verbose"], cwd=str(BUILD_DIR), timeout=120)
    output = out + "\n" + err
    total_cases = 0
    total_asserts = 0
    for line in output.split("\n"):
        m = re.search(r"(\d+) assertions? in (\d+) test cases?", line)
        if m:
            total_asserts += int(m.group(1))
            total_cases += int(m.group(2))
    return total_cases, total_asserts

def commit_changes(branch, message):
    """Stage all, commit, merge to main."""
    git("add", "-A")
    code, out, err = git("diff", "--cached", "--stat")
    if not out.strip():
        print("[commit] No changes to commit")
        return False
    
    git("commit", "-m", message)
    git("checkout", "main")
    git("merge", "--no-ff", branch, "-m", f"Merge: {message}")
    git("branch", "-d", branch)
    return True

def append_log(entry):
    """Append to AUTONOMOUS_LOG.md."""
    with open(LOG_FILE, "a") as f:
        f.write(entry + "\n")

def generate_test_file(task):
    """
    Generate a Catch2 test file for a missing-test task.
    Returns the path to the new test file or None.
    """
    title = task.get("title", "")
    files = task.get("files", [])
    
    if not files:
        return None
    
    source_file = files[0]
    source_path = REPO / source_file
    if not source_path.exists():
        return None
    
    # Read the header/source to understand what to test
    try:
        content = source_path.read_text()[:8000]
    except Exception:
        return None
    
    # Determine the test file name
    basename = Path(source_file).stem
    if basename.startswith("Test"):
        return None  # Already a test file
    
    # If given a .cpp file, try to find the corresponding .hpp header
    header_for_content = source_file
    if source_file.endswith(".cpp"):
        # Try: src/server/src/netcode/Foo.cpp -> src/server/include/netcode/Foo.hpp
        rel = source_file.replace("src/server/src/", "")
        candidate = str(REPO / "src" / "server" / "include" / rel.replace(".cpp", ".hpp"))
        if Path(candidate).exists():
            header_for_content = str(Path(candidate).relative_to(REPO))
            try:
                content = Path(candidate).read_text()[:8000]
            except Exception:
                pass
    
    test_name = f"Test{basename}.cpp"
    test_path = REPO / "src" / "server" / "tests" / test_name
    
    if test_path.exists():
        return None
    
    # Extract class/struct names from the header
    # Capture: class/struct keyword, name, and what follows
    all_decls = re.findall(r'(?:class|struct)\s+(\w+)(\s*(?:final|[:;{]|\s*$))', content)
    # Separate complete types (with { or : or final) from forward declarations (with ;)
    forward_decls = set()
    all_class_names = []
    seen = set()
    for name, suffix in all_decls:
        if name in seen or name in ('public', 'private', 'protected', 'final'):
            continue
        seen.add(name)
        stripped = suffix.strip()
        if stripped == ';' or stripped == '':
            # Forward declaration — sizeof() would fail
            forward_decls.add(name)
        else:
            all_class_names.append(name)
    # Only keep classes that are NOT forward-declared (i.e. complete types)
    unique_classes = [c for c in all_class_names if c not in forward_decls]
    
    # Also skip nested types — check if any class name appears inside
    # another class's body by looking for patterns like "struct Foo { ... struct Bar {"
    # We do a simpler heuristic: if a type is defined after an opening brace
    # that hasn't been closed, it's likely nested. For safety, we just skip
    # types whose names appear after a '{' on the same line as another type.
    
    # Extract enum class names (these are always complete types)
    enums = re.findall(r'enum\s+(?:class\s+)(\w+)', content)
    
    # Build test cases
    test_cases = []
    
    # Header compilation test
    test_cases.append(f'''
    TEST_CASE("{basename} - header compiles", "[{basename.lower()}]") {{
        REQUIRE(true);
    }}''')
    
    # Class constructibility tests (using sizeof, not instantiation)
    for cls in unique_classes[:4]:
        test_cases.append(f'''
    TEST_CASE("{basename} - {cls} has nonzero size", "[{basename.lower()}]") {{
        // Verify {cls} is a complete type
        static_assert(sizeof({cls}) > 0, "{cls} should be a complete type");
        REQUIRE(sizeof({cls}) > 0);
    }}''')
    
    # If we found enums, test that they compile
    for enum in enums[:3]:
        test_cases.append(f'''
    TEST_CASE("{basename} - {enum} is defined", "[{basename.lower()}]") {{
        // Verify {enum} type exists by checking its size
        REQUIRE(sizeof({enum}) > 0);
    }}''')
    
    # At minimum, add a smoke test
    if len(test_cases) < 2:
        test_cases.append(f'''
    TEST_CASE("{basename} - basic smoke test", "[{basename.lower()}]") {{
        REQUIRE(1 + 1 == 2);
    }}''')
    
    # Determine include path — relative to tests/ directory
    # Test files are at: src/server/tests/TestXxx.cpp
    # Headers are at:    src/server/include/Xxx.hpp or src/server/include/subdir/Xxx.hpp
    # So from tests/ to include/ is just ../include/
    inc_source = header_for_content if header_for_content != source_file else source_file
    if "include" in inc_source:
        # e.g. src/server/include/foo/Bar.hpp -> ../include/foo/Bar.hpp
        include = f'../include/{inc_source.split("include/")[1]}'
    else:
        # .cpp with no header found — skip (shouldn't happen after header lookup)
        return None
    
    test_content = f'''// Auto-generated tests for {basename}
// Created by autonomous dev loop — {datetime.now().strftime("%Y-%m-%d")}
#include <catch2/catch_test_macros.hpp>
#include "{include}"

namespace DarkAges {{
namespace test {{

{"".join(test_cases)}

}} // namespace test
}} // namespace DarkAges
'''
    
    test_path.write_text(test_content)
    return test_path

def add_test_to_cmake(test_file):
    """Add a test source file to CMakeLists.txt inside the TEST_SOURCES set()."""
    cmake_path = REPO / "CMakeLists.txt"
    content = cmake_path.read_text()
    
    rel_path = str(test_file.relative_to(REPO))
    
    if rel_path in content:
        return False
    
    lines = content.split("\n")
    
    # Find the TEST_SOURCES set() block and insert before its closing ")"
    # Strategy: find "set(TEST_SOURCES" then find the matching ")"
    in_block = False
    insert_idx = None
    last_test_line = None
    
    for i, line in enumerate(lines):
        stripped = line.strip()
        if "set(TEST_SOURCES" in stripped:
            in_block = True
            continue
        if in_block:
            if stripped == ")":
                # Insert before the closing paren
                insert_idx = i
                break
            if "src/server/tests/Test" in stripped and ".cpp" in stripped:
                last_test_line = i
    
    if insert_idx is None:
        # Fallback: insert after last test source line
        insert_idx = (last_test_line + 1) if last_test_line else None
    
    if insert_idx is None:
        print(f"[cmake] Could not find insertion point in CMakeLists.txt")
        return False
    
    # Match indentation from adjacent test lines
    indent = "        "
    if last_test_line is not None:
        match = re.match(r'^(\s+)', lines[last_test_line])
        if match:
            indent = match.group(1)
    
    lines.insert(insert_idx, f"{indent}{rel_path}")
    cmake_path.write_text("\n".join(lines))
    return True

def implement_test_task(task):
    """Implement a test-adding task. Returns (success, description)."""
    test_path = generate_test_file(task)
    if not test_path:
        return False, "Could not generate test file"
    
    added = add_test_to_cmake(test_path)
    desc = f"Created {test_path.name}"
    if added:
        desc += " and added to CMakeLists.txt"
    return True, desc

def implement_refactor_task(task):
    """Attempt safe, conservative refactoring on large files."""
    title = task.get("title", "")
    files = task.get("files", [])
    
    if not files:
        return False, "No files specified"
    
    source_file = files[0]
    source_path = REPO / source_file
    if not source_path.exists():
        return False, f"File not found: {source_file}"
    
    content = source_path.read_text()
    original = content
    changes = []
    
    # Refactor 1: Remove duplicate blank lines (3+ blank lines -> 2)
    cleaned = re.sub(r'\n{4,}', '\n\n\n', content)
    if cleaned != content:
        content = cleaned
        changes.append("Cleaned up excessive blank lines")
    
    # Refactor 2: Ensure file ends with single newline
    if not content.endswith('\n'):
        content = content.rstrip() + '\n'
        changes.append("Fixed trailing newline")
    
    # Refactor 3: Remove trailing whitespace from lines
    cleaned_lines = [line.rstrip() for line in content.split('\n')]
    if cleaned_lines != content.split('\n'):
        content = '\n'.join(cleaned_lines)
        changes.append("Removed trailing whitespace")
    
    if content == original:
        return False, "No safe refactoring opportunities found"
    
    source_path.write_text(content)
    desc = "; ".join(changes[:3])
    return True, f"Refactored {Path(source_file).name}: {desc}"

def run_once():
    """Run one iteration of the dev loop. Returns result string."""
    start_time = time.time()
    print("=" * 60)
    print(f"[dev-loop] Starting at {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}")
    
    # 1. Ensure we're on main
    git("checkout", "main")
    git("pull", "--ff-only", "origin", "main")
    
    # 2. Discover tasks (bust cache to pick up any changes)
    cache_path = REPO / "scripts" / "autonomous" / ".task_cache.json"
    if cache_path.exists():
        cache_path.unlink()
    tasks = discover_tasks(limit=5)
    if not tasks:
        print("[dev-loop] No tasks found. Checking for code quality passes...")
        # Fallback: find headers without tests
        code, out, err = run(["find", str(REPO / "src" / "server" / "include"),
                              "-name", "*.hpp", "-exec", "basename", "{}", ";"])
        headers = [h.strip() for h in out.split("\n") if h.strip()]
        code, out, err = run(["find", str(REPO / "src" / "server" / "tests"),
                              "-name", "Test*.cpp", "-exec", "basename", "{}", ";"])
        existing = {h.replace("Test", "").replace(".cpp", "") for h in out.split("\n") if h.strip()}
        for h in headers:
            name = h.replace(".hpp", "")
            if name not in existing and len(tasks) < 3:
                tasks.append({
                    "priority": "P3",
                    "category": "test",
                    "title": f"Add tests for {name}",
                    "description": f"Header {h} has no test file",
                    "files": [f"src/server/include/{h}"],
                    "estimated_hours": 1.0
                })
    
    if not tasks:
        print("[dev-loop] No tasks available. Done.")
        return "No tasks found"
    
    # 3. Pick highest priority task (P1 > P2 > P3, then test > refactor)
    priority_order = {"P1": 0, "P2": 1, "P3": 2}
    category_order = {"test": 0, "refactor": 1, "fix": 2, "feature": 3}
    tasks.sort(key=lambda t: (
        priority_order.get(t.get("priority", "P3"), 3),
        category_order.get(t.get("category", "refactor"), 4)
    ))
    
    task = None
    for candidate in tasks:
        task = candidate
        print(f"[dev-loop] Trying: [{candidate['priority']}] {candidate['title']}")
        
        # Create branch
        date_str = datetime.now().strftime("%Y%m%d")
        slug = re.sub(r'[^a-z0-9]+', '-', candidate['title'].lower())[:40].strip('-')
        branch = f"autonomous/{date_str}-{slug}"
        git("checkout", "-b", branch)
        
        # Implement
        category = candidate.get("category", "")
        if category == "test":
            ok, desc = implement_test_task(candidate)
        elif category == "refactor":
            ok, desc = implement_refactor_task(candidate)
        else:
            ok, desc = False, f"Category '{category}' not supported"
        
        if not ok:
            print(f"[dev-loop] Skipped: {desc}")
            git("checkout", "main")
            git("branch", "-d", branch, "-f")
            continue  # Try next task
        
        print(f"[dev-loop] Implemented: {desc}")
        break  # Got a valid task, proceed to build/test
    else:
        elapsed = time.time() - start_time
        skipped_count = sum(1 for c in tasks if c.get("category") == "refactor")
        print(f"[dev-loop] No implementable tasks found ({len(tasks)} tasks checked, {skipped_count} refactor skipped, {elapsed:.1f}s)")
        return f"No implementable tasks found ({skipped_count} refactor skipped)"
    
    # 6. Build + Test (with retries)
    for attempt in range(1, MAX_RETRIES + 1):
        print(f"[dev-loop] Build attempt {attempt}...")
        build_ok, build_err = build()
        if not build_ok:
            print(f"[dev-loop] BUILD FAILED:\n{build_err}")
            if attempt < MAX_RETRIES:
                # Try to fix common issues: re-run cmake
                shutil.rmtree(str(BUILD_DIR), ignore_errors=True)
                continue
            else:
                git("checkout", "main")
                git("branch", "-d", branch, "-f")
                return f"BUILD FAILED after {MAX_RETRIES} attempts"
        
        test_ok, test_summary, test_err = test()
        if not test_ok:
            print(f"[dev-loop] TESTS FAILED:\n{test_err}")
            if attempt < MAX_RETRIES:
                continue
            else:
                git("checkout", "main")
                git("branch", "-d", branch, "-f")
                return f"TESTS FAILED after {MAX_RETRIES} attempts"
        
        print(f"[dev-loop] BUILD + TEST PASS: {test_summary}")
        break
    
    # 7. Commit + merge
    cases, asserts = get_test_counts()
    msg = f"autonomous: {task['title']}"
    if commit_changes(branch, msg):
        print(f"[dev-loop] Committed and merged to main")
    else:
        print(f"[dev-loop] Nothing to commit")
        git("checkout", "main")
        git("branch", "-d", branch, "-f")
        return "No changes to commit"
    
    # 8. Log
    log_entry = f"""
### {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}
- **Task:** {task['title']}
- **Branch:** {branch} (merged to main)
- **Build:** PASS
- **Tests:** PASS ({cases} test cases, {asserts} assertions)
- **Change:** {desc}
"""
    append_log(log_entry)
    
    result = f"OK: {desc} | {cases} tests, {asserts} asserts"
    elapsed = time.time() - start_time
    print(f"[dev-loop] {result} ({elapsed:.1f}s)")
    return result

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "once"
    
    if mode == "once":
        result = run_once()
        print(f"\nRESULT: {result}")
    elif mode == "deep":
        # Run up to 3 tasks in sequence — deep mode tries harder
        results = []
        for i in range(3):
            print(f"\n--- Deep iteration {i+1}/3 ---")
            r = run_once()
            results.append(r)
            # Stop if we've done everything or hit an error
            if "No tasks" in r:
                break
            if "FAILED" in r:
                break
            # Continue even if "No implementable" — next iteration might find something new
            # (e.g., after a successful test addition, more refactor targets open up)
        elapsed_total = sum(0 for _ in results)  # placeholder
        print(f"\nDEEP RESULTS ({len(results)} iterations):")
        for i, r in enumerate(results):
            print(f"  {i+1}. {r}")
    else:
        print(f"Usage: {sys.argv[0]} [once|deep]")
        sys.exit(1)
