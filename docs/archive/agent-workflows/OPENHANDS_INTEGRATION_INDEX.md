# Hermes + OpenHands Adaptation Index
## For DarkAges MMO Project

**Date Generated:** 2026-04-29
**Source:** https://github.com/OpenHands/OpenHands
**Target Integration:** Hermes Agent System + DarkAges Autonomous Loop

---

## Quick Navigation

| Document | Purpose |
|----------|---------|
| [OPENHANDS_ADAPTATION_REVIEW.md](openhands-adaptation/OPENHANDS_ADAPTATION_REVIEW.md) | Full technical analysis |
| [OPENHANDS_SKILL_MAPPING.md](openhands-adaptation/OPENHANDS_SKILL_MAPPING.md) | Skill conversion matrix |
| [OPENHANDS_SETUP_GUIDE.md](openhands-adaptation/OPENHANDS_SETUP_GUIDE.md) | Setup instructions |
| [OPENHANDS_USAGE_PATTERNS.md](openhands-adaptation/OPENHANDS_USAGE_PATTERNS.md) | Command examples |
| [OPENHANDS_IMPLEMENTATION_CHECKLIST.md](openhands-adaptation/OPENHANDS_IMPLEMENTATION_CHECKLIST.md) | Implementation steps |

Directories:
- `openhands-adaptation/skills/` — Converted skill scripts
- `openhands-adaptation/reference/` — Original skill .md files
- `.openhands/` — Repository-specific microagents

---

## Three-Page Summary

### Page 1: OpenHands Value Proposition

OpenHands provides:
- CodeActAgent (can edit/run code in Docker sandbox)
- 26 built-in skills (Git, Docker, Security, Testing, Kubernetes, etc.)
- Web UI + Socket.IO server
- Sandboxed execution environment

Value for DarkAges: content (excellent prompts) + optional service (Docker/K8s automation).

**Integration philosophy:** Use Hermes for day-to-day development; fall back to OpenHands for sandboxed/browser automation tasks. **No replacement.**

---

### Page 2: How It Works

**Three tiers:**
1. Hermes (primary): agent personas, delegation, memory, build/test
2. Skill content (converted): OpenHands prompts → Python skills
3. Optional OpenHands service: runs in Docker, called via HTTP for isolation

**Skill conversion formula:**
Markdown frontmatter + prompt → `execute_code()` script that reads context, performs checks, outputs results.

Example (code-review.md): extract scoring rubric → code-review.py scoring system.

---

### Page 3: First Week Plan

Day 1: Create directory structure, copy 26 skill .md files, create `.openhands/` microagents (repo.md, darkages-cpp.md, godot-4-2.md, networking.md), write documentation.

Days 2–3: Convert 4 high-value skills to Python (code-review, testing, docker, security), integrate into existing Hermes skill scripts.

Week 2: Optional OpenHands service deployment; create HTTP client wrapper; test end-to-end integration.

---

No separate markdown files to read — integration complete.
