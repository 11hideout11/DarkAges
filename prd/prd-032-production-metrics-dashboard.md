# PRD-032: Production Metrics Dashboard

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Medium  
**Prerequisite:** MetricsExporter.hpp (exists)

---

## Introduction

The server has MetricsExporter.hpp for Prometheus metrics, but there are no production-ready dashboards for operators to monitor server health, player activity, and performance in real-time.

## Goals

- Create Grafana dashboard for server metrics
- Display key performance indicators
- Alert on anomaly thresholds
- Enable player activity tracking

## User Stories

### US-001: Server Health Dashboard
**Description:** As an operator, I want to see server health at a glance so that I know everything is working.

**Acceptance Criteria:**
- [ ] TPS (ticks per second) displayed
- [ ] Memory usage displayed
- [ ] Connection count displayed
- [ ] Error rate displayed

### US-002: Player Activity Metrics
**Description:** As a game designer, I want player activity metrics so that I can understand engagement.

**Acceptance Criteria:**
- [ ] Active players tracked
- [ ] Zone distribution shown
- [ ] Playtime tracked
- [ ] Session duration shown

### US-003: Performance Alerts
**Description:** As an operator, I want alerts when metrics exceed thresholds so that I can respond to issues.

**Acceptance Criteria:**
- [ ] TPS alert when <55
- [ ] Memory alert when >80%
- [ ] Connection limit warning
- [ ] Error spike notification

### US-004: Network Metrics
**Description:** As a network engineer, I want network quality metrics so that I can diagnose issues.

**Acceptance Criteria:**
- [ ] Packets/sec tracked
- [ ] Bandwidth usage shown
- [ ] RTT average displayed
- [ ] Connection quality shown

## Functional Requirements

- FR-1: Grafana dashboard JSON must include all core metrics
- FR-2: Dashboard must refresh at 1-second intervals
- FR-3: Alert rules configured for threshold violations
- FR-4: Data sources: Prometheus (already configured in infra/)

## Non-Goals

- Custom metrics SDK (use existing)
- Log aggregation (deferred - use ELK)
- Distributed tracing (deferred)
- Player analytics (beyond basic counts)

## Technical Considerations

- Existing: grafana/ directory in infra/
- Data source: Prometheus (docker-compose.monitoring.yml)
- Integration: /metrics endpoint (MetricsExporter)

## Success Metrics

- Dashboard: functional for all 4 sections
- Alerts: 100% actionable
- Refresh: <1s latency

---

**PRD Status:** Proposed  
**Author:** OpenHands Gap Analysis  
**Next Step:** Create dashboard JSON