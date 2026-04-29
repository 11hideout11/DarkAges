# Hermes + CCGS Adaptation Index
## For DarkAges MMO Project

**Date Generated:** 2026-04-29  
**Source Template:** https://github.com/Donchitos/Claude-Code-Game-Studios  
**Target Engine:** Godot 4.2.4 (C# client) / C++20 EnTT ECS server  
**Adaptation Target:** Hermes Agent System  

---

## Quick Navigation

| Document | Purpose | Read When |
|----------|---------|-----------|
| [CCGS_HERMES_ADAPTATION_REVIEW.md](CCGS_HERMES_ADAPTATION_REVIEW.md) | Full technical analysis | First read — understand scope & architecture |
| [HERMES_IMPLEMENTATION_PLAN.md](HERMES_IMPLEMENTATION_PLAN.md) | Step-by-step rollout plan | During setup — follow day-by-day |
| [AGENT_ROLE_MAPPING.md](AGENT_ROLE_MAPPING.md) | Which agent for which task | Daily reference during development |
| [ADAPTATION_MANIFEST.md](ADAPTATION_MANIFEST.md) | Checklist: done vs pending | Track progress, verify completion |

| Utility | Purpose | Usage |
|---------|---------|-------|
| [`.hermes/utils/delegate.py`](.hermes/utils/delegate.py) | Agent loader + delegation wrapper | `delegate_to('game-designer', goal, context)` or `team_work('team-combat', feature)` |
| [`.hermes/skills/scripts/code-review.py`](.hermes/skills/scripts/code-review.py) | Code review automation | Execute with target file path |
| [`.hermes/skills/scripts/gate-check.py`](.hermes/skills/scripts/gate-check.py) | Pre-merge quality gate | Execute on branch before PR |

| Hook | Purpose | Trigger |
|------|---------|--------|
| [`.git/hooks/commit-msg`](.git/hooks/commit-msg) | Validate staged files | `git commit` |
| [`.git/hooks/pre-push`](.git/hooks/pre-push) | Build + test before push | `git push` |

| Reference | Purpose |
|-----------|---------|
| [docs/engine-reference/godot/](docs/engine-reference/godot/) | Godot 4.2 best practices, deprecations, breaking changes |

---

## What Was Done

### Analyzed
- Entire CCGS template repository: 49 agents, 72 skills, 12 hooks, 11 rules, 39 templates
- DarkAges existing framework: autonomous loop, custom skills, evaluation system
- Hermes toolset: capabilities, gaps compared to Claude Code

### Created
- **4 documentation** files explaining the adaptation in detail
- **3 executable utilities** (delegation wrapper, code review, gate check)
- **2 Git hooks** (replacing Claude Code's lifecycle hooks)
- **Godot 4.2 engine reference** (8 files: VERSION, best practices, deprecated, breaking changes, 4 modules)

### Still Pending (User Action Required)

1. **Import CCGS agent personas** (49 .md files):
   ```bash
   cp -r /tmp/claude-code-game-studios/.claude/agents/* ~/.hermes/skills/agents/
   ```

2. **Copy templates & rules**:
   ```bash
   cp -r /tmp/claude-code-game-studios/.claude/docs/templates docs/
   cp /tmp/claude-code-game-studios/.claude/rules/*.md docs/rules/
   ```

3. **Customize core agents** with DarkAges-specific context:
   - godot-csharp-specialist.md
   - engine-programmer.md
   - gameplay-programmer.md
   - godot-shader-specialist.md
   - lead-programmer.md
   - game-designer.md
   - qa-tester.md
   
   *See HERMES_IMPLEMENTATION_PLAN.md Phase 1 step 3 for exact additions.*

4. **Test infrastructure**:
   ```bash
   python3 ~/.hermes/utils/delegate.py list-agents
   python3 ~/.hermes/skills/scripts/code-review.py src/server/combat/CombatSystem.cpp
   git add test.md && git commit -m test
   ```

5. **Update AGENTS.md** with Hermes integration section

6. **Convert additional skills** as needed (convert reference docs to executable scripts)

---

## Three-Page Summary

### Page 1: Why This Matters

**Problem:** DarkAges has robust autonomous infrastructure but lacks:
- Standardized agent roles with clear responsibilities
- Structured quality gates (design review, code review, architecture review)
- Documented workflows and templates for design docs, ADRs, sprint planning
- Professional studio coordination patterns

**Solution:** CCGS provides all of the above as content. Hermes provides execution. Together:
- DarkAges autonomous loop continues (task discovery → branch → build → test → PR)
- Augmented with CCGS agents for design, implementation, review
- Quality gates enforced via Git hooks + gate-check skill
- Consistent documentation via templates and ADR process
- Onboarding simplified via agent role definitions

**Value:** Production-grade process without sacrificing autonomy.

---

### Page 2: Architecture at a Glance

```
User (Hermes Session)
    │
    ├─ Delegates to agents (load persona from ~/.hermes/skills/agents/<agent>.md)
    │   ├─ Direct: "delegate to gameplay-programmer: implement X"
    │   └─ Team: "run team-combat for Y" → spawns 7 agents in parallel
    │
    ├─ Invokes skills (executable scripts in ~/.hermes/skills/scripts/)
    │   ├─ code-review.py → lead-programmer agent's wisdom applied
    │   ├─ gate-check.py → build+test+review+arch before merge
    │   ├─── (more as converted)
    │
    ├─ Git hooks run automatically:
    │   ├─ commit-msg validates design sections, JSON, hardcoded values
    │   └─ pre-push runs build + tests
    │
    └─ Consults engine reference (docs/engine-reference/godot/ 4.2 pinned)
        before any Godot API suggestion

Existing DarkAges autonomous loop (cron jobs) runs in parallel,
creating feature branches and PRs. CCGS agents enhance implementation
quality and add human-style review gates.
```

---

### Page 3: First Week Plan

**Day 1 (2 hours):**
1. Copy agent files (49) → `~/.hermes/skills/agents/`
2. Copy templates + rules → `docs/`
3. Customize 7 core agents with DarkAges context
4. Create Godot 4.2 engine reference (already done)
5. Validate Git hooks work

**Day 2 (4 hours):**
6. Test delegation wrapper: `delegate_to('game-designer', ...)`
7. Test code-review skill on real file
8. Test gate-check skill on ready branch
9. Convert team-combat orchestration to Python script
10. Document in AGENTS.md

**Day 3-5 (on-demand):**
11. Convert additional skills as needed by current work
12. Integrate agent_hint into discover_tasks.py (autonomous loop)
13. Run full cycle: design → review → impl → test → gate → merge
14. Fix issues, refine prompts, add more agent customization

**Week 2:**
15. Expand agent set (use all 49 as needed)
16. Maintain fork: track upstream CCGS changes, reapply custom patches

---

## Files Inventory

### Documentation (Project Root)
- `CCGS_HERMES_ADAPTATION_REVIEW.md` — Full analysis (sections 1–19, ~40KB)
- `HERMES_IMPLEMENTATION_PLAN.md` — Rollout steps with code snippets
- `AGENT_ROLE_MAPPING.md` — Quick lookup table
- `ADAPTATION_MANIFEST.md` — Todo list

### Code (Hermes)
- `.hermes/utils/delegate.py` — Wrapper: `load_agent()`, `delegate_to()`, `team_work()`
- `.hermes/skills/scripts/code-review.py` — Skill script (executable)
- `.hermes/skills/scripts/gate-check.py` — Skill script (executable)

### Git Hooks
- `.git/hooks/commit-msg` — Pre-commit validation
- `.git/hooks/pre-push` — Build+test before push

### Engine Reference (Godot 4.2)
- `docs/engine-reference/godot/VERSION.md`
- `docs/engine-reference/godot/current-best-practices.md`
- `docs/engine-reference/godot/deprecated-apis.md`
- `docs/engine-reference/godot/breaking-changes.md`
- `docs/engine-reference/godot/modules/animation.md`
- `docs/engine-reference/godot/modules/physics.md`
- `docs/engine-reference/godot/modules/rendering.md`
- `docs/engine-reference/godot/modules/networking.md`

---

## Key Decisions Record

| Decision | Rationale |
|----------|-----------|
| Pin Godot to 4.2, not 4.6 | DarkAges uses 4.2.4; SkeletonModifier3D IK, AgX, D3D12 not available |
| Use Git hooks, not Claude Code hooks | Hermes has no lifecycle hook integration; Git hooks work universally |
| Hybrid skill conversion (not full 72) | Convert high-value skills first; others as reference docs |
| Layer on existing autonomous loop | DarkAges loop already excellent; CCGS augments, doesn't replace |
| Keep agent count flexible | Start with 7 core agents; expand to 49 as needed per task |
| Model tiering hardcoded per agent | Simpler than dynamic tier selection; aligns with CCGS intent |

---

## Learning Resources

**Inside this adaptation:**
- Read `CCGS_HERMES_ADAPTATION_REVIEW.md` Part 1–3 for architecture understanding
- Study `delegate.py` to see how agent personas are loaded and prompts constructed
- Examine `code-review.py` for skill conversion pattern (CCGS procedure → Python script)
- Compare a CCGS agent .md file (from `/tmp/claude-code-game-studios/.claude/agents/`) with `delegate.py` prompt building

**External (CCGS template):**
- Repository: https://github.com/Donchitos/Claude-Code-Game-Studios
- Read `CLAUDE.md` (master config) and `docs/COLLABORATIVE-DESIGN-PRINCIPLE.md`
- Browse `.claude/skills/` for 72 skill procedures
- Browse `.claude/agents/` for 49 role definitions

**DarkAges context:**
- `AGENTS.md` — current agent map + build/test commands
- `AUTONOMOUS_ITERATION_FRAMEWORK.md` — existing autonomous loop
- `AI_COORDINATION_PROTOCOL.md` — custom coordination rules

---

## Support & Updates

**When upstream CCGS updates:**
1. Pull latest from https://github.com/Donchitos/Claude-Code-Game-Studios
2. Diff changes to `.claude/agents/`, `.claude/skills/`, `.claude/rules/`
3. Merge new agents/skills selectively
4. Re-apply customizations (Godot 4.2 pinning, DarkAges context blocks)
5. Document divergence in `PATCHES/` directory

**When DarkAges evolves:**
- Update agent customizations as architecture changes
- Add new agents if new roles emerge (e.g., analytics-engineer for metrics)
- Convert new skills as workflows stabilize
- Update engine reference if upgrading beyond Godot 4.2

---

**Index Document** — Generated 2026-04-29  
**Status:** Blueprint complete, ready for Phase 1 execution  
**Next action:** Run the copy commands above to import agent files and templates.
