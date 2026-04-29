# OpenHands Setup Guide
Integrating OpenHands with DarkAges Hermes environment

---

## Overview

OpenHands integration provides two capabilities:
1. Repository-specific guidance via `.openhands/` microagents
2. Optional standalone service (Docker) for sandboxed execution
3. Converted skill scripts that embody OpenHands patterns

This guide shows how to enable each layer.

---

## Prerequisites

- Python 3.12+ (for Hermes)
- Docker (if running OpenHands service)
- Git (already present)
- Optional: `pip-audit`, `git-secrets` for security audits

---

## Step 1: Directory Structure (Auto-created)

Run the bootstrap script — it copies files and creates `.openhands/`:
```bash
# This should already be done by adaptation package
ls .openhands/
# ├── setup.sh
# └── microagents/
#     ├── repo.md
#     ├── darkages-cpp.md
#     ├── godot-4-2.md
#     └── networking.md
```

If missing, copy from `openhands-adaptation/reference/bootstrap/` (future).

---

## Step 2: Microagents Verification

Microagents are loaded by OpenHands automatically when you work in the repo.
You can also read them from Hermes agents for context.

```bash
# Check none of the files are corrupted
for f in .openhands/microagents/*.md; do
    echo "=== $f ==="
    head -1 "$f"  # should be --- yaml frontmatter
done
```

---

## Step 3: Skill Scripts Installation

Converted skill scripts live in `openhands-adaptation/skills/` (or symlinked to `~/.hermes/skills/scripts/`).

Prefer symlink:
```bash
ln -s /root/projects/DarkAges/openhands-adaptation/skills/*.py ~/.hermes/skills/scripts/
chmod +x ~/.hermes/skills/scripts/*.py
```

Or copy directly:
```bash
cp openhands-adaptation/skills/*.py ~/.hermes/skills/scripts/
```

---

## Step 4: Optional — OpenHands Service

Only if you need Docker sandbox isolation or browser automation.

```bash
# Option A: Docker Compose (recommended)
mkdir -p /opt/openhands
cp openhands-adaptation/reference/docker-compose.yml /opt/openhands/
cd /opt/openhands
docker compose up -d

# Verify: curl http://localhost:3000/health
# Note: Web UI available at http://localhost:3000
```

**Configuration:**
```yaml
services:
  openhands:
    image: openhands:latest
    volumes:
      - ~/.openhands:/.openhands
      - /root/projects/DarkAges:/opt/workspace_base
      - /var/run/docker.sock:/var/run/docker.sock  # Docker-in-Docker
    ports:
      - "3000:3000"
```

**Stop:**
```bash
docker compose -f /opt/openhands/docker-compose.yml down
```

**Option B:** Skip entirely. Not needed for skill scripts.

---

## Step 5: Hermes Integration Check

Skills are automatically available to Hermes. Verify:
```bash
# List skill scripts known to Hermes
ls ~/.hermes/skills/scripts/

# Test one
python3 ~/.hermes/skills/scripts/security-audit.py /root/projects/DarkAges --dry-run
```

The gate-check skill (`.git/hooks/pre-push`) may already call these automatically.

---

## Step 6: Automated Triggers

OpenHands keywords activate microagents automatically when using the OpenHands service (if it's running). Hermes does not auto-trigger on keywords; you instead call skill scripts explicitly or delegate task to relevant domain agent.

Example:
```python
# In Hermes session:
execute_code("python3 ~/.hermes/skills/scripts/docker-manage.py status")
```

Or if you asked Hermes to run OpenHands via delegation:
```
User: Start the demo server using Docker
Hermes: (might delegate to devops-specialist) -> calls docker-manage.py start all
```

---

## Step 7: Upstream Updates (Monthly)

Refresh OpenHands skill content:
```bash
cd /tmp
git clone --depth 1 https://github.com/OpenHands/OpenHands.git openhands-latest
rsync -av --delete openhands-latest/skills/ openhands-adaptation/reference/
rm -rf openhands-latest
```

Then manually merge any new skill content into converted scripts where relevant.

---

## Verification Checklist

- [ ] `openhands-adaptation/` directory exists with 26 reference .md files
- [ ] `.openhands/microagents/` contains 4 domain guides
- [ ] `~/.hermes/skills/scripts/` contains at least code-review.py + converted scripts
- [ ] `git status` no unexpected changes
- [ ] (Optional) OpenHands service responds on http://localhost:3000
- [ ] `security-audit.py --help` or `docker-manage.py --help` works

---

## Troubleshooting

**Problem:** `setup.sh: permission denied`
**Fix:** `chmod +x .openhands/setup.sh`

**Problem:** Skill not found
**Fix:** Ensure symlink/copy to `~/.hermes/skills/scripts/` succeeded

**Problem:** OpenHands service fails to start
**Fix:** Check Docker is running (`docker ps`). Verify `docker.sock` mounted if inside container.

**Problem:** "Unknown Godot 4.6 API" warning
**Fix:** All DarkAges microagents pin to 4.2; consult `godot-4-2.md`. If any skill suggests 4.6, correct the source in `openhands-adaptation/reference/` contribution back upstream.

---

**End of setup guide.** Next: `OPENHANDS_USAGE_PATTERNS.md`.
