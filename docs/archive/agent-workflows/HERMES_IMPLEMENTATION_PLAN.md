# IMPLEMENTATION PLAN: Hermes + CCGS Integration for DarkAges

## OVERVIEW

Adapt Claude Code Game Studios template (49 agents, 72 skills) to work with Hermes agents for DarkAges MMO (Godot 4.2 C# client, C++20 server).

Integration approach: Layer CCGS content on top of existing DarkAges autonomous framework - do NOT replace.

---

## PHASE 1: FOUNDATION (Day 1-2)

### Step 1.1: Import Agent Personas

```bash
mkdir -p ~/.hermes/skills/agents
cp -r /tmp/claude-code-game-studios/.claude/agents/* ~/.hermes/skills/agents/
```

Customize core agents - edit these files to add DarkAges context:
- godot-csharp-specialist.md (add: 4.2.4, server-authoritative, client view-only)
- engine-programmer.md (add: EnTT ECS, 60Hz tick, protobuf, DarkAges:: namespace)
- gameplay-programmer.md (add: dual-layer server C++ + client C# pattern)
- godot-shader-specialist.md (pin to Godot 4.2 shading language)
- lead-programmer.md (add: EnTT patterns, DarkAges coding standards)

### Step 1.2: Import Rules & Templates

```bash
cp -r /tmp/claude-code-game-studios/.claude/docs/templates docs/
cp /tmp/claude-code-game-studios/.claude/rules/*.md docs/rules/
```

Update AGENTS.md with Hermes mapping section.

### Step 1.3: Create Godot 4.2 Engine Reference

Create docs/engine-reference/godot/ with:
- VERSION.md (pin to 4.2.4)
- deprecated-apis.md (4.2 deprecations only)
- current-best-practices.md (no 4.5/4.6 features like @abstract, SkeletonModifier3D)
- breaking-changes.md (future upgrade notes)
- modules/ (copy from CCGS but annotate version compatibility)

### Step 1.4: Update AGENTS.md

Add section documenting Hermes integration (agent locations, delegation pattern, skill invocation).

---

## PHASE 2: SKILL SYSTEM (Day 3-5)

Strategy: Hybrid - convert high-value skills, keep others as reference.

Priority skills to convert:
1. code-review.py
2. architecture-decision.py
3. gate-check.py
4. team-combat.py (orchestration template)
5. design-system.py
6. design-review.py
7. estimate.py
8. sprint-plan.py

Location: ~/.hermes/skills/scripts/<skill>.py
Invocation: User says "code review X" -> you call run_skill('code-review', target=X)

Conversion pattern: Read SKILL.md procedure -> implement in Python using hermes_tools (read_file, write_file, clarify, delegate_task).

---

## PHASE 3: GIT HOOKS (Day 6)

Adapt CCGS hook scripts to Git native:

- .git/hooks/commit-msg: from validate-commit.sh (adapt JSON parsing to plain git args)
- .git/hooks/pre-push: from validate-push.sh (run build+test)
- .git/hooks/post-merge: optional asset validation

Test hooks with valid/invalid commits.

---

## PHASE 4: WORKFLOW TEST (Day 7-10)

Run full feature cycle:
design -> review -> architecture-decision -> implementation -> code-review -> gate-check -> merge

Document in docs/HERMES_WORKFLOW_EXAMPLE.md.

---

## PHASE 5: AUTONOMOUS LOOP AUGMENTATION (Week 2)

- Enhance discover_tasks.py to emit agent_hint field
- PR descriptions include "Suggested agents: gameplay-programmer, qa-tester"
- Optional: auto-delegate based on hint (requires Hermes CLI)

---

## PART 17: GODOT 4.2 ENGINE REFERENCE - CONTENT GUIDE

Create docs/engine-reference/godot/:

VERSION.md:
| Engine Version | Godot 4.2.4 |
| LLM Cutoff | May 2025 (covers 4.2, verify) |
| WARNING | Do not use APIs introduced after 4.2 (4.3+: @abstract, variadic; 4.5+: Shader Baker; 4.6+: SkeletonModifier3D, D3D12) |

current-best-practices.md (highlights):
- C#: GetNode<T> cached, ResourceLoader.Load<T>, typed signals
- Animation: SkeletonIK3D for IK (not SkeletonModifier3D), AnimationTree state machines
- Physics: GodotPhysics3D (default), CharacterBody3D, _PhysicsProcess
- Rendering: Vulkan backend, ACESFilmic tonemap, FXAA/TAA
- GDScript: no @abstract, no variadic args

deprecated-apis.md:
- yield() -> await signal
- connect(string) -> callable Connect
- instance() -> instantiate()
- get_world() -> get_world_3d/2d
- OS.* -> Time.* etc.

modules/: Copy CCGS versions but annotate with 4.2 vs 4.5/4.6.

---

## PART 18: DELEGATION WRAPPER

Create ~/.hermes/utils/delegate.py:

```python
def delegate_to(agent_name, goal, context=""):
    agent = load_agent_from_file(agent_name)
    prompt = f"""You are {agent.name}.
{agent.description}

DarkAges Context: Server C++20 EnTT ECS 60Hz; Client Godot 4.2 C#.
{context}

Collaboration Protocol: Question -> Options -> Decision -> Draft -> Approval.
Task: {goal}
"""
    model = agent.model  # from frontmatter or tier default
    return delegate_task(goal=prompt, model=model)

def team_work(team_name, feature, context=""):
    # spawn parallel agents per team definition
    tasks = []
    for agent, subgoal in TEAM_COMPOSITIONS[team_name]:
        tasks.append(delegate_to(agent, subgoal.format(feature=feature), context))
    results = delegate_task(tasks=tasks)
    return synthesize(results)
```

---

## PART 19: AGENT SELECTION CHEAT SHEET

Gameplay feature -> gameplay-programmer + godot-csharp-specialist
Code review -> lead-programmer
Architecture decision -> technical-director
Design doc -> game-designer
Shader effect -> godot-shader-specialist
Tests -> qa-tester
Performance issue -> performance-analyst
Network protocol -> engine-programmer (or network-programmer if exists)
UI screen -> ux-designer + godot-csharp-specialist
Combat system -> team-combat (7 agents)
Narrative quest -> team-narrative
Polish pass -> team-polish
Level/zone -> team-level

---

## PART 20: MAINTENANCE

Keep upstream CCGS clone at /tmp/claude-code-game-studios. Periodically git fetch to get updates. Reapply custom patches when updating:
- Godot 4.2 pinning
- DarkAges context injection (server/client description)
- Skill tool-call adaptation (Task -> delegate_task)

Patch files in patches/ directory with clear application instructions.

---

## NEXT STEPS

Day 1:
1. Copy agents to ~/.hermes/skills/agents/
2. Edit core 5 agents with DarkAges context
3. Create docs/engine-reference/godot/ (4.2 pin)
4. Copy rules + templates
5. Update AGENTS.md with integration section

Day 2:
6. Write delegate.py wrapper
7. Test delegate_to('game-designer', 'propose item rarity tiers')
8. Install Git hooks (adapted)
9. Validate hook behavior

Day 3:
10. Convert code-review skill
11. Test on existing file
12. Convert gate-check
13. Test pre-merge gating

Day 4-5:
14. Convert team-combat orchestration
15. Test with hypothetical feature
16. Document full workflow

Week 2:
17. Hook into autonomous loop (agent_hint)
18. Document complete system
19. Train/prepare for production use

---

RECOMMENDATION: Proceed with Phase 1 immediately. Adaptation is high-value low-risk.
