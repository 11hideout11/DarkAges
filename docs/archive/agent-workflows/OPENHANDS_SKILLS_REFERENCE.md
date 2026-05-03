# DarkAges Skill Reference — OpenHands Adaptation

## Standalone Executable Skills (Hermes)

All installed in `~/.hermes/skills/scripts/`. Each supports `--json` output.

| Script | Purpose | Exit Code |
|--------|---------|-----------|
| `security-audit.py` | Scan codebase for hardcoded secrets | 0 = clean, 1 = findings (if `--strict`) |
| `docker-manage.py` | Control demo containers (start/stop/restart/logs/status) | 0 = success |
| `testing-pipeline.py` | Targeted test execution based on changed files | 0 = tests passed |
| `test-flakiness.py` | Repeated test runs to detect unstable tests | 0 = stable, 1 = flaky detected |
| `coverage-report.py` | Generate lcov/gcovr HTML coverage report | 0 = success, 1 = below threshold (if `--rate`) |
| `pr-create.py` | Create GitHub PR from current branch | 0 = created |
| `pr-comment.py` | Add comment to existing PR | 0 = success |
| `code-review.py` | Architectural code review (Hermes session only, requires clarify) | CCGS |
| `gate-check.py` | Pre-merge gate (build → test → review → ADR check) | 0 = PASS, 1 = FAIL |

### Quick Examples

```bash
# Security scan of server code
python3 ~/.hermes/skills/scripts/security-audit.py --path src/server --strict

# Manage demo containers
python3 ~/.hermes/skills/scripts/docker-manage.py start
python3 ~/.hermes/skills/scripts/docker-manage.py logs client -f

# Run only tests affected by recent changes
python3 ~/.hermes/skills/scripts/testing-pipeline.py --changed

# Detect flaky tests
python3 ~/.hermes/skills/scripts/test-flakiness.py --repeat 20 --threshold 0.95

# Generate coverage report
python3 ~/.hermes/skills/scripts/coverage-report.py --rate 80

# Create PR (requires gh or GITHUB_TOKEN)
python3 ~/.hermes/skills/scripts/pr-create.py --title "Feat: add Foot IK" --body "Implements..."

# Comment on PR (requires gh or GITHUB_TOKEN)
python3 ~/.hermes/skills/scripts/pr-comment.py --pr 42 --body "LGTM :+1:"
```

---

## OpenHands Microagent Knowledge (Reference Only)

These `.md` files live in `openhands-adaptation/reference/`. They encode OpenHands's institutional knowledge. To use them, copy relevant ones into `.openhands/` or let the OpenHands server load them as knowledge.

| File | Purpose |
|------|---------|
| `github.md` | GitHub API usage, PR creation via `create_pr` MCP tool |
| `gitlab.md` | GitLab MR creation via `create_mr` MCP tool |
| `bitbucket.md` | Bitbucket PR creation via `create_bitbucket_pr` MCP tool |
| `azure_devops.md` | Azure DevOps PR tool |
| `ssh.md` | Remote server access patterns |
| `onboarding.md` | New developer onboarding checklist |
| `agent-builder.md` | Constructing custom OpenHands agents |
| `add_agent.md` | Creating new microagents |
| `agent_memory.md` | Repository memory conventions |
| `add_repo_inst.md` | Bootstrapping repository `.openhands/microagents/repo.md` |
| `update_pr_description.md` | Auto-update PR desc to reflect changes |
| `update_test.md` | Align test expectations with implementation |
| `address_pr_comments.md` | Address review comments via GitHub API |
| `fix_test.md` | Fix failing test functions |
| `code-review.md` | Comprehensive code review rubric |
| `codereview-roasted.md` | Lightweight review style |
| `security.md` | Security audit procedures (converted) |
| `docker.md` | Container management (converted) |
| `fix-py-line-too-long.md` | Python line-length fixing (not applicable) |
| `kubernetes.md` | K8s deployment knowledge |
| `npm.md` | Node.js package management knowledge |
| `pdflatex.md` | LaTeX compilation knowledge |
| `swift-linux.md` | Swift on Linux |
| `flarglebargle.md` | Test microagent |

**Note:** Knowledge files marked "type: knowledge" provide guidance to OpenHands agents; they are **not** converted to standalone scripts.

---

## Conversion Mapping Summary

| Upstream Skill | Converted? | Script | Notes |
|----------------|-----------|--------|-------|
| `security` | ✅ | `security-audit.py` | stdlib-only scanner |
| `docker` | ✅ | `docker-manage.py` | demo harness containers |
| `testing` | ✅ | `testing-pipeline.py` | affected test selection |
| `code-review` | ⚠️ | `code-review.py` | CCGS origin; needs Hermes tools |
| `github` / `gitlab` / `bitbucket` | ✅ MCP | `pr-create.py` + `pr-comment.py` | gh CLI / API wrappers |
| `update_pr_description` | REF | — | useful as agent knowledge |
| `fix_test` / `update_test` | REF | — | best left to agent reasoning |
| `kubernetes`, `npm`, `pdflatex`, `swift-linux` | REF | — | domain-specific, not needed |

---

## Integration Points

### MCP Bridge (Optional)
When OpenHands service runs, Hermes can call `create_pr`, `create_mr`, etc. via [`native-mcp`](https://hermes.dev/docs/mcp). See `OPENHANDS_MCP_INTEGRATION.md`.

### Autonomous Loop
Skills are invoked by the autonomous generator (`scripts/autonomous/cron_dev_loop.py`) as task implementations:

- **Fix phase**: `testing-pipeline --changed`, then targeted re-run with `--bisect`
- **Security gate**: `security-audit --strict` before commit
- **Pre-merge**: `gate-check.py` runs build + tests + review

### Git Hooks
- `.git/hooks/pre-push` — runs gate-check
- `.git/hooks/commit-msg` — validates commit message format + design doc presence

---

## Versioning & Maintenance

- All standalone skills are **pure Python stdlib** (no external dependencies beyond standard Linux utilities: clang-format, dotnet, gh, lcov/gcovr)
- Skill versions: derived from OpenHands originals (v1.0.0) plus local revision
- Update path: follow upstream OpenHands skill guides in `openhands-adaptation/reference/`
- Hermes-specific patches live in `.hermes/skills/scripts/` symlinks → source in `openhands-adaptation/skills/`

---

## Quick Index

| Need | Skill |
|------|-------|
| Scan for secrets | `security-audit.py` |
| Start/stop demo containers | `docker-manage.py` |
| Run tests that changed | `testing-pipeline.py` |
| Check test stability | `test-flakiness.py` |
| See coverage | `coverage-report.py` |
| Create PR | `pr-create.py` |
| Comment on PR | `pr-comment.py` |
| Full quality gate | `gate-check.py` |
| Code review | `code-review.py` (session) |

