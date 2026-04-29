# OpenHands Usage Patterns for DarkAges
Quick commands and invocation patterns

---

## Direct Skill Invocation

Converted OpenHands skills are executable Python scripts in `~/.hermes/skills/scripts/` (or symlinked from `openhands-adaptation/skills/`).

```bash
# Security audit (check for secrets, CVEs)
python3 ~/.hermes/skills/scripts/security-audit.py \
    --path /root/projects/DarkAges \
    [--strict] [--fail-on HIGH]

# Docker demo harness control
python3 ~/.hermes/skills/scripts/docker-manage.py start all
python3 ~/.hermes/skills/scripts/docker-manage.py status server
python3 ~/.hermes/skills/scripts/docker-manage.py logs client -f

# Testing pipeline (test selection, flakiness detection)
python3 ~/.hermes/skills/scripts/testing-pipeline.py \
    --changed  # run tests affected by git diff
python3 ~/.hermes/skills/scripts/testing-pipeline.py \
    --bisect ./test/combat  # identify flaky tests
```

---

## Hermes Delegation Pattern

OpenHands content is integrated into Hermes agents. When you delegate a task to a domain agent, that agent can invoke skills as tools.

```
User: Review security of NetworkManager.cpp
Hermes: delegates to security-specialist agent (if exists)
      OR delegates to engine-programmer who calls:
          execute_code("security-audit.py --path src/network --fail-on HIGH")
```

---

## Gate-Check Integration

`gate-check.py` runs pre-merge validation. OpenHands-derived skills can be added to its pipeline:

```python
# Inside gate-check.py
result = execute_code("security-audit.py --path . --json")
if result['status'] != 'clean':
    abort_merge("Security audit failed: {findings}")
```

---

## Fallback Pattern

If OpenHands service is unavailable ( Docker not installed ), skills degrade gracefully:
- security-audit.py: still runs grep-based scans (slower)
- docker-manage.py: skip if docker not accessible
- testing-pipeline.py: always available (no deps)

Guard with:
```python
import shutil
if shutil.which('docker') is None:
    print("WARNING: Docker not installed; docker-manage disabled")
    sys.exit(0)  # no-op fallback
```

---

## Repository Setup (One-Time)

After cloning DarkAges:
```bash
# Bootstrap .openhands/ if missing (already done in this adaptation)
mkdir -p .openhands/microagents
cp openhands-adaptation/bootstrap/setup.sh .openhands/
cp openhands-adaptation/bootstrap/microagents/*.md .openhands/microagents/
chmod +x .openhands/setup.sh
./.openhands/setup.sh
```

This installs pre-commit hooks and prepares environment.

---

## Upstream Content Sync

Keep OpenHands skill content up-to-date:
```bash
# Monthly refresh
cd /tmp
git clone --depth 1 https://github.com/OpenHands/OpenHands.git oh-temp
rsync -av --delete oh-temp/skills/ openhands-adaptation/reference/
rm -rf oh-temp
echo "Upstream skills refreshed. Review changes and update converted scripts if needed."
```

---

## Service Control (If Deployed)

```bash
# Start OpenHands service (optional)
docker compose -f /opt/openhands/docker-compose.yml up -d

# Check health
curl -s http://localhost:3000/health | jq .

# View logs
docker compose -f /opt/openhands/docker-compose.yml logs -f openhands

# Stop
docker compose -f /opt/openhands/docker-compose.yml down
```

Note: The service is NOT required for skill scripts to work.

---

## Integration Checklist

- [ ] Skill scripts copied to `~/.hermes/skills/scripts/` and executable
- [ ] `security-audit.py --dry-run` passes
- [ ] `docker-manage.py status` connects to Docker daemon
- [ ] `.openhands/setup.sh` has been run
- [ ] Microagents readable by Hermes agents (path known)
- [ ] (Optional) OpenHands service responds on port 3000

---

End of usage patterns. Next: implementation checklist.
