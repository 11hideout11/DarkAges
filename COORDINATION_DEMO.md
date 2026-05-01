# Multi-Agent Coordination Demo

**Created:** 2026-05-01  
**Purpose:** Demonstrate synchronization workflow between local Hermes Agent and cloud OpenHands agents.

## Test Sequence

1. Initial state: clean, synced with remote
2. Create this file locally
3. Commit with conventional message
4. Push to remote (trigger pre-push hook with full test suite)
5. Verify remote sync via `git operation verification`
6. Clean up: delete file, commit, push

## Observations

- Pre-push hook runs ctest suite (~80s for 2129 test cases)
- Remote sync verification uses `git fetch origin` + `git status`
- Conflict handling: if divergent, `git pull --rebase` required

**This file will be deleted at end of demo.**