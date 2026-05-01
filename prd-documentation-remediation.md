# PRD: Documentation Remediation
## Introduction
Multiple documentation files contain inconsistent information, outdated numbers, and contradictory statements about demo readiness. This creates confusion for developers and stakeholders about project state.
**Problem Statement:** Documentation drift is causing contradictory statements in AGENTS.md, PROJECT_STATUS.md, and other files, making it difficult to determine actual project state.
---

## Goals
- Achieve consistent documentation across all files
- Update outdated numbers to current values
- Resolve contradictory statements
- Establish documentation update workflow
- Create single source of truth
---

## User Stories
### US-001: Audit Documentation
**Description:** As a technical writer, I need to audit all docs so I can identify inconsistencies.
**Acceptance Criteria:**
- [ ] All .md files listed and categorized
- [ ] Inconsistencies identified and catalogued
- [ ] Test counts verified against build
- [ ] Build configuration verified
- [ ] Gap analysis documented

### US-002: Update PROJECT_STATUS.md
**Description:** As a stakeholder, I need accurate project status so I can make decisions.
**Acceptance Criteria:**
- [ ] All test counts updated to current values
- [ ] Demo readiness status resolved
- [ ] Phase completion status verified
- [ ] Build configuration accurate
- [ ] GNS status documented

### US-003: Update AGENTS.md
**Description:** As a developer, I need accurate agent context so I can contribute effectively.
**Acceptance Criteria:**
- [ ] Recent commits section current
- [ ] State section reflects reality
- [ ] Phase status accurate
- [ ] Test metrics updated
- [ ] Gaps section maintained

### US-004: Create Documentation Workflow
**Description:** As a project lead, I need documentation standards so drift doesn't recur.
**Acceptance Criteria:**
- [ ] Documentation update checklist created
- [ ] Update frequency documented
- [ ] Single source of truth identified
- [ ] PR review checklist updated
- [ ] Staleness automated detection
---

## Functional Requirements
- FR-1: Create comprehensive doc audit script
- FR-2: Verify all test counts against actual runs
- FR-3: Update PROJECT_STATUS.md with verified values
- FR-4: Update AGENTS.md with current state
- FR-5: Resolve Phase 1-5 documentation gaps
- FR-6: Create docs/CURRENT_STATUS.md template
- FR-7: Add doc linting to CI pipeline
- FR-8: Document update runbook
- FR-9: Remove or mark obsolete files
- FR-10: Create doc index with last-verified dates
---

## Non-Goals
- No rewriting for style consistency alone
- No new documentation formats
- No removal of historical context
- No external documentation hosting
---

## Success Metrics
- 100% of .md files with current dates
- No contradictory statements in active docs
- All test counts match actual test runs
- Build configuration consistent across docs
- Automated doc linting passing
---

## Open Questions
1. Which file is the single source of truth?
2. What is the update frequency (weekly, per-PR)?
3. Should stale docs trigger PR blocks?
4. Is documentation owner assigned?
---