# PRD: Art Pipeline Implementation
## Introduction
The Art Pipeline research phase is complete with comprehensive specifications in `docs/ART_PIPELINE_*.md` and `assets/manifest.blob`. The critical path now requires implementing the asset creation pipeline, integrating assets into Godot, and achieving demo-ready visuals.
**Problem Statement:** The project has placeholder visuals (Kenney assets) that limit demo quality and user engagement. Full implementation requires 3D models, textures, animations, and VFX integration.
---
## Goals
- Achieve visually appealing demo ready for user demonstrations
- Create production-quality character with locomotion animations
- Implement combat VFX (damage numbers, hit flash, outline shaders)
- Establish asset creation pipeline with automated validation
- Reduce draw calls to <80 per frame at 60fps
- Support 3 zones with distinct visual identity
---
## User Stories
### US-001: Implement Character Creation Pipeline
**Description:** As an artist, I need a validated character creation pipeline so I can create and import assets efficiently.
**Acceptance Criteria:**
- [ ] Blender → GLTF export validated
- [ ] Godot import with correct settings automated
- [ ] Texture resolution limits enforced
- [ ] Animation retargeting documented
- [ ] Toolchain validation scripts functional
### US-002: Create Player Character with Animations
**Description:** As a player, I want to see a quality character with smooth animations so the demo is engaging.
**Acceptance Criteria:**
- [ ] Player male model with PBR textures
- [ ] 6+ locomotion animations (idle, walk, run, sprint, jump, fall)
- [ ] Combat animations (attack, hit reaction, death)
- [ ] AnimationTree integration complete
- [ ] IK foot alignment on slopes
### US-003: Implement Combat VFX Shaders
**Description:** As a player, I want visible combat feedback so combat feels impactful.
**Acceptance Criteria:**
- [ ] Damage numbers floating up from hit targets
- [ ] Hit flash on damage taken
- [ ] Outline shader on targeted enemies
- [ ] Particle effects for abilities
- [ ] Hit stop camera effect functional
### US-004: Create Demo Zone Visuals
**Description:** As a player, I want visually distinct zones so each area feels unique.
**Acceptance Criteria:**
- [ ] Zone98: Tutorial environment with clear paths
- [ ] Zone99: Combat arena with modular kit placement
- [ ] Zone100: Boss arena with atmospheric lighting
- [ ] Zone loading <5 seconds
- [ ] Draw call budget enforced per zone
---
## Functional Requirements
- FR-1: Install and configure Blender + Git LFS
- FR-2: Create asset validation scripts
- FR-3: Model player character (15K tris, 4 PBR maps)
- FR-4: Rig character with SkeletonIK3D support
- FR-5: Create locomotion + combat animation set
- FR-6: Implement VFX shaders (damage, outline, dissolve)
- FR-7: Build 3 demo zones from modular kit
- FR-8: Integrate assets into Godot scenes
- FR-9: Enforce draw call budgets
- FR-10: Implement texture streaming
---
## Non-Goals
- No real-time shadows (performance)
- No procedural world generation
- No full open world streaming (MVP scope)
- No mobile platform support
- No custom shader tool development
---
## Technical Considerations
### Production Strategy (12-week sprint)
- **Month 1:** Toolchain validation → Character foundation
- **Month 2:** Weapons & Combat VFX
- **Month 3:** Worlds & Polishing
### Parallel Tracks
- **Track A:** Characters (model → textures → rig → animations)
- **Track B:** Weapons (models → hitbox collisions)
- **Track C:** Environment (modular kit → zone assembly)
- **Track D:** VFX (shaders → particles → integration)
---
## Open Questions
1. Is full character customization planned?
2. Should motion capture be used for animations?
3. What is the budget for asset production hours?
4. Should NPCs share rigs with player?
---
*Filename: `prd-art-pipeline-implementation.md`*
*Created: 2026-05-01*