#!/usr/bin/env python3
"""Docker lifecycle management for DarkAges demo harness."""

import subprocess, sys, argparse, os, time

COMPOSE_FILE = os.path.expanduser("~/projects/DarkAges/docker-compose.yml")
if not os.path.exists(COMPOSE_FILE):
    COMPOSE_FILE = "/root/projects/DarkAges/docker-compose.yml"

SERVICES = ["server", "client", "metrics"]

def run_cmd(cmd, check=False):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(f"ERROR: {result.stderr.strip()}", file=sys.stderr)
        sys.exit(result.returncode)
    return result

def start(service=None):
    if service:
        run_cmd(f"docker compose -f {COMPOSE_FILE} up -d {service}", check=True)
        _wait(service)
    else:
        run_cmd(f"docker compose -f {COMPOSE_FILE} up -d", check=True)
        for s in SERVICES:
            _wait(s, timeout=10)
    print("Services started.")

def stop(service=None):
    target = service if service else "all"
    run_cmd(f"docker compose -f {COMPOSE_FILE} stop {target if service else ''}")

def restart(service):
    run_cmd(f"docker compose -f {COMPOSE_FILE} restart {service}", check=True)
    _wait(service)
    print(f"Restarted {service}.")

def status():
    run_cmd(f"docker compose -f {COMPOSE_FILE} ps")

def logs(service=None, follow=False):
    flag = "-f" if follow else ""
    svc = service or ""
    subprocess.run(f"docker compose -f {COMPOSE_FILE} logs {flag} {svc}", shell=True)

def _wait(service, timeout=30):
    start = time.time()
    while time.time() - start < timeout:
        r = subprocess.run(
            f"docker inspect --format='{{{{.State.Health.Status}}}}' darkages-{service}",
            shell=True, capture_output=True, text=True
        )
        if r.stdout.strip() == "healthy":
            return
        time.sleep(2)
    print(f"WARNING: {service} not healthy after {timeout}s", file=sys.stderr)

def main():
    parser = argparse.ArgumentParser(description="Manage DarkAges demo containers")
    parser.add_argument("action", choices=["start", "stop", "restart", "status", "logs"])
    parser.add_argument("service", nargs="?", help="Service name: server|client|metrics")
    parser.add_argument("-f","--follow", action="store_true", help="Follow logs (with logs action)")
    args = parser.parse_args()

    if args.action == "start": start(args.service)
    elif args.action == "stop": stop(args.service)
    elif args.action == "restart": restart(args.service)
    elif args.action == "status": status()
    elif args.action == "logs": logs(args.service, follow=args.follow)

if __name__ == "__main__":
    main()
