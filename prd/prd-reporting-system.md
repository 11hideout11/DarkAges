# PRD: Reporting & Moderation System

## Introduction

DarkAges has no player reporting or moderation system. Toxic behavior cannot be reported, and griefers cannot be moderated. This PRD implements a basic reporting system.

## Goals

- Report toxic players
- Report reasons (spam, harassment, cheating)
- Optional evidence capture
- Admin review queue

## User Stories

### RPT-001: Report Player
**Description:** As a player, I want to report bad behavior.

**Acceptance Criteria:**
- [ ] Right-click → Report option
- [ ] Select reason
- [ ] Optional comment
- [ ] Submit confirmation

### RPT-002: Admin Queue
**Description:** As an admin, I want to review reports.

**Acceptance Criteria:**
- [ ] Report queue view
- [ ] Player history
- [ ] Action: warn/ban/kick
- [ ] Close report

### RPT-003: Auto-Reports
**Description:** As a system, I want to automate some reports.

**Acceptance Criteria:**
- [ ] Spam detection flag
- [ ] Death loop detection
- [ ] Speed hack auto-flag

## Functional Requirements

- FR-1: Report packet
- FR-2: Report database table
- FR-3: Admin panel (future)

## Non-Goals

- Live admin spectate
- Auto-ban system
- Appeals system

## Technical Considerations

### Report Database
```sql
CREATE TABLE reports (
    report_id uuid,
    reporter_id bigint,
    reported_id bigint,
    reason text,
    comment text,
    timestamp timestamp,
    status text,
    admin_action text,
    PRIMARY KEY (report_id)
);
```

## Success Metrics

- Report can be submitted
- Admin can view

## Open Questions

- Report reasons list?
- Auto-trigger thresholds?