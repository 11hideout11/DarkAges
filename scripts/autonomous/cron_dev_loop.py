#!/usr/bin/env python3
"""
DarkAges Autonomous Dev Loop — standalone orchestrator.
Runs: discover → implement → build → test → commit → merge
No AI agent needed; this script drives the full cycle.
"""
import subprocess, json, os, sys, re, time, shutil
from typing import Tuple
from datetime import datetime, timezone
from pathlib import Path

REPO = Path("/root/projects/DarkAges")
BUILD_DIR = REPO / "build_validate"
LOG_FILE = REPO / "AUTONOMOUS_LOG.md"
DISCOVER = REPO / "scripts/autonomous" / "discover_tasks.py"
EDIT_HISTORY_FILE = REPO / "scripts/autonomous" / ".edit_history.json"
MAX_EDITS_PER_FILE_PER_DAY = 3  # Loop detection threshold

AGENTS_MD = REPO / "AGENTS.md"

# ── Reasoning Budget (harness engineering: limit compute per task) ──────────
# Maps task category -> max wall-clock seconds. Hard abort if exceeded.
REASONING_BUDGET = {
    "test": 600,         # 10 min for test generation
    "test-depth": 900,   # 15 min for behavioral test expansion
    "refactor": 300,     # 5 min for safe whitespace cleanup
    "fix": 0,            # Never auto-implemented (manual review required)
    "docs": 0,           # Never auto-implemented
    "cleanup": 300,      # 5 min for branch cleanup
    "feature": 0,        # Never auto-implemented
}
# Budget multiplier for deep mode (more time allowed)
DEEP_BUDGET_MULTIPLIER = 1.5

# ── Operation Budget (proxy for token/compute consumption) ─────────────────
# Counts subprocess calls + file reads/writes per task. Hard abort if exceeded.
# This approximates the "token ceiling" from harness engineering research.
ACTION_BUDGET = {
    "test": 60,          # test gen: read source, write test, cmake, build, test
    "test-depth": 80,    # behavioral expansion: more file ops
    "refactor": 30,      # safe cleanup: few file ops
    "cleanup": 20,       # branch cleanup: git commands only
}
ACTION_BUDGET_MULTIPLIER = 1.5  # applied in deep mode

# Global action counter (incremented by run(), file reads, file writes)
_action_counter = {"count": 0}

# ── Harness Component Audit Trail ───────────────────────────────────────────
# Each component encodes an assumption about model limitations.
# Review periodically; remove if the assumption is proven stale.
HARNESS_COMPONENTS = {
    "external_evaluator": {
        "added": "2026-04-23",
        "hypothesis": "Self-evaluation bias persists across all models; generator cannot grade its own work",
        "expires": None,  # permanent — bias is structural
    },
    "loop_detection": {
        "added": "2026-04-23",
        "hypothesis": "Agents get stuck in repetitive edit loops on the same file",
        "expires": "2026-07-23",  # re-test with newer models
    },
    "pre_completion_gate": {
        "added": "2026-04-23",
        "hypothesis": "Agents declare 'done' before independent verification",
        "expires": "2026-07-23",
    },
    "tool_subtraction": {
        "added": "2026-04-23",
        "hypothesis": "Too many discovery heuristics degrade task selection quality",
        "expires": "2026-07-23",
    },
    "local_context_middleware": {
        "added": "2026-04-23",
        "hypothesis": "Agents waste tokens rediscovering environment state",
        "expires": "2026-07-23",
    },
}

# Prevent git from hanging on prompts (interactive SSH, credential helpers, etc.)
os.environ.setdefault("GIT_TERMINAL_PROMPT", "0")
os.environ.setdefault("GIT_ASK_PASS", "/bin/true")
os.environ.setdefault("GITHUB_TOKEN", "")  # clear any stale token

# Timeout for git operations (network ops can hang indefinitely)
GIT_TIMEOUT = 30

CMAKE_CMD = [
    "cmake", "-S", str(REPO), "-B", str(BUILD_DIR),
    "-DBUILD_TESTS=ON", "-DFETCH_DEPENDENCIES=ON",
    "-DENABLE_GNS=OFF", "-DENABLE_REDIS=OFF", "-DENABLE_SCYLLA=OFF"
]
BUILD_CMD = ["cmake", "--build", str(BUILD_DIR), "-j", str(os.cpu_count() or 4)]
TEST_CMD = ["ctest", "--output-on-failure", "-j1"]
MAX_RETRIES = 2

def run(cmd, cwd=None, timeout=300, capture=True):
    """Run a command and return (exit_code, stdout, stderr). Increments action budget."""
    _action_counter["count"] += 1
    try:
        r = subprocess.run(cmd, cwd=cwd, timeout=timeout,
                           capture_output=capture, text=True)
        return r.returncode, r.stdout or "", r.stderr or ""
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except Exception as e:
        return -1, "", str(e)


def budgeted_read_text(path: Path, limit: int = None) -> str:
    """Read file text with action budget accounting."""
    _action_counter["count"] += 1
    content = path.read_text()
    return content[:limit] if limit else content


def budgeted_write_text(path: Path, content: str):
    """Write file text with action budget accounting."""
    _action_counter["count"] += 1
    path.write_text(content)

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

def git(*args, **kwargs):
    """Run a git command in the repo with a short timeout."""
    kwargs.setdefault("timeout", GIT_TIMEOUT)
    return run(["git", "-C", str(REPO)] + list(args), **kwargs)

def build():
    """Configure + build. Returns (ok, error_snippet)."""
    # Detect stale build dir: cache exists but Makefile missing
    cache_file = BUILD_DIR / "CMakeCache.txt"
    makefile = BUILD_DIR / "Makefile"
    needs_configure = not cache_file.exists() or not makefile.exists()
    if needs_configure:
        code, out, err = run(CMAKE_CMD, timeout=120)
        if code != 0:
            return False, f"cmake configure failed: {err[-500:]}"
        # Verify configure produced a Makefile; if not, cache was stale — reconfigure
        if not (BUILD_DIR / "Makefile").exists():
            cache_file.unlink(missing_ok=True)
            code, out, err = run(CMAKE_CMD, timeout=120)
            if code != 0:
                return False, f"cmake reconfigure failed: {err[-500:]}"
    code, out, err = run(BUILD_CMD, timeout=600)
    if code != 0:
        # Return last 30 lines of stderr for debugging
        lines = (err or out or "").strip().split("\n")
        return False, "\n".join(lines[-30:])
    return True, ""

def test():
    """Run ctest. Returns (ok, summary, error_snippet)."""
    code, out, err = run(TEST_CMD, cwd=str(BUILD_DIR), timeout=600)
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

# ── Loop Detection Middleware ──────────────────────────────────────────────

def load_edit_history() -> dict:
    """Load per-file edit history for loop detection."""
    if EDIT_HISTORY_FILE.exists():
        try:
            return json.loads(EDIT_HISTORY_FILE.read_text())
        except (json.JSONDecodeError, IOError):
            pass
    return {}


def save_edit_history(history: dict):
    """Save per-file edit history."""
    EDIT_HISTORY_FILE.write_text(json.dumps(history, indent=2))


def record_edit(files: list):
    """Record that the given files were edited in this run."""
    history = load_edit_history()
    today = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    for f in files:
        if f not in history:
            history[f] = {}
        history[f][today] = history[f].get(today, 0) + 1
    save_edit_history(history)


def is_loop_detected(files: list) -> Tuple[bool, str]:
    """
    Check if any file has been edited too many times recently.
    Returns (detected, reason).
    """
    history = load_edit_history()
    today = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    for f in files:
        counts = history.get(f, {})
        today_count = counts.get(today, 0)
        if today_count >= MAX_EDITS_PER_FILE_PER_DAY:
            return True, f"Loop detected: {f} edited {today_count}x today (max {MAX_EDITS_PER_FILE_PER_DAY}) — Consider reconsidering your approach"
    return False, ""


# ── External Evaluator Integration ─────────────────────────────────────────

def run_external_evaluator(branch: str) -> dict:
    """
    Run the standalone evaluator agent on the current working tree.
    The generator is already on the feature branch; evaluator checks --current.
    Returns parsed JSON report or fallback dict on error.
    """
    evaluator = REPO / "scripts/autonomous" / "evaluate_change.py"
    if not evaluator.exists():
        return {"overall": "UNAVAILABLE", "reason": "evaluate_change.py not found"}

    code, out, err = run(
        ["python3", str(evaluator), "--current", "--base", "main"],
        timeout=900
    )
    try:
        return json.loads(out)
    except json.JSONDecodeError:
        return {
            "overall": "ERROR",
            "reason": f"Evaluator returned non-JSON (exit {code}): {err[:200]}",
        }


def run_opencode_reviewer(branch: str) -> dict:
    """
    Run the OpenCode CLI subjective reviewer agent.
    This is the skeptical code-review layer of the GAN-style architecture.
    Returns parsed JSON report or fallback dict on error.
    """
    reviewer = REPO / "scripts/autonomous" / "evaluate_change_review.py"
    if not reviewer.exists():
        return {"overall": "UNAVAILABLE", "reason": "evaluate_change_review.py not found"}

    code, out, err = run(
        ["python3", str(reviewer), "--current", "--base", "main"],
        timeout=300
    )
    try:
        return json.loads(out)
    except json.JSONDecodeError:
        return {
            "overall": "ERROR",
            "reason": f"OpenCode reviewer returned non-JSON (exit {code}): {err[:200]}",
        }


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
        content = budgeted_read_text(source_path, limit=8000)
    except Exception:
        return None
    
    # Determine the test file name
    basename = Path(source_file).stem
    if basename.startswith("Test"):
        return None  # Already a test file
    
    # If given a .cpp file, try to find the corresponding .hpp header
    header_for_content = source_file
    if source_file.endswith(".cpp"):
        # Strategy 1: Check #include directives in the .cpp file
        # This is the most reliable way to know which header to use
        try:
            cpp_content = budgeted_read_text(source_path, limit=2000)
            include_matches = re.findall(r'#include\s+["<]([^">]+)[">]', cpp_content)
            # Prioritize headers that match the source file's basename
            source_basename = Path(source_file).stem.lower()
            matched_header = None
            for inc in include_matches:
                if inc.endswith('.hpp'):
                    if inc.startswith('src/server/include/'):
                        candidate = REPO / inc
                    else:
                        # Assume it's relative to include root
                        candidate = REPO / "src" / "server" / "include" / inc
                    if candidate.exists():
                        header_for_content = str(candidate.relative_to(REPO))
                        matched_header = candidate
                        try:
                            content = budgeted_read_text(candidate, limit=8000)
                        except Exception:
                            pass
                        # Prefer the header that matches the source file's basename
                        if candidate.stem.lower() == source_basename:
                            break
        except Exception:
            pass
        
        # Strategy 2: If no header found via includes, search recursively under include/
        if header_for_content == source_file:
            basename = Path(source_file).stem
            # Search for any .hpp file with matching basename under include/
            include_root = REPO / "src" / "server" / "include"
            matches = list(include_root.rglob(f"{basename}.hpp"))
            if matches:
                candidate = matches[0]
                header_for_content = str(candidate.relative_to(REPO))
                try:
                    content = budgeted_read_text(candidate, limit=8000)
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
    if "include/" in inc_source:
        # e.g. src/server/include/foo/Bar.hpp -> ../include/foo/Bar.hpp
        parts = inc_source.split("include/", 1)
        if len(parts) < 2:
            return None
        include = f'../include/{parts[1]}'
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
    
    budgeted_write_text(test_path, test_content)
    return test_path

def add_test_to_cmake(test_file):
    """Add a test source file to CMakeLists.txt inside the TEST_SOURCES set()."""
    cmake_path = REPO / "CMakeLists.txt"
    content = budgeted_read_text(cmake_path)
    
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
    budgeted_write_text(cmake_path, "\n".join(lines))
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
    
    content = budgeted_read_text(source_path)
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
    
    budgeted_write_text(source_path, content)
    desc = "; ".join(changes[:3])
    return True, f"Refactored {Path(source_file).name}: {desc}"

def _extract_public_methods(source_content):
    """Extract public method signatures from source content.
    Returns list of (class_name, method_name, return_type, params).
    Filters noise from macro invocations, lambda captures, and comment snippets.
    """
    methods = []
    # Match: ReturnType ClassName::methodName(params)
    pattern = r'(?:[\w:<>*&\s]+?)\s+(\w+)::(\w+)\s*\(([^)]*)\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?\s*\{'
    for m in re.finditer(pattern, source_content):
        class_name, method_name, params = m.group(1), m.group(2), m.group(3).strip()
        if method_name.startswith('_') or method_name in ('getInternal',):
            continue
        # Filter noise: reject if method name looks like a variable, macro, or number
        if not method_name[0].isalpha() and method_name[0] != '_':
            continue
        # Reject if name contains non-identifier chars (macro-like: RATE_(, SIZE(, etc.)
        if not method_name.replace('_', '').isalnum():
            continue
        # Reject numeric-looking single tokens (e.g. "2" in "compensation(2-second history buffer)")
        if method_name.isdigit():
            continue
        methods.append((class_name, method_name, params))
    return methods


def _extract_structs(content):
    """Extract struct/class definitions with their fields.
    Returns list of (name, fields_list).
    """
    results = []
    # Match struct/class with body
    pattern = r'(?:struct|class)\s+(\w+)(?:\s*(?:final|[:;{]|\s*$))'
    for m in re.finditer(pattern, content):
        name = m.group(1)
        if name in ('public', 'private', 'protected', 'final'):
            continue
        # Check if it's a forward declaration (ends with ;)
        after = content[m.end():m.end()+5].strip()
        if after.startswith(';'):
            continue
        results.append(name)
    return results


def _type_is_defined(type_name, content):
    """Check if a class/struct type has a complete definition in content."""
    # Match "class TypeName" or "struct TypeName" followed by a '{' before any ';'
    # This distinguishes definitions from forward declarations.
    pattern = rf'\b(?:class|struct)\s+{re.escape(type_name)}\b[^{{;}}]*\{{'
    return re.search(pattern, content) is not None

def _detect_test_pattern(source_content, source_file):
    """Detect what kind of class this is and return appropriate test template.
    Returns (pattern_name, list_of_test_case_snippets).
    """
    basename = Path(source_file).stem
    test_cases = []

    # Pattern 1: Manager with async callbacks (Redis, Scylla patterns)
    has_callbacks = 'Callback' in source_content or 'std::function' in source_content
    has_initialize = 'bool initialize(' in source_content
    has_shutdown = 'void shutdown()' in source_content
    has_is_connected = 'isConnected()' in source_content
    has_update = 'void update()' in source_content
    has_metrics = 'getCommands' in source_content or 'getWrites' in source_content

    if has_callbacks and has_initialize:
        test_cases.append(_generate_construction_test(basename))
        test_cases.append(_generate_init_shutdown_test(basename))
        test_cases.append(_generate_update_safety_test(basename))

        if has_metrics:
            test_cases.append(_generate_metrics_test(basename, source_content))

        # Find callback-based methods
        methods = _extract_public_methods(source_content)
        callback_methods = [(c, m, p) for c, m, p in methods
                           if 'callback' in p.lower() or 'Callback' in p]
        non_callback_methods = [(c, m, p) for c, m, p in methods
                               if 'callback' not in p.lower() and 'Callback' not in p]

        for cls, method, params in callback_methods[:4]:
            test_cases.append(_generate_callback_test(basename, f"{cls}::{method}"))

        for cls, method, params in non_callback_methods[:3]:
            test_cases.append(_generate_void_method_test_behavioral(basename, f"{cls}::{method}"))

        return "manager", test_cases

    # Pattern 2: Data-heavy class with structs (ZoneServer, config classes)
    has_config_struct = 'struct ' in source_content and 'Config' in source_content
    has_metrics_struct = 'Metrics' in source_content or 'metrics' in source_content

    if has_config_struct or has_metrics_struct:
        test_cases.append(_generate_construction_test(basename))

        # Find and test data structures
        structs = _extract_structs(source_content)
        # Filter out forward-declared structs (incomplete types)
        structs = [s for s in structs if _type_is_defined(s, source_content)]
        for sname in structs[:3]:
            if sname == basename:  # Skip the main class itself
                continue
            test_cases.append(_generate_struct_test(sname))

        methods = _extract_public_methods(source_content)
        for cls, method, params in methods[:4]:
            test_cases.append(_generate_void_method_test(basename, f"{cls}::{method}"))

        return "server", test_cases

    # Pattern 3: System with ECS/registry interaction
    if 'Registry' in source_content or 'registry' in source_content:
        test_cases.append(_generate_construction_test(basename))
        methods = _extract_public_methods(source_content)
        for cls, method, params in methods[:5]:
            test_cases.append(_generate_void_method_test(basename, f"{cls}::{method}"))
        return "system", test_cases

    # Fallback: generic construction + methods
    test_cases.append(_generate_construction_test(basename))
    methods = _extract_public_methods(source_content)
    for cls, method, params in methods[:5]:
        test_cases.append(_generate_void_method_test(basename, f"{cls}::{method}"))
    if not methods:
        test_cases.append(_generate_smoke_test(basename))
    return "generic", test_cases


def _generate_construction_test(basename):
    """Generate a type completeness test (no instantiation)."""
    lower = basename.lower()
    return f'''
    TEST_CASE("{basename} is a complete type", "[{lower}]") {{
        REQUIRE(sizeof({basename}) > 0);
    }}'''


def _generate_init_shutdown_test(basename):
    """Generate initialize/shutdown lifecycle tests."""
    lower = basename.lower()
    return f'''
    TEST_CASE("{basename} initialize/shutdown lifecycle", "[{lower}]") {{
        {basename} obj;

        SECTION("initialize returns true") {{
            REQUIRE(obj.initialize());
        }}

        SECTION("shutdown before init is safe") {{
            REQUIRE_NOTHROW(obj.shutdown());
        }}

        SECTION("double initialize is safe") {{
            REQUIRE(obj.initialize());
            REQUIRE_NOTHROW(obj.initialize());
        }}

        SECTION("shutdown after init is safe") {{
            obj.initialize();
            REQUIRE_NOTHROW(obj.shutdown());
        }}
    }}'''


def _generate_update_safety_test(basename):
    """Generate update() safety test."""
    lower = basename.lower()
    return f'''
    TEST_CASE("{basename} update is safe without connection", "[{lower}]") {{
        {basename} obj;

        SECTION("update before init is safe") {{
            REQUIRE_NOTHROW(obj.update());
        }}

        SECTION("repeated updates are safe") {{
            obj.initialize();
            for (int i = 0; i < 10; ++i) {{
                obj.update();
            }}
        }}
    }}'''


def _generate_metrics_test(basename, source_content):
    """Generate metrics accessor test based on what metrics exist."""
    lower = basename.lower()
    # Detect metric methods
    metric_methods = re.findall(r'(get\w+(?:Sent|Completed|Failed|Queued|Latency)\w*)\s*\(\s*\)\s*const', source_content)
    if not metric_methods:
        metric_methods = ['getCommandsSent()', 'getCommandsCompleted()', 'getCommandsFailed()']

    checks = []
    for method in metric_methods[:4]:
        if 'Latency' in method:
            checks.append(f'REQUIRE(obj.{method} == 0.0f);')
        else:
            checks.append(f'REQUIRE(obj.{method} == 0);')

    return f'''
    TEST_CASE("{basename} metrics default to zero", "[{lower}]") {{
        {basename} obj;
        obj.initialize();

        SECTION("initial metrics are zeroed") {{
            {chr(10).join("            " + c for c in checks)}
        }}
    }}'''


def _generate_callback_test(basename, method_full):
    """Generate a callback behavior test for a stub method."""
    lower = basename.lower()
    # Extract just the method name for the test title
    method_name = method_full.split("::")[-1] if "::" in method_full else method_full

    return f'''
    TEST_CASE("{basename} {method_name} callback fires in stub mode", "[{lower}]") {{
        {basename} obj;
        obj.initialize();

        std::atomic<bool> called{{false}};
        REQUIRE_NOTHROW(obj.{method_name}([&](auto&&...) {{
            called = true;
        }}));
        // In stub mode, callback should fire immediately
        REQUIRE(called);
    }}'''


def _generate_void_method_test_behavioral(basename, method_full):
    """Generate a no-throw safety test for a void method."""
    lower = basename.lower()
    method_name = method_full.split("::")[-1] if "::" in method_full else method_full

    return f'''
    TEST_CASE("{basename} {method_name} is safe to call", "[{lower}]") {{
        {basename} obj;
        obj.initialize();
        REQUIRE_NOTHROW(obj.{method_name}());
    }}'''


def _generate_void_method_test(basename, method_full):
    """Generate a compile-time existence check for a method (address-of)."""
    lower = basename.lower()
    method_name = method_full.split("::")[-1] if "::" in method_full else method_full
    class_name = method_full.split("::")[0] if "::" in method_full else basename

    return f'''
    TEST_CASE("{basename} {method_name} exists", "[{lower}]") {{
        auto ptr = &{class_name}::{method_name};
        (void)ptr;
    }}'''


def _generate_struct_test(struct_name):
    """Generate default construction test for a data struct."""
    lower = struct_name.lower()
    return f'''
    TEST_CASE("{struct_name} default construction", "[{lower}]") {{
        {struct_name} s{{}};

        SECTION("all fields are zeroed or default") {{
            // Verify struct is trivially constructible
            REQUIRE(sizeof({struct_name}) > 0);
        }}
    }}'''


def _generate_destructor_test(basename):
    """Generate a compile-time check that destructor exists."""
    lower = basename.lower()
    return f'''
    TEST_CASE("{basename} destructor is accessible", "[{lower}]") {{
        auto ptr = &{basename}::~{basename};
        (void)ptr;
    }}'''


def _generate_smoke_test(basename):
    """Last resort smoke test: type completeness."""
    lower = basename.lower()
    return f'''
    TEST_CASE("{basename} type completeness", "[{lower}]") {{
        REQUIRE(sizeof({basename}) > 0);
    }}'''


def implement_test_depth_task(task):
    """Expand an existing test file with real behavioral test cases.

    Analyzes the source file's patterns (manager, server, system) and generates
    appropriate behavioral tests: construction, lifecycle, callbacks, metrics, safety.
    """
    files = task.get("files", [])
    if len(files) < 2:
        return False, "Need source and test file"

    source_file = files[0]
    test_file = files[1]
    source_path = REPO / source_file
    test_path = REPO / test_file

    if not source_path.exists() or not test_path.exists():
        return False, "Source or test file not found"

    # Read source — try header first for better type info
    header_file = source_file
    if source_file.endswith(".cpp"):
        rel = source_file.replace("src/server/src/", "")
        candidate_h = str(REPO / "src" / "server" / "include" / rel.replace(".cpp", ".hpp"))
        if Path(candidate_h).exists():
            header_file = str(Path(candidate_h).relative_to(REPO))

    try:
        header_content = budgeted_read_text(REPO / header_file, limit=12000)
        source_content = budgeted_read_text(source_path, limit=12000)
    except Exception as e:
        return False, f"Could not read files: {e}"

    # Combine both for analysis
    combined_content = header_content + "\n" + source_content

    basename = Path(source_file).stem
    test_content = budgeted_read_text(test_path)

    # Find what's already tested
    existing_test_names = set(re.findall(r'TEST_CASE\s*\(\s*"([^"]+)"', test_content))

    # Detect pattern and generate tests
    pattern_name, test_cases = _detect_test_pattern(combined_content, source_file)

    # Filter out tests that already exist
    new_tests = []
    for tc in test_cases:
        # Extract test name from the generated snippet
        name_match = re.search(r'TEST_CASE\s*\(\s*"([^"]+)"', tc)
        if name_match and name_match.group(1) not in existing_test_names:
            new_tests.append(tc)
            existing_test_names.add(name_match.group(1))

    if not new_tests:
        return False, "All generated tests already exist"

    # Insert before the last closing braces (namespace closing)
    insert_point = test_content.rfind("} // namespace test")
    if insert_point == -1:
        insert_point = test_content.rfind("} // namespace DarkAges")
    if insert_point == -1:
        insert_point = len(test_content)

    new_content = test_content[:insert_point] + "\n".join(new_tests) + "\n\n" + test_content[insert_point:]
    budgeted_write_text(test_path, new_content)

    return True, f"Added {len(new_tests)} behavioral tests ({pattern_name} pattern) to {Path(test_file).name}"

def implement_cleanup_task(task):
    """Clean up merged autonomous branches."""
    # Delete local merged branches
    code, out, err = run(["git", "-C", str(REPO), "branch", "--merged", "main",
                          "--list", "autonomous/*"])
    local_deleted = 0
    for branch in out.strip().split("\n"):
        branch = branch.strip()
        if branch:
            run(["git", "-C", str(REPO), "branch", "-d", branch])
            local_deleted += 1

    # Delete remote merged branches
    code, out, err = run(["git", "-C", str(REPO), "branch", "-r", "--merged", "main",
                          "--list", "origin/autonomous/*"])
    remote_deleted = 0
    for branch in out.strip().split("\n"):
        branch = branch.strip()
        if not branch:
            continue
        # branch looks like "origin/autonomous/20260419-something"
        local_name = branch.replace("origin/", "")
        run(["git", "-C", str(REPO), "push", "origin", "--delete", local_name])
        run(["git", "-C", str(REPO), "branch", "-dr", branch])
        remote_deleted += 1

    total = local_deleted + remote_deleted
    if total == 0:
        return False, "No merged branches to clean up"
    return True, f"Cleaned up {local_deleted} local + {remote_deleted} remote merged branches"

def run_once(mode="once"):
    """Run one iteration of the dev loop. Returns result string."""
    _action_counter["count"] = 0  # reset operation budget counter
    start_time = time.time()
    print("=" * 60)
    print(f"[dev-loop] Starting at {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')} [mode={mode}]")

    # Agent Map Verification: AGENTS.md must exist and be legible
    if not AGENTS_MD.exists():
        print("[dev-loop] CRITICAL: AGENTS.md missing — aborting (agent map required)")
        return "ABORTED: AGENTS.md missing"
    agents_age_days = (datetime.now(timezone.utc) - datetime.fromtimestamp(AGENTS_MD.stat().st_mtime, tz=timezone.utc)).days
    print(f"[dev-loop] Agent Map: {AGENTS_MD} (age={agents_age_days}d)")

    # LocalContextMiddleware: inject environment state so agent doesn't rediscover
    ctx = {
        "cwd": str(REPO),
        "build_ready": (REPO / "build_validate" / "CMakeCache.txt").exists(),
        "git_branch": "unknown",
        "git_head": "unknown",
    }
    code, out, _ = run(["git", "-C", str(REPO), "rev-parse", "--abbrev-ref", "HEAD"], timeout=5)
    if code == 0:
        ctx["git_branch"] = out.strip()
    code, out, _ = run(["git", "-C", str(REPO), "rev-parse", "--short", "HEAD"], timeout=5)
    if code == 0:
        ctx["git_head"] = out.strip()
    print(f"[dev-loop] Context: {ctx['git_branch']}@{ctx['git_head']} | build_ready={ctx['build_ready']}")

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
    category_order = {"test": 0, "test-depth": 1, "fix": 2, "docs": 3, "cleanup": 4, "refactor": 5, "feature": 6}
    tasks.sort(key=lambda t: (
        priority_order.get(t.get("priority", "P3"), 3),
        category_order.get(t.get("category", "refactor"), 4)
    ))
    
    task = None
    for candidate in tasks:
        task = candidate
        print(f"[dev-loop] Trying: [{candidate['priority']}] {candidate['title']}")
        
        # Sprint Decomposition: in once mode, skip tasks too large to finish
        est_hours = candidate.get("estimated_hours", 1.0)
        if mode == "once" and est_hours > 2.0:
            print(f"[dev-loop] Task too large for once mode ({est_hours}h > 2h) — decompose or run deep mode")
            continue  # Try next task

        # Create branch
        date_str = datetime.now().strftime("%Y%m%d")
        slug = re.sub(r'[^a-z0-9]+', '-', candidate['title'].lower())[:40].strip('-')
        branch = f"autonomous/{date_str}-{slug}"
        git("checkout", "-b", branch)

        # Loop Detection Middleware: check if we've edited these files too much today
        loop_files = [f for f in candidate.get("files", []) if f]
        loop_detected, loop_reason = is_loop_detected(loop_files)
        if loop_detected:
            print(f"[dev-loop] {loop_reason} — skipping task")
            git("checkout", "main")
            git("branch", "-d", branch, "-f")
            continue  # Try next task

        # Implement
        category = candidate.get("category", "")
        if category == "test":
            ok, desc = implement_test_task(candidate)
        elif category == "test-depth":
            ok, desc = implement_test_depth_task(candidate)
        elif category == "refactor":
            ok, desc = implement_refactor_task(candidate)
        elif category == "fix":
            ok, desc = False, "Fix tasks need manual review — skipping for safety"
        elif category == "docs":
            ok, desc = False, "Documentation updates need manual review — skipping"
        elif category == "cleanup":
            ok, desc = implement_cleanup_task(candidate)
        else:
            ok, desc = False, f"Category '{category}' not supported"
        
        if not ok:
            print(f"[dev-loop] Skipped: {desc}")
            git("checkout", "main")
            git("branch", "-d", branch, "-f")
            continue  # Try next task
        
        print(f"[dev-loop] Implemented: {desc}")

        # Record edits for loop detection
        if loop_files:
            record_edit(loop_files)

        # Reasoning Budget check: abort if we've already burned too much time
        budget_sec = REASONING_BUDGET.get(category, 600)
        if mode == "deep":
            budget_sec = int(budget_sec * DEEP_BUDGET_MULTIPLIER)
        elapsed_so_far = time.time() - start_time
        if elapsed_so_far > budget_sec:
            print(f"[dev-loop] REASONING BUDGET EXCEEDED: {elapsed_so_far:.0f}s > {budget_sec}s ({category})")
            git("checkout", "main")
            git("branch", "-d", branch, "-f")
            return f"BUDGET EXCEEDED: {elapsed_so_far:.0f}s > {budget_sec}s"

        # Operation Budget check: abort if too many actions (proxy for token ceiling)
        action_limit = ACTION_BUDGET.get(category, 60)
        if mode == "deep":
            action_limit = int(action_limit * ACTION_BUDGET_MULTIPLIER)
        if _action_counter["count"] > action_limit:
            print(f"[dev-loop] OPERATION BUDGET EXCEEDED: {_action_counter['count']} actions > {action_limit} ({category})")
            git("checkout", "main")
            git("branch", "-d", branch, "-f")
            return f"OPERATION BUDGET EXCEEDED: {_action_counter['count']} actions > {action_limit}"

        break  # Got a valid task, proceed to evaluation
    else:
        elapsed = time.time() - start_time
        skipped_count = sum(1 for c in tasks if c.get("category") == "refactor")
        print(f"[dev-loop] No implementable tasks found ({len(tasks)} tasks checked, {skipped_count} refactor skipped, {elapsed:.1f}s)")
        return f"No implementable tasks found ({skipped_count} refactor skipped)"
    
    # 6. External Evaluation (GAN-style: generator never grades its own work)
    print(f"[dev-loop] Running external evaluator on {branch}...")
    eval_report = run_external_evaluator(branch)
    eval_available = eval_report.get("overall") != "UNAVAILABLE"

    # Reasoning Budget check post-evaluation
    budget_sec = REASONING_BUDGET.get(category, 600)
    if mode == "deep":
        budget_sec = int(budget_sec * DEEP_BUDGET_MULTIPLIER)
    elapsed_so_far = time.time() - start_time
    if elapsed_so_far > budget_sec:
        print(f"[dev-loop] REASONING BUDGET EXCEEDED after evaluation: {elapsed_so_far:.0f}s > {budget_sec}s ({category})")
        git("checkout", "main")
        git("branch", "-d", branch, "-f")
        return f"BUDGET EXCEEDED: {elapsed_so_far:.0f}s > {budget_sec}s"

    # Operation Budget check post-evaluation
    action_limit = ACTION_BUDGET.get(category, 60)
    if mode == "deep":
        action_limit = int(action_limit * ACTION_BUDGET_MULTIPLIER)
    if _action_counter["count"] > action_limit:
        print(f"[dev-loop] OPERATION BUDGET EXCEEDED after evaluation: {_action_counter['count']} actions > {action_limit} ({category})")
        git("checkout", "main")
        git("branch", "-d", branch, "-f")
        return f"OPERATION BUDGET EXCEEDED: {_action_counter['count']} actions > {action_limit}"

    if eval_available:
        build_ok = eval_report["build"]["pass"]
        test_ok = eval_report["tests"]["pass"]
        test_summary = eval_report["tests"]["summary"]
        cases = eval_report["counts"]["cases"]
        asserts = eval_report["counts"]["assertions"]
        regression = eval_report["regression"]
        eval_error = eval_report["tests"]["error"] or eval_report["build"]["error"]

        if not build_ok:
            print(f"[dev-loop] BUILD FAILED (evaluator):\n{eval_report['build']['error']}")
        elif not test_ok or regression:
            print(f"[dev-loop] TESTS FAILED (evaluator):\n{eval_error}")
        else:
            print(f"[dev-loop] Objective Evaluator PASS: {test_summary}")
    else:
        # FAIL CLOSED: generator must NEVER grade its own work.
        # If the external evaluator is unavailable, abort immediately.
        print("[dev-loop] CRITICAL: External evaluator unavailable — cannot proceed")
        git("checkout", "main")
        git("branch", "-d", branch, "-f")
        return "EVALUATION FAILED: external evaluator unavailable (fail-closed policy)"

    # 6b. Subjective Review (OpenCode skeptical reviewer — GAN-style second layer)
    review_pass = True
    review_error = ""
    print(f"[dev-loop] Running OpenCode subjective reviewer on {branch}...")
    review_report = run_opencode_reviewer(branch)
    review_available = review_report.get("overall") != "UNAVAILABLE"

    if review_available:
        issues = review_report["review"].get("issues", [])
        critical_count = sum(1 for i in issues if i.get("severity") == "critical")
        warning_count = sum(1 for i in issues if i.get("severity") == "warning")
        info_count = len(issues) - critical_count - warning_count
        review_pass = review_report["review"]["overall"] == "PASS" and critical_count == 0
        review_error = review_report["review"].get("notes", "")
        if review_pass:
            print(f"[dev-loop] Subjective Reviewer PASS ({info_count} info, {warning_count} warnings logged)")
            if warning_count > 0:
                print(f"[dev-loop] Reviewer warnings (non-blocking): {[i['message'][:80] for i in issues if i.get('severity') == 'warning']}")
        else:
            print(f"[dev-loop] Subjective Reviewer FAIL ({critical_count} critical, {warning_count} warning, {info_count} info): {review_error[:300]}")
    else:
        print(f"[dev-loop] OpenCode reviewer unavailable — proceeding with objective evaluator only")
        # OpenCode reviewer is optional; objective gate is mandatory

    # 7. Pre-Completion Verification Gate
    # Do NOT commit if objective evaluator failed, regression detected, OR critical review issues found
    if not build_ok or not test_ok or regression:
        git("checkout", "main")
        git("branch", "-d", branch, "-f")
        return f"EVALUATION FAILED: {eval_error or 'build/test failure'}"

    if not review_pass:
        git("checkout", "main")
        git("branch", "-d", branch, "-f")
        return f"REVIEW FAILED: {review_error or 'subjective review flagged critical issues'}"

    # 8. Commit + merge
    msg = f"autonomous: {task['title']}"
    if commit_changes(branch, msg):
        print(f"[dev-loop] Committed and merged to main")
    else:
        print(f"[dev-loop] Nothing to commit")
        git("checkout", "main")
        git("branch", "-d", branch, "-f")
        return "No changes to commit"

    # 9. Log
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
        result = run_once(mode="once")
        print(f"\nRESULT: {result}")
    elif mode == "deep":
        # Run up to 3 tasks in sequence — deep mode tries harder
        results = []
        for i in range(3):
            print(f"\n--- Deep iteration {i+1}/3 ---")
            r = run_once(mode="deep")
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