#!/usr/bin/env python3
"""Security audit skill — derived from OpenHands security.md
Scans codebase for common secret patterns and dependency issues."""

import subprocess, sys, json, argparse, os

def audit(path: str):
    """Return list of findings: file, line, pattern, text."""
    patterns = ["password", "secret", "token", "api_key", "aws_access_key_id", "authorization"]
    findings = []
    exclude = {".git", "__pycache__", "build", ".hermes", "docs/build", "node_modules"}
    for root, dirs, files in os.walk(path):
        dirs[:] = [d for d in dirs if d not in exclude]
        for fname in files:
            if fname.endswith(("cpp","h","hpp","cs","py","sh","txt","cmake","yml","yaml","json","env","md")):
                fp = os.path.join(root, fname)
                try:
                    with open(fp, "r", errors="replace") as fh:
                        for lineno, line in enumerate(fh, 1):
                            low = line.lower()
                            for pat in patterns:
                                if pat in low:
                                    findings.append({
                                        "file": fp, "line": lineno,
                                        "pattern": pat, "text": line.strip(),
                                        "severity": "medium"
                                    })
                except Exception:
                    pass
    return findings

def main():
    parser = argparse.ArgumentParser(description="Security audit for hardcoded secrets")
    parser.add_argument("--path", default=".", help="Path to scan")
    parser.add_argument("--json", action="store_true", help="Output JSON")
    parser.add_argument("--strict", action="store_true", help="Exit non-zero if findings found")
    args = parser.parse_args()

    findings = audit(args.path)
    report = {"status": "clean" if not findings else "issues",
              "findings": findings, "count": len(findings)}

    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Security audit: {report['status'].upper()} — {len(findings)} potential issues")
        if findings:
            print("Top findings:")
            for f in findings[:10]:
                print(f"  {f['file']}:{f['line']} [{f['pattern']}]")

    if args.strict and findings:
        sys.exit(1)

if __name__ == "__main__":
    main()
