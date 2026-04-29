# Hermes + OpenHands Adaptation Index
## For DarkAges MMO Project

**Date:** 2026-04-29 | **Source:** https://github.com/OpenHands/OpenHands | **Target:** Hermes + DarkAges

---

## Quick Navigation

| File | Purpose |
|------|---------|
| `openhands-adaptation/OPENHANDS_ADAPTATION_REVIEW.md` | Full technical analysis |
| `openhands-adaptation/OPENHANDS_SKILL_MAPPING.md` | 26-skill conversion matrix |
| `openhands-adaptation/OPENHANDS_SETUP_GUIDE.md` | .openhands/ directory setup |
| `openhands-adaptation/OPENHANDS_USAGE_PATTERNS.md` | Quick commands |
| `openhands-adaptation/OPENHANDS_IMPLEMENTATION_CHECKLIST.md` | Step-by-step todo |

```
openhands-adaptation/
├── skills/          # Converted skill scripts (Python)
└── reference/       # Upstream OpenHands skill .md files
.openhands/
├── setup.sh
└── microagents/
    ├── repo.md
    ├── darkages-cpp.md
    ├── godot-4-2.md
    └── networking.md
```

---

## Three-Page Summary

### Page 1: What & Why

OpenHands = AI-driven development platform (CodeActAgent, Docker sandbox, 26 skills).

**Value for DarkAges:**
- Excellent DevOps content (Git, Docker, Security, Testing patterns)
- Optional sandboxed execution for isolation
- No conflict with existing Hermes/CCGS agents

**Strategy:** Extract skill content → executable scripts; run service only when needed.

---

### Page 2: How It Fits

```
Hermes (primary dev platform) ← reads ← Skill content (converted skills)
       ↓ optional calls
OpenHands Service (Docker, browser automation)
```

- Hermes continues autonomous loop (unchanged)
- OpenHands skills used as deterministic checkers (security-audit, docker-manage)
- OpenHands service only for Playwright browser automation or complex K8s workflows (not currently needed)

---

### Page 3: One Week Plan

Day 1: Done — review document + mapping + directory structure + `.openhands/` microagents
Day 2: Convert testing/docker/security skills; enhance code-review
Day 3: Test all skills on real code; integration validate
Week 2: Optional service deployment; convert remaining skills on-demand

**Total time investment:** 8–12 hours for Phase 1–2. Phase 3+ only if needed.

---

Start with `OPENHANDS_ADAPTATION_REVIEW.md` for deep dive.
