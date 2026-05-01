# PRD: Leaderboard System

## Introduction

DarkAges lacks any leaderboard system to track player achievements and competition. Players cannot view top performers, daily rankings, or their personal stats relative to others. This PRD implements a leaderboard system using ScyllaDB.

## Goals

- Display top players by category
- Track daily and all-time standings
- Show player personal rank

## User Stories

### LDR-001: View Leaderboard
**Description:** As a player, I want to see top players.

**Acceptance Criteria:**
- [ ] Categories: Kills, Damage, Wins
- [ ] Daily and all-time views
- [ ] Top 100 display
- [ ] Player personal rank shown

### LDR-002: Rank Updates
**Description:** As a system, I want ranks to update.

**Acceptance Criteria:**
- [ ] Kills increment on NPC kill
- [ ] Damage tracked
- [ ] Wins tracked on PvP/boss

## Functional Requirements

- FR-1: Leaderboard table in ScyllaDB
- FR-2: Update on combat events
- FR-3: Query endpoints

## Non-Goals

- Weekly leaderboards
- Regional filtering

## Technical Considerations

### Database Table
```sql
CREATE TABLE leaderboard_daily (
    category text,
    day text,
    player_id bigint,
    score bigint,
    PRIMARY KEY ((category, day), score, player_id
);
```

## Success Metrics

- Leaderboard UI accessible
- Rankings update correctly

## Open Questions

- Categories to track?
- Update frequency?