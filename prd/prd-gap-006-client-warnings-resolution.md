# PRD-GAP-006: Client Build Warnings Resolution

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P1 - Medium  
**Category:** Build - Client Quality

---

## Introduction

The client C# code builds successfully with 0 errors, BUT there are 208 warnings - all CS8618 (non-nullable property pattern). While these don't block the build, they indicate potential null reference issues and reduce signal-to-noise ratio for real warnings.

**Problem:** Properties in Godot 4.2 have non-nullable annotations but no initializers, causing CS8618 warnings. These should be fixed for code quality and to catch real issues.

---

## Goals

- Reduce C# warnings from 208 to 0
- Fix all CS8618 patterns properly
- Maintain null-safety semantics
- No functionality regression

---

## User Stories

### US-001: Null-Safe Property Patterns
**Description:** As a developer, I want properties to have null-safe patterns.

**Acceptance Criteria:**
- [ ] All properties either initialized or marked nullable
- [ ] No CS8618 warnings on build
- [ ] Properties initialized inline or in _Ready()

### US-002: Optional Pattern Cleanup
**Description:** As a developer, I want unused optionals handled.

**Acceptance Criteria:**
- [ ] Unused [Export] props either used or removed
- [ ] Unused Node references freed or commented
- [ ] Dead code removed

---

## Technical Considerations

- **Current State:**
  - 208 CS8618 warnings
  - All are non-nullable property patterns
  - Zero build errors

- **Files Affected:**
  - UITheme.cs
  - HitEffect.cs
  - CombatParticleSystem.cs
  - FootIKController.cs
  - NPCManager.cs
  - SaveManager.cs
  - And others in client/

- **Fix Patterns:**
  ```csharp
  // Before (warning)
  [Export] private Label _titleLabel;
  
  // After (no warning) - initialize
  [Export] private Label _titleLabel = null;
  
  // OR mark nullable
  [Export] private Label? _titleLabel;
  
  // OR initialize in _Ready
  [Export] private Label _titleLabel = null;
  public override void _Ready() => _titleLabel = GetNode<Label>("TitleLabel");
  ```

---

## Success Metrics

- **Build:**
  - Zero C# warnings
  - Zero C# errors
  - Clean build output

- **Quality:**
  - All null-safety patterns correct
  - No potential NREs in code