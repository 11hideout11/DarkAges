#!/usr/bin/env python3
"""
CI-friendly validation script.
Exit 0 if all tests pass, exit 1 if any fail.

Usage:
    python3 validate_ci.py              # Run demo then validate
    python3 validate_ci.py --no-demo   # Just validate existing logs
"""

import argparse
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path("/root/projects/DarkAges")

def main():
    parser = argparse.ArgumentParser(description="CI Validation")
    parser.add_argument('--no-demo', action='store_true', help='Skip demo run')
    args = parser.parse_args()
    
    if not args.no_demo:
        # Run demo
        result = subprocess.run(
            [sys.executable, str(PROJECT_ROOT / "tools/demo/full_demo.py"), "--quick"],
            cwd=str(PROJECT_ROOT),
        )
        if result.returncode != 0:
            print(f"Demo failed with code {result.returncode}")
            return 1
    
    # Run validation - rely on exit code
    result = subprocess.run(
        [sys.executable, str(PROJECT_ROOT / "tools/analysis/unified_analysis.py"), 
         "--validate"],
        cwd=str(PROJECT_ROOT),
    )
    
    if result.returncode == 0:
        print("✅ CI Validation PASSED")
        return 0
    else:
        print("❌ CI Validation FAILED")
        return 1

if __name__ == '__main__':
    sys.exit(main())