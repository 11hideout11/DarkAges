# DarkAges Agent Role Mapping
## CCGS Template -> Hermes Delegation

Quick reference: which CCGS agent for which task.

TIER 1 - DIRECTORS (Opus):
- technical-director: architecture decisions, ADR conflicts, tech stack choices
- producer: sprint planning, milestone review, scope/risk management
- creative-director: vision/pillar questions, creative direction

TIER 2 - LEADS (Sonnet):
- lead-programmer: code reviews, API design, refactoring strategy, standards
- game-designer: system design, formula balancing, GDD authoring
- art-director: visual style, art bible, shader/CFX approval
- audio-director: sound palette, music style, audio event definitions
- narrative-director: quest structure, character arcs, lore consistency
- qa-lead: test strategy, coverage planning, quality bar

TIER 3 - SPECIALISTS (Sonnet/Haiku):
- gameplay-programmer: server C++ EnTT systems implementation
- engine-programmer: build system, ECS internals, networking layer
- network-programmer: protocol design, packet validation, bandwidth
- tools-programmer: build tools, dev pipelines, editor tooling
- systems-designer: system decomposition, dependency mapping
- level-designer: zone layout, encounter pacing, NavigationMesh
- economy-designer: currency, loot tables, economy balancing
- technical-artist: VFX, shaders, particle systems, materials
- sound-designer: SFX, audio events, mix tuning
- writer: quest text, dialogue, lore, item descriptions
- world-builder: prop placement, environmental storytelling
- ux-designer: input flow, navigation, accessibility, UI mockups
- performance-analyst: profiling, hotpath analysis, memory optimization
- security-engineer: input validation review, anti-cheat patterns
- devops-engineer: Docker, CI/CD, deployment, monitoring
- qa-tester: test plan authoring, validation

ENGINE SPECIALISTS (Godot):
- godot-csharp-specialist: PRIMARY - all C# client code, Godot 4.2 idioms
- godot-shader-specialist: shader programming, visual effects

TEAM ORCHESTRATION (parallel):
- team-combat: designer, programmer, AI, VFX, audio, QA, C# specialist (7 agents)
- team-narrative: narrative director, writer, level designer, world builder
- team-ui: ux designer, C# specialist, technical artist
- team-polish: tech artist, sound designer, performance analyst, QA
- team-audio: sound designer, audio director
- team-level: level designer, world builder, tech artist
- team-qa: QA tester, lead programmer
- team-release: producer, release manager, technical director

USAGE:
User: "delegate to gameplay-programmer: implement crafting"
Hermes: loads agent persona -> delegate_task with proper model (sonnet)

User: "run team-combat for parry system"
Hermes: spawns 7 agents in parallel, synthesizes results

DECISION TREE:
- New system design -> game-designer
- Implement feature -> gameplay-programmer + godot-csharp-specialist
- Client UI -> godot-csharp-specialist (+ ux-designer)
- Visual effect -> technical-artist
- Code review -> lead-programmer
- Architecture decision -> technical-director
- Sprint planning -> producer
- Write tests -> qa-tester
- Performance issue -> performance-analyst
- Zone creation -> team-level
- Combat mechanic -> team-combat
- Quest narrative -> team-narrative
