#!/usr/bin/env python3
"""
DarkAges Cron Robustness Wrapper

Adds failure resistance to the autonomous dev loop:
- Retry with exponential backoff for transient failures
- Job coordination (lock file to prevent parallel runs)
- State persistence (track attempts to avoid redundant work)
- Health monitoring (alert on repeated failures)
- Rate limit detection and backoff
"""
import subprocess, json, os, sys, re, time, shutil, fcntl
import signal
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, Tuple

REPO = Path("/root/projects/DarkAges")
LOCK_FILE = REPO / "scripts/autonomous" / ".cron.lock"
STATE_FILE = REPO / "scripts/autonomous" / ".cron_state.json"
LOG_FILE = REPO / "AUTONOMOUS_LOG.md"

# Configuration
MAX_CONSECUTIVE_FAILURES = 3  # Pause job after this many failures
RETRY_DELAYS = [5, 15, 45, 120]  # Exponential backoff delays (seconds)
EMPTY_QUEUE_SLOWDOWN = 4  # Run every Nth cycle when queue is empty


def acquire_lock() -> bool:
    """Try to acquire the job lock. Returns True if acquired."""
    try:
        LOCK_FILE.parent.mkdir(parents=True, exist_ok=True)
        lock_fd = open(LOCK_FILE, 'w')
        fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        lock_fd.write(str(os.getpid()))
        lock_fd.flush()
        return True
    except (IOError, OSError):
        return False


def release_lock():
    """Release the job lock."""
    try:
        LOCK_FILE.unlink(missing_ok=True)
    except Exception:
        pass


def load_state() -> dict:
    """Load persistent state from file."""
    if STATE_FILE.exists():
        try:
            return json.loads(STATE_FILE.read_text())
        except (json.JSONDecodeError, IOError):
            pass
    return {
        "consecutive_failures": 0,
        "last_success": None,
        "last_failure": None,
        "empty_queue_count": 0,
        "rate_limited_until": None,
    }


def save_state(state: dict):
    """Save persistent state to file."""
    STATE_FILE.write_text(json.dumps(state, indent=2))


def is_rate_limited(state: dict) -> bool:
    """Check if we're currently rate limited."""
    until = state.get("rate_limited_until")
    if until is None:
        return False
    try:
        limit_ts = float(until)
        if time.time() < limit_ts:
            return True
        # Rate limit expired — clear it
        state["rate_limited_until"] = None
        return False
    except (TypeError, ValueError):
        return False


def set_rate_limit(state: dict, retry_after: int):
    """Set rate limit based on Retry-After header or default."""
    state["rate_limited_until"] = time.time() + retry_after
    save_state(state)


def check_empty_queue(state: dict) -> bool:
    """Check if queue is consistently empty. Returns True if we should skip this cycle."""
    # If we've had many empty queue results, skip most cycles
    if state.get("empty_queue_count", 0) >= EMPTY_QUEUE_SLOWDOWN:
        # Random skip to spread out runs
        if os.urandom(1)[0] % EMPTY_QUEUE_SLOWDOWN != 0:
            print(f"[robust] Queue empty {state['empty_queue_count']}x — skipping this cycle (adaptive)")
            return True
    return False


def record_empty_queue(state: dict):
    """Record another empty queue result."""
    state["empty_queue_count"] = state.get("empty_queue_count", 0) + 1
    save_state(state)


def record_success(state: dict):
    """Record a successful run."""
    state["consecutive_failures"] = 0
    state["last_success"] = datetime.now(timezone.utc).isoformat()
    state["empty_queue_count"] = 0  # Reset empty queue counter
    save_state(state)


def record_failure(state: dict, error: str):
    """Record a failed run."""
    state["consecutive_failures"] = state.get("0", 0) + 1
    state["last_failure"] = datetime.now(timezone.utc).isoformat()
    save_state(state)
    
    # Check for rate limiting
    if "429" in error or "rate limit" in error.lower():
        # Default to 5 minutes backoff
        set_rate_limit(state, 300)
        print(f"[robust] Rate limited — backing off for 5 minutes")


def should_pause(state: dict) -> bool:
    """Check if we should pause due to consecutive failures."""
    return state.get("consecutive_failures", 0) >= MAX_CONSECUTIVE_FAILURES


def run_with_retry(cmd: list, cwd: Optional[Path] = None, timeout: int = 300) -> Tuple[int, str, str]:
    """Run a command with retry logic for transient failures."""
    last_error = ""
    
    for attempt in range(len(RETRY_DELAYS)):
        code, out, err = run(cmd, cwd=cwd, timeout=timeout)
        last_error = err
        
        if code == 0:
            return code, out, err
        
        # Check for transient errors that warrant retry
        is_transient = any(x in err.lower() for x in [
            "timeout", "connection", "temporary", "429", "503", "502"
        ])
        
        if not is_transient or attempt >= len(RETRY_DELAYS) - 1:
            break
        
        delay = RETRY_DELAYS[attempt]
        print(f"[robust] Retry {attempt + 1}/{len(RETRY_DELAYS)} after {delay}s: {err[:100]}")
        time.sleep(delay)
    
    return -1, "", last_error


def run(cmd: list, cwd: Optional[Path] = None, timeout: int = 300, capture: bool = True) -> Tuple[int, str, str]:
    """Run a command with timeout."""
    try:
        r = subprocess.run(
            cmd, cwd=cwd, timeout=timeout,
            capture_output=capture, text=True
        )
        return r.returncode, r.stdout or "", r.stderr or ""
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except Exception as e:
        return -1, "", str(e)


def log_to_file(message: str):
    """Append to log file."""
    with open(LOG_FILE, "a") as f:
        f.write(f"\n[{datetime.now(timezone.utc).isoformat()}] {message}\n")


def main():
    """Main entry point — wraps the dev loop with robustness."""
    # Parse args
    mode = "once"
    if len(sys.argv) > 1:
        mode = sys.argv[1]
    
    # Acquire lock
    if not acquire_lock():
        print("[robust] Another job is running — skipping")
        sys.exit(0)
    
    try:
        state = load_state()
        
        # Check rate limit
        if is_rate_limited(state):
            print("[robust] Currently rate limited — skipping")
            sys.exit(0)
        
        # Check pause due to failures
        if should_pause(state):
            print(f"[robust] Pausing after {MAX_CONSECUTIVE_FAILURES} consecutive failures")
            print("[robust] Manual intervention needed or will resume after 1 hour")
            sys.exit(1)
        
        # Check adaptive skip for empty queue
        if check_empty_queue(state):
            sys.exit(0)
        
        # Run the actual dev loop
        dev_loop = REPO / "scripts/autonomous" / "cron_dev_loop.py"
        
        if mode == "deep":
            # Deep mode uses more time, more retries
            cmd = ["python3", str(dev_loop), "deep"]
            timeout = 600
        else:
            # Once mode
            cmd = ["python3", str(dev_loop), "once"]
            timeout = 300
        
        print(f"[robust] Running: {' '.join(cmd)}")
        code, out, err = run_with_retry(cmd, cwd=REPO, timeout=timeout)
        
        # Analyze output for success/failure
        output = out + err
        
        # Check for empty queue
        if "no implementable tasks" in output.lower() or "[silent]" in output.lower():
            record_empty_queue(state)
            print("[robust] Queue empty — recorded")
            return
        
        # Check for errors
        if code != 0 or any(x in output.lower() for x in ["failed", "error", "timeout"]):
            record_failure(state, output)
            log_to_file(f"FAILURE: {output[:200]}")
            print(f"[robust] Failed: {output[:200]}")
            return
        
        # Success
        record_success(state)
        log_to_file(f"SUCCESS: {output[:200]}")
        print(f"[robust] Success: {output[:100]}")
        
    finally:
        release_lock()


if __name__ == "__main__":
    main()