#!/usr/bin/env python3
"""
One-command demo + analysis pipeline.

Runs the demo then automatically analyzes the results.

Usage:
    python3 run_and_analyze.py              # Full demo (~60s)
    python3 run_and_analyze.py --quick    # Quick demo (~30s)  
    python3 run_and_analyze.py --validate  # Validation only (no demo)
"""

import argparse
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

PROJECT_ROOT = Path("/root/projects/DarkAges")
DEMO_SCRIPT = PROJECT_ROOT / "tools/demo/full_demo.py"
ANALYSIS_SCRIPT = PROJECT_ROOT / "tools/analysis/unified_analysis.py"
ARTIFACTS = PROJECT_ROOT / "tools/demo/artifacts"

def run_demo(quick: bool = False, duration: int = 45, npcs: int = 3) -> tuple[int, float]:
    """Run the demo and return exit code and duration."""
    start = time.time()
    
    cmd = [sys.executable, str(DEMO_SCRIPT)]
    if quick:
        cmd.append("--quick")
    else:
        cmd.extend(["--duration", str(duration), "--npcs", str(npcs)])
    
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=str(PROJECT_ROOT))
    
    duration_sec = time.time() - start
    return result.returncode, duration_sec

def analyze() -> int:
    """Run analysis on latest logs."""
    result = subprocess.run(
        [sys.executable, str(ANALYSIS_SCRIPT), "--validate", "--quiet"],
        cwd=str(PROJECT_ROOT),
        capture_output=True,
        text=True
    )
    
    print(result.stdout)
    if result.stderr:
        print(result.stderr, file=sys.stderr)
    
    return result.returncode

def validate_only() -> int:
    """Just run validation without demo."""
    result = subprocess.run(
        [sys.executable, str(ANALYSIS_SCRIPT), "--validate"],
        cwd=str(PROJECT_ROOT),
        capture_output=True,
        text=True
    )
    
    # Parse output
    print(result.stdout)
    return result.returncode

def main():
    parser = argparse.ArgumentParser(description="Run demo and analyze")
    parser.add_argument('--quick', action='store_true', help='Quick demo')
    parser.add_argument('--duration', type=int, default=45, help='Demo duration')
    parser.add_argument('--npcs', type=int, default=3, help='NPC count')
    parser.add_argument('--validate', action='store_true', help='Just validate existing')
    parser.add_argument('--no-build', action='store_true', help='Skip build')
    
    args = parser.parse_args()
    
    if args.validate:
        print("Validation only...")
        return validate_only()
    
    # Run demo
    print(f"\n{'='*60}")
    print(f"DEMO + ANALYSIS PIPELINE")
    print(f"{'='*60}")
    
    exit_code, duration = run_demo(
        quick=args.quick, 
        duration=args.duration,
        npcs=args.npcs
    )
    
    if exit_code != 0:
        print(f"\n❌ Demo failed (exit code {exit_code})")
        return exit_code
    
    print(f"\n✅ Demo completed in {duration:.1f}s")
    
    # Analyze
    print(f"\n{'='*60}")
    print(f"ANALYSIS")
    print(f"{'='*60}\n")
    
    analysis_exit = analyze()
    
    if analysis_exit == 0:
        print(f"\n✅ ALL VALIDATIONS PASSED")
    else:
        print(f"\n⚠️ Some validations failed")
    
    return analysis_exit

if __name__ == '__main__':
    sys.exit(main())