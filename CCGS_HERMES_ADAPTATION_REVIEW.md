# CCGS Template -> Hermes Adaptation Review
## Claude Code Game Studios for DarkAges (Godot 4.2 C#)

**Date:** 2026-04-29
**Template Repo:** https://github.com/Donchitos/Claude-Code-Game-Studios
**Target Project:** DarkAges MMO (C++20 server / Godot 4.2 C# client)
**Adaptation Agent:** Hermes (Nous -> NVIDIA -> OpenAI fallback routing)

---

## PART 1: TEMPLATE ARCHITECTURE ANALYSIS

### 1.1 Directory Structure Overview

CCGS is organized under .claude/ with agents, skills, hooks, rules, docs.
DarkAges has its own autonomous framework. No .claude/ exists yet.

Key subdirectories:
- .claude/agents/      49 agent definitions (YAML frontmatter + markdown instructions)
- .claude/skills/      72 slash-command skills (one folder per skill with SKILL.md)
- .claude/hooks/       12 lifecycle bash scripts (session-start, validate-commit, etc.)
- .claude/rules/       11 path-scoped coding standards (engine-code.md, gameplay-code.md, ...)
- .claude/docs/        workflow-catalog.yaml, technical-preferences.md, templates/, engine-reference/
- docs/engine-reference/godot/ Godot-specific best practices (currently pinned to 4.6)

### 1.2 How It Works in Claude Code

User types /skill-name -> Claude Code loads SKILL.md -> executes multi-phase procedure
-> may spawn subagents via Task tool -> subagents share session context -> result returned.

Agents follow three-tier hierarchy:
Human Developer
  -> technical-director, producer, creative-director (Tier 1, Opus)
  -> leads: lead-programmer, game-designer, art-director, ... (Tier 2, Sonnet)
  -> specialists: gameplay-programmer, engine-programmer, ... (Tier 3, Sonnet/Haiku)

Hooks fire automatically on session lifecycle events (settings.json controls which scripts run).

---

## PART 2: HERMES CAPABILITIES MATRIX

CCGS Feature                      | Hermes Equivalent           | Gap?
----------------------------------|----------------------------|------
. agents .md persona files | Read file + inject into delegate_task prompt | Content reusable, need loader
. skills SKILL.md procedures | Could convert to skill_view format or follow manually | Skill system differs
/ slash-command invocation | No direct command parser; use natural language | Need NL or wrapper
settings.json hooks (auto) | No lifecycle hooks; use git hooks + cron + manual | Partial workaround
Task subagent (shared context)| delegate_task (isolated sessions) | Must pass context explicitly
Auto model tiering (haiku/sonnet/opus) | Manual per delegate_task | Must encode tier per agent
rule auto-load by path | No auto-load; must manually check | Manual discipline or wrapper
status line injection | No prompt injection; use session-state file | Fine
permissions allow/deny | Tool-level config only | Less granular

VERDICT: Content heavily reusable; integration layer requires reimplementation.

---

## PART 3: GODOT 4.2 vs 4.6 - CRITICAL VERSION PINNING

CCGS template is pinned to Godot 4.6 (as of 2026-02-12 docs).
DarkAges uses Godot 4.2.4.

BREAKING CHANGES that affect template agents:
1) IK SYSTEM: 4.6 restored SkeletonModifier3D nodes (CCIK, FABRIK, etc). 4.2 has SkeletonIK3D or manual. DarkAges Foot IK already implemented in 4.2 style.
2) PHYSICS: 4.6 makes Jolt default. 4.2 uses GodotPhysics3D. DarkAges uses server-side custom physics - minimal client impact.
3) GDSCRIPT: 4.6 adds @abstract, variadic args. DarkAges uses C#, so not affected.
4) RENDERING: 4.6 default D3D12 on Windows, AgX tonemapper. 4.2 uses Vulkan default. Not critical for demo.

ACTION: Create Godot 4.2 engine reference mirroring CCGS structure but with 4.2-specific practices.
- docs/engine-reference/godot/VERSION.md -> pin to 4.2.4
- docs/engine-reference/godot/current-best-practices.md -> 4.2 idioms only
- docs/engine-reference/godot/deprecated-apis.md -> deprecations in 4.2 (not 4.6 ones)
- docs/engine-reference/godot/modules/* -> annotate 4.2 vs 4.6 features
Update all Godot specialist agents to consult these 4.2 docs, not 4.6.

---

## PART 4: AGENT MAPPING FOR HERMES

All 49 agent .md files can be reused as persona templates.

ESSENTIAL AGENTS for DarkAges (start with these 7):
1. gameplay-programmer - Server C++ + client C# implementation
2. engine-programmer - EnTT ECS, build system, networking internals
3. godot-csharp-specialist - Primary for client code (C# Godot 4.2)
4. godot-shader-specialist - Visual effects, shader debugging
5. lead-programmer - Code reviews, standards enforcement
6. game-designer - System design, GDD authoring
7. qa-tester - Test planning and validation

HIGH-VALUE orchestration skills to convert:
- team-combat (designer -> programmer -> AI -> VFX -> audio -> QA parallel)
- team-narrative (narrative director + writer + level-designer)
- team-ui (ux-designer + C# specialist + tech-artist)
- team-polish (tech-artist + sound + performance + QA)

Load pattern:
delegate_task(
  goal = f"""You are {agent_name}. {agent_instructions}
  DarkAges context: Server - C++20 EnTT ECS, 60Hz tick; Client - Godot 4.2 C#.
  Task: {user_goal}
  """,
  model = agent_tier_model[agent_name]  # sonnet or opus
)

---

## PART 5: SKILL SYSTEM ADAPTATION

Best path: Hybrid approach.
- Decision-tree skills (start, brainstorm, gate-check) -> convert to execute_code scripts
- Checklist skills (code-review, architecture-review) -> keep as reference docs, follow manually initially

CONVERT FIRST: code-review, gate-check, architecture-decision, team-combat (orchestration), design-system.

Skill format conversion: CCGS SKILL.md references Task, AskUserQuestion, Write.
Replace with delegate_task, clarify, write_file/patch.

Example: code-review skill becomes Python script that
- reads target file
- determines applicable rule (engine/gameplay/network) from path
- checks ADR references
- outputs review report
- asks user to save

---

## PART 6: GIT HOOK SUBSTITUTION

install:
  commit-msg  <- validate-commit.sh (adapted)
  pre-push     <- validate-push.sh (adapted)
  post-merge   <- validate-assets.sh (optional)

Adaptation changes: CCGS versions read JSON from stdin (Claude Code). Git hooks get args or stdin differently.
Rewrite input parsing to use git-native methods.
Keep all validation logic:
- Design doc required sections
- JSON validity in assets/data/
- Hardcoded gameplay value detection
- Unassigned TODO/FIXME

Place adapted hooks in .git/hooks/ and ensure executable.
