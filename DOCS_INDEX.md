# DarkAges Documentation Index

**Last Updated:** 2026-05-03

---

## Quick Reference

| What You Need | See This File |
|--------------|---------------|
| Current project state | [AGENTS.md](./AGENTS.md) |
| Build & test status | [AGENTS.md](./AGENTS.md) |
| PRD requirements | [prd/](./prd/) directory |
| Phase summaries | [phases/](./phases/) directory |
| Architecture docs | [docs/](./docs/) directory |

---

## Primary Files (Keep at Root)

| File | Purpose |
|------|---------|
| `README.md` | Project overview, quick start |
| `AGENTS.md` | **Authoritative** — current state, PRD status, execution history |
| `VERSION` | Version number |

---

## Organization

```
DarkAges/
├── README.md              # Overview + quick start
├── AGENTS.md             # Authoritative state (READ THIS FIRST)
├── VERSION              # Version number
│
├── docs/                # Technical documentation
│   ├── API_CONTRACTS.md
│   ├── NETWORK_PROTOCOL.md
│   ├── DATABASE_SCHEMA.md
│   ├── collision-matrix.md
│   └── ...
│
├── prd/                 # Product Requirements (90+ docs)
│   ├── prd-master.md    # Master PRD list
│   ├── prd-001-server-core.md
│   └── ...
│
├── phases/              # Phase execution summaries
│   ├── PHASE0_SUMMARY.md
│   ├── PHASE1-5_SUMMARY.md
│   └── ...
│
├── docs/archive/         # Historical docs (2026-01)
│
├── reports/             # Build/test reports
│
└── openhands-adaptation/ # OpenHands integration
```

---

## Blocked Items (External)

| Item | Blocker |
|------|---------|
| GNS Runtime | WebRTC submodule (`webrtc.googlesource.com`) restricted |
| Production DB | Docker daemon required |

---

## Test Baseline

- **Test Cases:** 1305
- **Assertions:** 7254  
- **Pass Rate:** 100%

---

## Getting Started

```bash
# Build
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

# Test
cd build && ctest --output-on-failure
```

---

*This file helps agents navigate the repository. See AGENTS.md for authoritative state.*