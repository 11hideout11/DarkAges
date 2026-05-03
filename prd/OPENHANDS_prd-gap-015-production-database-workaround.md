# PRD: Production Database Workaround

## Introduction

The Production Database PRD (PRD-018) specifies Redis 7 and Scylla 5.4 for caching and persistence, but the docker-compose configuration requires a Docker daemon that isn't available in the current environment. This PRD documents the workaround to achieve data persistence without Docker, and the path to enable full production DB when Docker is available.

## Goals

- Document the Docker blocker clearly
- Implement JSON file-based fallback for development
- Maintain database schema compatibility
- Document path to enable Redis/Scylla when Docker available

## User Stories

### US-001: Issue Documentation
**Description:** As a developer, I need clear documentation of the Docker blocker so I can communicate status.

**Acceptance Criteria:**
- [ ] Root cause documented: Docker daemon not available in CI/sandbox
- [ ] Docker-compose files exist and are valid
- [ ] Workaround path documented for developers without Docker

### US-002: File-Based Fallback
**Description:** As a developer, I need a working persistence layer so I can test save/load functionality.

**Acceptance Criteria:**
- [ ] JSON file-based persistence uses existing JSON database (data/*.json)
- [ ] Server writes to disk on player save
- [ ] Server reads from disk on player load
- [ ] Works without any external services

### US-003: Database Schema Preserved
**Description:** As a backend developer, I want the database schema to match production so migration is easier.

**Acceptance Criteria:**
- [ ] Redis schema documented in DATABASE_SCHEMA.md
- [ ] Scylla CQL schema documented
- [ ] JSON fallback uses equivalent schema (field names match)
- [ ] When Docker available, minimal code changes to switch

### US-004: Docker-Compose Ready
**Description:** As an operator, I want docker-compose to work so production deployment is validated.

**Acceptance Criteria:**
- [ ] docker-compose.dev.yml exists with Redis 7 + Scylla 5.4
- [ ] docker-compose.yml validated in repo (syntax checked)
- [ ] When Docker available, `docker compose up` starts services
- [ ] Health checks documented for each service

## Functional Requirements

- FR-1: Document Docker requirement in README.md
- FR-2: Document JSON fallback mode in deployment docs
- FR-3: JSON database continues to work (no regression)
- Redis/Scylla config stays in tree (no deletion)
- FR-4: Create internal ticket for Docker environment request
- FR-5: Add environment variable to toggle DB mode

## Non-Goals

- Docker installation support (out of scope for code)
- Alternative databases (no MySQL, Postgres for now)
- Production deployment without Docker
- Timeline for Docker resolution (external)

## Technical Considerations

- JSON files in data/ already used for abilities, items, quests, zones
- Player save data can use equivalent JSON format
- Redis provides: session cache, real-time leaderboard, pub/sub events
- Scylla provides: player persistence, quest tracking, economy ledger

## Success Metrics

- Players can save and load progress (JSON fallback works)
- Docker-compose valid syntax (no YAML errors)
- Schema documented for future Redis/Scylla migration

## Open Questions

- When should we prioritize switching to Redis/Scylla?
- Should we add more caching layers in code now?
- Any features that absolutely require Redis?

---

*Generated: 2026-05-03*