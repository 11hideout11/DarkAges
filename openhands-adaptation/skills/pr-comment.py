#!/usr/bin/env python3
"""
Add comment to a GitHub PR — derived from address_pr_comments.md (scripted variant).
"""

import subprocess, sys, argparse, os, json, re

def run(cmd):
    return subprocess.run(cmd, shell=True, capture_output=True, text=True)

def main():
    parser = argparse.ArgumentParser(description="Add comment to GitHub PR")
    parser.add_argument("--pr", required=True, help="PR number")
    parser.add_argument("--body", required=True, help="Comment body (markdown)")
    parser.add_argument("--json", action="store_true", help="JSON output")
    args = parser.parse_args()

    # Prefer gh
    if subprocess.run(["which", "gh"], capture_output=True).returncode == 0:
        r = subprocess.run(
            ["gh", "pr", "comment", str(args.pr), "--body", args.body],
            capture_output=True, text=True
        )
        if r.returncode == 0:
            report = {"status": "commented", "pr": args.pr}
            print(json.dumps(report) if args.json else f"Comment added to PR #{args.pr}")
            sys.exit(0)
        else:
            print(f"gh error: {r.stderr}", file=sys.stderr)
            sys.exit(1)

    # Fallback: GitHub API
    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        print("ERROR: gh CLI not found and no GITHUB_TOKEN", file=sys.stderr)
        sys.exit(1)

    # Determine owner/repo
    remote = run("git remote get-url origin")
    m = re.search(r'github\.com[:/](.+?)/(.+?)(\.git)?$', remote.stdout)
    if not m:
        print("ERROR: Cannot determine repo", file=sys.stderr)
        sys.exit(1)
    owner, repo = m.group(1), m.group(2)

    api = f"https://api.github.com/repos/{owner}/{repo}/issues/{args.pr}/comments"
    payload = json.dumps({"body": args.body})
    r = subprocess.run(
        ["curl", "-s", "-X", "POST", api,
         "-H", f"Authorization: token {token}",
         "-H", "Accept: application/vnd.github+json",
         "-d", payload],
        capture_output=True, text=True
    )
    try:
        data = json.loads(r.stdout)
    except:
        print(f"API error: {r.stdout}", file=sys.stderr)
        sys.exit(1)

    if 'id' in data:
        report = {"status": "commented", "pr": args.pr, "comment_id": data['id']}
        print(json.dumps(report) if args.json else f"Commented on PR #{args.pr}")
        sys.exit(0)
    else:
        print(f"API: {data.get('message','error')}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
