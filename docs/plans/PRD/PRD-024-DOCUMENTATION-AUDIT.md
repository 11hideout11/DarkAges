# PRD-024: Documentation Audit & Drift Correction

**Version:** 1.0
**Status:** 🔄 Not Started — Inconsistent docs (PROJECT_ISSUES_TRACKER.md Issue #10)
**Owner:** DOCUMENTATION_AGENT
**Priority:** MEDIUM (P3 — Maintainability)
**Dependencies:** All PRDs (documentation depends on actual implementation state)

---

## 1. Overview

### 1.1 Purpose
Resolve **systemic documentation drift** across the DarkAges codebase where:
- Project status documents contradict each other (PROJECT_STATUS.md vs AGENTS.md)
- Implementation details in docs don't match actual code
- Phase documentation missing for Phases 1-5
- PRD status fields not updated after completion
- README files out of date

### 1.2 Scope
All documentation in `docs/`, root README files, and AGENTS.md:
- Project status documents (PROJECT_STATUS.md, AGENTS.md)
- Phase summary files (*_SUMMARY.md)
- PRD completion status
- README files at repo root and subdirectories
- API documentation in `docs/api/` (if exists)
- Skill reference docs

**EXCLUDED:** Inline code comments (Doxygen-style) — separate effort.

---

## 2. Documentation Inventory

### 2.1 Master Document List

| Category | Files | Status |
|----------|-------|--------|
| **Project Status** | PROJECT_STATUS.md, AGENTS.md, NEXT_AGENT_PROMPT.md | ⚠️ Contradiction: PROJECT_STATUS claims MVP Ready; AGENTS.md says NOT READY |
| **Phase Docs** | PHASE0_SUMMARY.md, PHASE6_*.md, PHASE8_*.md | ⚠️ Missing PHASE1-5 summaries |
| **Planning** | All PRDs in docs/plans/PRD/ | ⚠️ Status fields stale |
| **Implementation** | tools/demo/, src/server/include/ | ⚠️ API docs missing |
| **Research** | Research/ThirdPersonCombatStandardsResearch/ | ✅ Complete (mapped) |

### 2.2 Document Drift Examples

**Example 1 — MVP Ready Claim:**
```
PROJECT_STATUS.md line 1: "Project is NOW FULLY DEMO READY"
AGENTS.md line 86: "Project is NOT ready for MVP under updated criteria"

Resolution: AGENTS.md is authoritative. PROJECT_STATUS.md outdated.
```

**Example 2 — PRD Completion Status:**
```
AGENTS.md Table:
  PRD-001 SERVER CORE: COMPLETE
  PRD-002 NETWORKING: COMPLETE
  PRD-003 COMBAT: COMPLETE
  ... up to PRD-007

PRD directory has PRD-001 through PRD-024 now.
But PRD-008 through PRD-023 are marked "In Progress" in file header.
Are they actually implemented? NO — just planned.

Resolution: Mark unimplemented PRDs as "PLANNED" or "NOT STARTED", only "COMPLETE" if done.
```

**Example 3 — Missing Phase Summaries:**
```
Phases 1-5: zero documentation files.
AGENTS.md calls them "UNVERIFIED".
MASTER_TASK_TRACKER.md has placeholder checkboxes.

Resolution: Create summary docs (already in PRD-013).
```

---

## 3. Requirements

### 3.1 Audit Requirements
ID     | Requirement                          | Priority | Method
-------|--------------------------------------|----------|-------
DOC-001 | Resolve PROJECT_STATUS vs AGENTS.md contradiction | P0 | File comparison + sign-off from project lead
DOC-002 | Audit all PRD status fields          | P0       | Verify each file's "Status:" line matches actual completion
DOC-003 | Create missing phase summaries       | P0       | PRD-013 will handle this
DOC-004 | Standardize document metadata        | P1       | Add YAML frontmatter (status, owner, last-updated) to all docs
DOC-005 | Validate hyperlinks (no broken links)| P1       | Link checker across docs/
DOC-006 | Update README files                 | P2       | Root README and subdir READMEs
DOC-007 | Document SoA (Architecture Decision Records) | P2 | ADR folder for major decisions

---

## 4. Current State Assessment

### 4.1 Phase Documentation Matrix

| Phase | Summary File | Evidence | Verdict |
|-------|--------------|----------|---------|
| 0 | PHASE0_SUMMARY.md ✓ | Code + tests | DOCUMENTED |
| 1 | MISSING | Code exists but untraced | UNVERIFIED → Needs PRD-013 |
| 2 | MISSING | Code exists (multi-client) but undoc'd | UNVERIFIED |
| 3 | MISSING | Code exists (input/movement) | UNVERIFIED |
| 4 | MISSING | Code exists (latency simulation) | UNVERIFIED |
| 5 | MISSING | Code exists (NPC replication) | UNVERIFIED |
| 6 | PHASE6_COMPLETION_SUMMARY.md ✓ | Evidence present | DOCUMENTED |
| 7 | Not listed | Tests exist (phase7 tests?) | UNVERIFIED (No summary) |
| 8 | PHASE8_* (multi) ✓ | Documentation complete | DOCUMENTED |
| 9 | Not listed | Phase 9 test exists | Needs summary doc |
| 10 | Not listed | Phase 10 test exists | Needs summary doc |

**Gap:** Phases 1-5, 7, 9, 10 summary docs missing.

---

## 5. Document Standardization

### 5.1 YAML Frontmatter Template

Add to EVERY markdown file in docs/:
```yaml
---
title: "Document Title"
status: "In Progress"  # Complete | In Progress | Draft | Deprecated
created: "2026-01-15"
updated: "2026-05-01"
authors: ["Hermes Agent"]
maintainers: ["COMBAT_AGENT"]
---
```

**Implementation:** Batch script to add frontmatter to existing docs.

### 5.2 Link Validation

Run link checker:
```bash
# Simple Python script
python tools/doc/link_checker.py docs/
# Checks for:
# - [text](path) links where target file doesn't exist
# - Broken intra-doc anchors (#section-name that doesn't exist)
```

Fix all broken links before merge.

---

## 6. Action Plan (by PRD number)

### 6.1 PRD Completion Audit (Cross-PRD Task)

Create `docs/plans/PRD/PRD-IMPLEMENTATION-STATUS.md`:
| PRD | File Status | Actual Implementation | Gap | Action |
|-----|-------------|---------------------|-----|--------|
| PRD-001 | COMPLETE | Server core implemented | None | ✓ |
| PRD-002 | COMPLETE | Networking done | None | ✓ |
| PRD-003 | COMPLETE | Combat done | None | ✓ |
| PRD-004 | COMPLETE | Sharding done? Check code | Maybe | Investigate |
| PRD-005 | COMPLETE | Client work done | None? | Verify |
| PRD-006 | COMPLETE | Infra complete | None | ✓ |
| PRD-007 | COMPLETE | Testing framework done | None | ✓ |
| PRD-008 | PLANNED | CombatStateMachine NOT built | Gap | Implement |
| PRD-009 | PLANNED | Zones exist but not validated | Gap | Validate |
| ... | ... | ... | ... | ... |

**Audit process:** For each PRD, grep codebase for key deliverables, mark actual status.

### 6.2 PRD Status Update Script

`tools/maintenance/update_prd_status.py`:
```python
import re, glob

prds = glob.glob("docs/plans/PRD/PRD-*.md")
for prd in prds:
    with open(prd) as f:
        content = f.read()
    # Check Deliverables section for file existence
    # Auto-update Status: line based on file presence
    # (Conservative: only flag COMPLETE if all files exist)
```

---

## 7. Phase Summary Creation (Handled by PRD-013)

**Defer to PRD-013.** This task:
- Creates PHASE{1,2,3,4,5}_SUMMARY.md
- Verifies actual code presence
- Documents gaps
- Updates MASTER_TASK_TRACKER.md

**PRD-013 is responsible.** Document here that this is delegated.

---

## 8. AGENTS.md Validation

**Current inconsistency:**
- AGENTS.md says "Project is NOT ready for MVP under updated criteria"
- But AGENTS.md also has Recent Commits claiming all those features fixed

**Action:**
1. Do NOT change AGENTS.md top-level MVP readiness claim (keep "NOT READY")
2. Update Recent Commits to accurately reflect implemented features only
3. Verify each "completed" item corresponds to actual PRD completion

---

## 9. Deliverables from This PRD

### 9.1 Created Documents

- `docs/DOCUMENTATION_AUDIT_2026-05-01.md` — this audit report
- `docs/plans/PRD/IMPLEMENTATION_MATRIX.md` — PRD vs code verification table
- `docs/CONTRIBUTING.md` update with documentation standards (frontmatter, link rules)

### 9.2 Tools

- `tools/doc/link_checker.py` — validates internal links
- `tools/doc/prd_status_updater.py` — batch update PRD status fields
- `docs/STYLEGUIDE.md` — writing conventions for future docs

### 9.3 Fixed Documents

- PROJECT_STATUS.md — updated to reflect actual state (NOT READY)
- AGENTS.md — verified Recent Commits accuracy
- All PRDs (008-024) — status fields corrected to "PLANNED" or "IN PROGRESS"
- README.md (root) — links to current docs

---

## 10. Acceptance Criteria

✅ **Quality Gate**
- Zero broken internal links (link checker passes)
- All documents have YAML frontmatter
- PRD status fields match documented implementation status
- PROJECT_STATUS.md consistent with AGENTS.md (both say "NOT READY")

✅ **Completeness**
- Phase 1-5 summary documents created by PRD-013
- IMPLEMENTATION_MATRIX.md accurately tracks all PRDs (008-024)
- No "UNVERIFIED" labels remaining after PRD-013 completes

✅ **Process**
- Contributing guide updated with doc standards
- CI includes documentation lint step (optional, future)

---

## 11. Risk Notes

⚠️ **Risk:** Documentation audit reveals many more gaps
- **Mitigation:** Create separate PRDs for any newly discovered gaps

⚠️ **Risk:** Cannot resolve AGENTS.md vs PROJECT_STATUS.md contradiction
- **Mitigation:** Escalate to project lead for final call; default to AGENTS.md

⚠️ **Risk:** Documentation updates cause merge conflicts with active PRs
- **Mitigation:** Scope audit to docs/ not code/; co-ordinate with CODE freeze if needed

---

## 12. Related Work

- **PRD-013** — handles Phase 1-5 summary creation
- **PRD-024** (this file) — handles metadata, link integrity, PRD status
- Future PRD (post-audit) — any newly discovered gaps

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to DOCUMENTATION_AGENT (after PRD-013 completes phase summaries)
