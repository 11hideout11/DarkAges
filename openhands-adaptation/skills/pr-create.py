#!/usr/bin/env python3
"""
PR creation skill — derived from OpenHands github.md + create_pr tool concept.
Creates a GitHub PR from current branch using gh CLI or GitHub API.
"""

import subprocess, sys, argparse, os, json, re

def run_cmd(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result

def main():
    parser = argparse.ArgumentParser(description="Create GitHub PR from current branch")
    parser.add_argument("--title", required=True, help="PR title")
    parser.add_argument("--body", default="", help="PR description")
    parser.add_argument("--base", default="main", help="Target branch")
    parser.add_argument("--draft", action="store_true", help="Create draft PR")
    parser.add_argument("--json", action="store_true", help="JSON output")
    args = parser.parse_args()

    # Get current branch
    branch_res = run_cmd("git branch --show-current")
    branch = branch_res.stdout.strip()
    if not branch:
        print("ERROR: Cannot determine current branch", file=sys.stderr)
        sys.exit(1)

    # Check origin remote
    remote_res = run_cmd("git remote get-url origin")
    if remote_res.returncode != 0:
        print("ERROR: No 'origin' remote", file=sys.stderr)
        sys.exit(1)
    remote_url = remote_res.stdout.strip()

    # Try gh CLI
    if subprocess.run(["which", "gh"], capture_output=True).returncode == 0:
        cmd = ["gh", "pr", "create", "--title", args.title,
               "--body", args.body, "--base", args.base, "--head", branch]
        if args.draft:
            cmd.append("--draft")
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            url = result.stdout.strip().split('\n')[-1]
            report = {"status": "created", "url": url, "branch": branch}
            print(json.dumps(report) if args.json else f"PR created: {url}")
            sys.exit(0)
        else:
            print(f"gh error: {result.stderr}", file=sys.stderr)
            sys.exit(1)

    # Fallback to GitHub API
    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        print("ERROR: Install gh CLI or set GITHUB_TOKEN", file=sys.stderr)
        sys.exit(1)

    m = re.search(r'github\.com[:/](.+?)/(.+?)(\.git)?$', remote_url)
    if not m:
        print(f"ERROR: Cannot parse repo from {remote_url}", file=sys.stderr)
        sys.exit(1)
    owner, repo = m.group(1), m.group(2)

    diff_res = run_cmd(f"git diff {args.base}...{branch}")
    diff = diff_res.stdout

    api_url = f"https://api.github.com/repos/{owner}/{repo}/pulls"
    payload = json.dumps({
        "title": args.title, "body": args.body,
        "head": branch, "base": args.base, "draft": args.draft
    })
    result = subprocess.run(
        ["curl", "-s", "-X", "POST", api_url,
         "-H", f"Authorization: token {token}",
         "-H", "Accept: application/vnd.github+json",
         "-d", payload],
        capture_output=True, text=True
    )
    try:
        data = json.loads(result.stdout)
    except:
        print(f"API error: {result.stdout}", file=sys.stderr)
        sys.exit(1)

    if 'number' in data:
        report = {"status": "created", "url": data['html_url'], "number": data['number']}
        print(json.dumps(report) if args.json else f"PR #{data['number']}: {data['html_url']}")
        sys.exit(0)
    else:
        print(f"API: {data.get('message','error')}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
