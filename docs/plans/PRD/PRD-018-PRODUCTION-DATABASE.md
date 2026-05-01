# PRD-018: Production Database Integration — Redis + ScyllaDB Activation

**Version:** 1.0
**Status:** 🔄 Not Started — Requires infrastructure (Redis + ScyllaDB)
**Owner:** DATABASE_AGENT
**Priority:** HIGH (P2 — Infrastructure Completeness)
**Dependencies:** None (independent service)
**Issue:** #5 from PROJECT_ISSUES_TRACKER.md

---

## Implementation Status (2026-05-01)

### 📋 Prerequisites Needed
- [ ] Docker environment with Redis + ScyllaDB
- [ ] Integration tests with real DB containers
- [ ] Migration scripts

### Blocked By
- Requires external infrastructure not available in current environment
- Requires docker-compose environment setup

---

## 1. Overview

### 1.1 Purpose
Activate **real database backends** (Redis + ScyllaDB) in production-like environments, replacing the current stub implementations. This provides:
- Persistent session storage (Redis)
- Persistent event/gameplay logs (ScyllaDB)
- Integration validation before deployment
- Database migration scripts
- CI integration tests against real DB containers

### 1.2 Current State

**Database code exists but is stubbed:**
```
src/server/src/db/
  RedisManager.cpp     ← Real implementation, but only activated if BUILD_WITH_REDIS
  ScyllaManager.cpp   ← Real implementation, but only activated if BUILD_WITH_SCYLLA
  CombatEventLogger_stub.cpp ← Active by default (does nothing)
  DatabaseManager.cpp ← Factory that chooses stub vs real based on flags
```

**Build flags:**
```
CMake options:
  -DENABLE_REDIS=OFF   → uses CombatEventLogger_stub
  -DENABLE_SCYLLA=OFF → uses ScyllaManager_stub (or missing)
```

**Result:** Zero tests exercise real database code. Cannot validate persistence, query performance, or data integrity.

---

## 2. Requirements

### 2.1 Functional Requirements
ID     | Requirement                         | Priority | Details
-------|-------------------------------------|----------|--------
DB-001 | Redis integration tests             | P0       | Connect to real Redis (Docker), verify session read/write
DB-002 | ScyllaDB integration tests          | P0       | Connect to real Scylla (Docker), verify CQL queries
DB-003 | CI pipeline DB jobs                 | P0       | GitHub Actions runs DB integration tests on PRs
DB-004 | Database migration scripts          | P1       | Schema versioning (Redis keyspace, Scylla tables)
DB-005 | Configuration examples              | P1       | docker-compose.yml for local dev (Redis + Scylla)
DB-006 | Connection retry logic              | P1       | Graceful handling of DB startup race conditions
DB-007 | Production deployment guide         | P1       | Step-by-step for AWS/GCP deployment
DB-008 | Monitoring/health checks            | P2       | DB connectivity alerts

### 2.2 Non-Functional
- Test runtime: <5 minutes for full DB integration suite
- CI resources: ~1GB Docker memory per job (Redis ~512MB, Scylla ~1GB)
- Zero flakiness: tests must be idempotent (clear DB between runs)
- Backwards compatibility: schema migrations preserve existing data

---

## 3. Scope

### 3.1 In Scope
- `tests/server/integration/db/` — new integration test suite
- `.github/workflows/ci-db.yml` — new CI job (or extend existing)
- `infra/docker-compose.db.yml` — local dev DB stack
- `docs/database-migrations.md` — schema evolution guide
- `infra/terraform/` (if exists) — production DB provisioning

### 3.2 Out of Scope
- Database backup/restore automation (Phase 12+)
- Multi-region replication (Phase 13+)
- Database sharding/scaling (covered in PRD-004 already)
- Data warehouse analytics (future)

---

## 4. Technical Solution

### 4.1 Testing Architecture

**Integration tests** spin up ephemeral Docker containers:

```yaml
# .github/workflows/ci-db.yml
jobs:
  db-integration-test:
    runs-on: ubuntu-latest
    services:
      redis:
        image: redis:7-alpine
        ports: [6379:6379]
        options: --health-cmd "redis-cli ping"
      scylla:
        image: scylladb/scylla:5.2
        ports: [9042:9042]
        options: --health-cmd "cqlsh -e 'describe cluster'"
    steps:
      - uses: actions/checkout@v3
      - name: Build server with DB enabled
        run: cmake -DENABLE_REDIS=ON -DENABLE_SCYLLA=ON .. && make -j$(nproc)
      - name: Run DB integration tests
        run: ctest -R "DB_INTEGRATION" --output-on-failure
```

### 4.2 Test Cases

**tests/server/integration/db/TestRedisIntegration.cpp:**
```cpp
TEST(RedisIntegration, SessionRoundTrip) {
    RedisManager redis("localhost", 6379);
    ASSERT_TRUE(redis.Connect());
    
    PlayerSession session{"player123", "token456", time(NULL) + 3600};
    ASSERT_TRUE(redis.SaveSession(session));
    
    auto retrieved = redis.LoadSession("player123");
    EXPECT_EQ(retrieved.session_id, "player123");
    EXPECT_EQ(retrieved.auth_token, "token456");
}
```

**tests/server/integration/db/TestScyllaIntegration.cpp:**
```cpp
TEST(ScyllaIntegration, CombatEventLogging) {
    ScyllaManager scylla("localhost", 9042);
    ASSERT_TRUE(scylla.Connect());
    
    CombatEvent event{
        .entity_id = 1,
        .event_type = CombatEvent::Type::PlayerHit,
        .timestamp = get_time(),
        .damage = 50
    };
    ASSERT_TRUE(scylla.InsertCombatEvent(event));
    
    auto events = scylla.QueryEntityEvents(1, 10);
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].damage, 50);
}
```

### 4.3 Docker Compose for Local Development

`infra/docker-compose.db.yml`:
```yaml
version: '3.8'
services:
  redis:
    image: redis:7-alpine
    ports: ["6379:6379"]
    volumes: ["redis-data:/data"]
    command: redis-server --appendonly yes

  scylla:
    image: scylladb/scylla:5.2
    ports: ["9042:9042"]
    volumes: ["scylla-data:/var/lib/scylla"]
    command: --smp 2 --memory 2G
    healthcheck:
      test: ["CMD", "cqlsh", "-e", "describe cluster"]
      interval: 10s
      timeout: 5s
      retries: 5

volumes:
  redis-data:
  scylla-data:
```

**Usage:** `docker-compose -f infra/docker-compose.db.yml up -d`

### 4.4 Schema Migrations

**Redis:** No migrations needed (key-value store). Keyspace: `darkages:*`

**ScyllaDB:** Table schema versioning.

```
Schema v1 (current):
  CREATE TABLE combat_events (
      entity_id bigint,
      event_time timestamp,
      event_type text,
      damage int,
      PRIMARY KEY ((entity_id), event_time)
  ) WITH CLUSTERING ORDER BY (event_time DESC);

Migration strategy:
  - Table name with version suffix: combat_events_v1
  - On schema change: create combat_events_v2, backfill, switch
  - Deployment: run migration script before server start
```

**Migration script:** `infra/db/migrations/001_initial.cql`

---

## 5. File Deliverables

| Path | Type | Description |
|------|------|-------------|
| `.github/workflows/ci-db.yml` | CI | Database integration job (Redis + Scylla) |
| `tests/server/integration/db/TestRedisIntegration.cpp` | Test | Redis round-trip tests |
| `tests/server/integration/db/TestScyllaIntegration.cpp` | Test | Scylla CQL tests |
| `infra/docker-compose.db.yml` | DevOps | Local dev DB stack |
| `docs/database-migrations.md` | Docs | Schema evolution procedures |
| `infra/db/migrations/001_initial.cql` | Migration | Scylla table creation + indexes |
| `src/server/src/db/CombatEventLogger_real.cpp` | RENAME | Rename stub→real for clarity (optional) |

---

## 6. Acceptance Criteria

✅ **Functional Integration**
- Redis integration tests pass: session read/write/expiry
- Scylla integration tests pass: combat event insert/query
- Tests run in CI (both ENABLE_REDIS=ON/OFF jobs succeed)
- Docker Compose stack starts in <30 seconds locally

✅ **Production Readiness**
- Schema migrations documented and versioned
- Production deployment guide includes DB setup instructions
- Connection retry logic handles DB startup race (server starts before DB)
- Health checks: `GET /health/db` returns Redis+Scylla status

✅ **Quality**
- Zero flakiness (10 consecutive CI runs: all pass)
- Tests are isolated: each test clears DB before/after
- No secrets in repo (use GH Actions secrets for production)

✅ **Performance Budget**
- DB operations: Redis <1ms, Scylla <5ms p99
- Integration test suite runtime <5 minutes in CI

---

## 7. Testing Plan

### 7.1 Local Development
```bash
# Start databases
docker-compose -f infra/docker-compose.db.yml up -d

# Verify connectivity
redis-cli ping           # → PONG
cqlsh -e "DESCRIBE KEYSPACES"  # → system, darkages

# Run tests
ctest -R "DB_INTEGRATION" -VV
```

### 7.2 CI Pipeline
```
Job: Database Integration Test (Ubuntu)
  - Services: redis, scylla (Docker)
  - Build: cmake -DENABLE_REDIS=ON -DENABLE_SCYLLA=ON
  - Test: ctest -R DB_INTEGRATION --output-on-failure
  - Teardown: containers stop

Job: Database Integration Test (Windows)
  - Services: Redis for Windows (or WSL2 Redis), Scylla for Windows?
  - Skipped if DB not available on Windows runners (acceptable)
```

---

## 8. Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| ScyllaDB Docker image too heavy (>2GB) | CI slows down | Use `scylladb/scylla:5.2-linux` slim image; cache layers |
| Redis port conflict on developer machines | CI passes locally fails | Use random port in CI; configurable via env var |
| Test data not cleaned between runs | Flaky tests | Fixtures use `FLUSHALL` on Redis; `TRUNCATE` on Scylla |
| Scylla startup slow (>60s) | CI timeout | Add startup wait + health check loop (max 2min) |
| Production DB credentials leaked | Security incident | Use secrets manager; .gitignore config file |

---

## 9. Production Deployment

**Terraform (if used):**
```hcl
resource "aws_elasticache_replication_group" "darkages_redis" {
  replication_group_id = "darkages-redis"
  description          = "DarkAges session store"
  num_node_groups     = 1
  replica_count       = 1  # HA
  engine              = "redis"
  node_type           = "cache.t3.micro"
}

resource "aws_scylla_cluster" "darkages" {
  # Use ScyllaDB Cloud or self-hosted on EC2
}
```

**Kubernetes (if used):**
```yaml
apiVersion: v1
kind: Secret
metadata:
  name: darkages-db-creds
data:
  redis-password: <base64>
  scylla-username: <base64>
---
apiVersion: apps/v1
kind: Deployment
spec:
  template:
    spec:
      containers:
      - name: darkages-server
        env:
        - name: REDIS_HOST
          value: "darkages-redis"
        - name: SCYLLA_CONTACT_POINTS
          value: "darkages-scylla"
```

---

## 10. Success Metrics

| Metric | Target | Date |
|--------|--------|------|
| Integration test coverage | >= 80% of DB code paths | 2026-05-15 |
| CI DB job pass rate | 100% (0 flakiness) | Ongoing |
| DB startup time (local) | <30 seconds | 2026-05-10 |
| Production provisioning time | <15 minutes | Phase 12 |

---

## 11. Related Work

- **PRD-004** (Sharding) — may require DB clustering strategy (future)
- **PRD-006** (Infrastructure) — overall deployment decisions
- **PROJECT_ISSUES_TRACKER.md**: Issue #5 tracks this work

---

## 12. Acceptance Sign-off

**Criteria (all must pass before merge):**
1. ✓ Redis integration tests: 100% pass on CI
2. ✓ Scylla integration tests: 100% pass on CI
3. ✓ Local dev: `docker-compose up` works on fresh machine
4. ✓ No stub code remains in production build (stub files deleted)
5. ✓ Documentation complete (migrations, deployment, troubleshooting)
6. ✓ Zero regression in unit tests (2129 baseline)

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to DATABASE_AGENT for implementation
