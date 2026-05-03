# PRD-GAP-004: Production Metrics Dashboard

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P2 - Medium  
**Category:** Production - Observability

---

## Introduction

PRD-032 (Production Metrics Dashboard) proposes a monitoring solution, BUT the implementation doesn't exist. For production readiness, operators need to see server health, player counts, zone status, combat metrics, and performance data.

**Problem:** No dashboard exists. Prometheus metrics may be configured but not exposed in a usable UI. Grafana dashboards in infra/monitoring may not be connected to live metrics.

---

## Goals

- Real-time server metrics in dashboard UI
- Player count and zone population tracking
- Combat event monitoring (kills, damage, deaths)
- Performance metrics (FPS, tick time, latency)
- Alert thresholds for critical issues

---

## User Stories

### US-001: Server Health Overview
**Description:** As an operator, I want to see server health at a glance.

**Acceptance Criteria:**
- [ ] Server uptime displays
- [ ] CPU/Memory usage shows
- [ ] Active connections count shows
- [ ] Green/yellow/red status indicators

### US-002: Player Metrics
**Description:** As an operator, I want to see player activity.

**Acceptance Criteria:**
- [ ] Current players online shows
- [ ] Players per zone shows
- [ ] Connection/disconnection events log
- [ ] Peak player count today shows

### US-003: Zone Status
**Description:** As an operator, I want to see zone health.

**Acceptance Criteria:**
- [ ] All zones list with status
- [ ] Entity count per zone shows
- [ ] NPC count per zone shows
- [ ] Zone tick time shows

### US-004: Combat Metrics
**Description:** As an operator, I want to monitor combat activity.

**Acceptance Criteria:**
- [ ] Kills per minute shows
- [ ] Damage dealt shows
- [ ] Deaths per minute shows
- [ ] Top damage dealers shows

### US-005: Performance Metrics
**Description:** As an operator, I want to see performance data.

**Acceptance Criteria:**
- [ ] Server tick time shows (target: <16ms at 60Hz)
- [ ] Network latency shows
- [ ] Snapshot frequency shows
- [ ] Memory usage trend shows

---

## Functional Requirements

- FR-1: MetricsSystem collects server health metrics
- FR-2: MetricsSystem collects player metrics
- FR-3: MetricsSystem collects zone metrics  
- FR-4: MetricsSystem collects combat metrics
- FR-5: HTTP endpoint exposes /metrics in Prometheus format
- FR-6: WebSocket pushes real-time updates to dashboard

---

## Non-Goals

- Distributed tracing - deferred
- Log aggregation (ELK) - use existing
- Custom alerting rules - basic thresholds only
- Historical data retention > 24h - deferred
- Auto-scaling triggers - deferred

---

## Technical Considerations

- **Existing Code:**
  - infra/monitoring/grafana/dashboards/darkages-zones.json - exists
  - docs/runbooks/metrics-reference.md - exists
  
- **Metrics to Expose:**
  - `darkages_players_online` - gauge
  - `darkages_zone_entities` - gauge by zone
  - `darkages_combat_kills` - counter
  - `darkages_tick_time_ms` - histogram

- **Integration:**
  - Use existing Prometheus client library
  - Expose /metrics HTTP endpoint
  - Connect Grafana to Prometheus

---

## Success Metrics

- **Functional:**
  - Dashboard loads and shows data
  - Metrics update in real-time
  - Alerts trigger on thresholds

- **Operational:**
  - MTTR improved by seeing issues faster
  - No manual log parsing for basic checks