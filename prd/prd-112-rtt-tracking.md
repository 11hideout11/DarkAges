# PRD-112: Network RTT Tracking System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Prerequisite:** PRD-027 GNS Runtime (in progress)

---

## Introduction

The INetworkSocket.cpp has a TODO at line 421: "Implement RTT tracking". RTT (Round-Trip Time) tracking is essential for lag compensation, predictive systems, and network quality monitoring. Currently, there is no RTT measurement in the network layer.

## Goals

- Measure per-connection RTT (Round-Trip Time)
- Report network latency to game systems
- Enable lag compensation features
- Support server-authoritative predictions

## User Stories

### US-001: RTT Measurement
**Description:** As a network engineer, I need to measure RTT for each connection so that I know the client's network latency.

**Acceptance Criteria:**
- [ ] RTT calculated per connection
- [ ] RTT updated on each packet exchange
- [ ] RTT accessible via API

### US-002: Moving Average RTT
**Description:** As a game designer, I need stable RTT values so that lag compensation doesn't jitter.

**Acceptance Criteria:**
- [ ] RTT uses moving average (not raw)
- [ ] Sample window: last N packets (recommend 10)
- [ ] Outlier filtering applied

### US-003: RTT API Access
**Description:** As a game system developer, I need to query RTT so that I can implement lag compensation.

**Acceptance Criteria:**
- [ ] GetRTT(ConnectionHandle) returns milliseconds
- [ ] GetAverageRTT(ConnectionHandle) returns smoothed value
- [ ] Returns -1 for disconnected

### US-004: Connection Quality Tracking
**Description:** As an operator, I need network quality metrics so that I can diagnose issues.

**Acceptance Criteria:**
- [ ] Packet loss tracking
- [ ] Jitter measurement
- [ ] Connection quality score

## Functional Requirements

- FR-1: INetworkSocket must implement GetRTT(ConnectionHandle) returning float (milliseconds)
- FR-2: INetworkSocket must implement GetAverageRTT(ConnectionHandle) returning smoothed float
- FR-3: RTT updated on each packet send/receive with timestamp
- FR-4: Moving average window: 10 packets
- FR-5: Outliers (>2x average) filtered from average
- FR-6: INetworkSocket must implement GetConnectionQuality() returning 0-100 score

## Non-Goals

- RTT-based auto-scaling (deferred)
- Real-time RTT graphs (UI deferred)
- Automatic lag compensation adjustment (deferred to game systems)
- RTT-based matchmaking (deferred)

## Technical Considerations

- Measurement: Timestamp in packet header, echo back, measure delta
- Update frequency: Every packet exchange
- API location: INetworkSocket.hpp
- Integration: Called by CombatSystem for lag compensation

## Success Metrics

- RTT accuracy: ±5ms of actual
- RTT update frequency: per-packet
- Moving average: smooth (no jitter >10ms)
- Connection quality: 0-100 score

## Open Questions

1. **Q: What packet format for RTT measurement?**
   - A: Use existing heartbeat packet type

2. **Q: Sample window size?**
   - A: 10 packets is standard - configurable

3. **Q: Include in every packet or separate?**
   - A: Include timestamp in all reliable packets

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Gap Analysis  
**Next Step:** Implement RTT measurement in INetworkSocket