# PRD: Client C# Warnings Resolution

## Introduction

The Godot 4.2 C# client build produces 208 warnings, all related to CS8618 (non-nullable field without initializer). While these don't prevent compilation, they indicate code quality issues and may hide real problems. This PRD specifies resolving these warnings to achieve a clean build.

## Goals

- Reduce client build warnings from 208 to 0
- Fix all CS8618 non-nullable field patterns
- Maintain null-safety semantics (no crashes from null refs)
- Document and handle edge cases where null initialization makes sense

## User Stories

### US-001: Code Audit
**Description:** As a developer, I need to understand which files have warnings so I can prioritize fixes.

**Acceptance Criteria:**
- [ ] Build output shows warnings organized by file
- [ ] Files with warnings identified: SaveManager.cs, UITheme.cs, HitEffect.cs, CombatParticleSystem.cs, FootIKController.cs, NPCManager.cs
- [ ] Each file's warning count documented

### US-002: Field Initialization Fixes
**Description:** As a developer, I want to initialize non-nullable fields so warnings are resolved.

**Acceptance Criteria:**
- [ ] Each [SerializeField] field has explicit initializer or null-coalescing assignment
- [ ] Non-nullable reference types replaced with nullable (?) where null is valid
- [ ] OnReady fields check for null before use (null-conditional or early return)

### US-003: Null Checks
**Description:** As a developer, I want to add null guards where needed so code doesn't crash.

**Acceptance Criteria:**
- [ ] All GetNode calls check for null before use
- [ ] Signals only emitted if targets connected
- [ ] External dependencies documented with null-handling strategy

### US-004: Build Verification
**Description:** As a QA, I want to verify warnings are resolved so the build is clean.

**Acceptance Criteria:**
- [ ] Client build completes with 0 warnings
- [ ] All existing functionality still works
- [ ] No regressions in unit tests

## Functional Requirements

- FR-1: Fix CS8618 in SaveManager.cs (DirAccess API migration fields)
- FR-2: Fix CS8618 in UITheme.cs (StyleBoxFlat properties, label fields)
- FR-3: Fix CS8618 in HitEffect.cs (particle system null handling)
- FR-4: Fix CS8618 in CombatParticleSystem.cs (Vector3.Y property)
- FR-5: Fix CS8618 in FootIKController.cs (CombatState enum qualified)
- FR-6: Fix CS8618 in NPCManager.cs (dead code removed)
- FR-7: Audit additional client C# files for consistency

## Non-Goals

- No API changes to game functionality
- No new features or content
- No changes to server C++ code
- No Godot version upgrade (stay on 4.2.4)

## Technical Considerations

- Use `= null!` for fields that are assigned in `_Ready()` but compiler can't track
- Use `= default` for value types that can't be null
- Use `[MaybeNull]` attribute for return types that may be null
- Document rationale in code comments for edge cases

## Success Metrics

- Client build: 0 warnings, 0 errors
- All 208 CS8618 warnings resolved
- Build time unchanged (no significant performance impact)

## Open Questions

- Should we adopt nullable reference types project-wide?
- Should we enable "strict" null checks in project settings?
- Any fields that genuinely can't be initialized safely?

---

*Generated: 2026-05-03*