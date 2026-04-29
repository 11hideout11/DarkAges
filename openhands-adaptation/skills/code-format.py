#!/usr/bin/env python3
"""
Code formatting enforcement — fixes long lines for C++/C# projects.
Uses clang-format for C++ headers/sources and dotnet format for C#.
Skips third-party and build directories.
"""

import subprocess, sys, argparse, os, re

EXCLUDE = {'.git', '__pycache__', 'build', 'build_validate', '.hermes', 'deps', 'node_modules', 'dist', 'bin', 'obj'}

def find_files(root, exts):
    files = []
    for dirpath, dirs, filenames in os.walk(root):
        # prune
        dirs[:] = [d for d in dirs if d not in EXCLUDE]
        for fn in filenames:
            if any(fn.endswith(ext) for ext in exts):
                files.append(os.path.join(dirpath, fn))
    return files

def run_clang_format(files, check=False):
    if not files:
        return True, "No C++ files to format"
    cmd = ["clang-format", "--style=file"]
    if check:
        cmd.append("--dry-run")
    else:
        cmd.append("-i")
    cmd.extend(files)
    r = subprocess.run(cmd, capture_output=True, text=True)
    ok = r.returncode == 0
    return ok, r.stdout + r.stderr

def run_dotnet_format(files, check=False):
    # dotnet format works on project/solution files or directories
    # Find .csproj files in the file set's directories
    dirs = sorted(set(os.path.dirname(f) for f in files))
    all_ok = True
    output = []
    for d in dirs:
        cmd = ["dotnet", "format", d]
        if check:
            cmd.append("--verify-no-changes")
        else:
            cmd.extend(["--fix-whitespace", "--fix-style", "error"])
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            all_ok = False
        output.append(r.stdout + r.stderr)
    return all_ok, "\n".join(output)

def main():
    parser = argparse.ArgumentParser(description="Enforce code formatting for DarkAges (C++/C#)")
    parser.add_argument("--check", action="store_true", help="Check only, do not modify")
    parser.add_argument("--json", action="store_true", help="Output JSON")
    parser.add_argument("--path", default=".", help="Root path to scan")
    args = parser.parse_args()

    cpp_files = find_files(args.path, ['.cpp', '.hpp', '.h', '.cc'])
    cs_files = find_files(args.path, ['.cs'])

    cpp_ok, cpp_out = run_clang_format(cpp_files, check=args.check)
    cs_ok, cs_out = run_dotnet_format(cs_files, check=args.check)

    report = {
        "cpp": {"files": len(cpp_files), "ok": cpp_ok},
        "csharp": {"files": len(cs_files), "ok": cs_ok},
        "overall": cpp_ok and cs_ok,
    }

    if args.json:
        print(json.dumps(report))
    else:
        print(f"C++ files: {len(cpp_files)}  {'OK' if cpp_ok else 'NEEDS FORMAT'}")
        print(f"C# files: {len(cs_files)}  {'OK' if cs_ok else 'NEEDS FORMAT'}")
        if cpp_out.strip():
            print("clang-format:", cpp_out[:500])
        if cs_out.strip():
            print("dotnet format:", cs_out[:500])
        if not report['overall']:
            print("\nRun without --check to auto-fix.")
            sys.exit(1)

if __name__ == "__main__":
    main()
