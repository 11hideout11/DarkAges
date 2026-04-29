# OpenHands Skill Conversion Matrix
For DarkAges MMO - Hermes Integration

Upstream skills: 26 | Converted: 4 | Reuse: 4 | Reference: 5 | Skip: 12 | Future: 1

## Conversion Legend
- CONVERT -> Python script : high priority
- REUSE: already have in Hermes
- REFERENCE: useful patterns only
- SKIP: not applicable

## Full Mapping

| Skill | Type | Action | Target |
|-------|------|--------|--------|
| code-review.md | knowledge | REUSE | code-review.py (augment rubric)
| testing.md | knowledge | CONVERT | testing-pipeline.py
| docker.md | knowledge | CONVERT | docker-manage.py
| security.md | knowledge | CONVERT | security-audit.py
| git.md | knowledge | REF | already covered
| github.md | knowledge | REF | only if GH Actions needed
| gitlab.md | knowledge | SKIP | not using
| kubernetes.md | knowledge | FUTURE | optional cloud phase
| onboarding.md | knowledge | REF | pattern for CONTRIBUTING
| fix_test.md | knowledge | REUSE | covered by test-fixing-pitfalls
| default-tools.md | knowledge | REUSE | Hermes tools cover
| add_agent.md | knowledge | SKIP | not building agents
| add_repo_inst.md | workflow | DONE | .openhands/repo.md already created
| address_pr_comments.md | knowledge | REF | PR workflow in AGENTS.md
| update_pr_description.md | knowledge | SKIP | manual update
| update_test.md | knowledge | REUSE | covered
| ssh.md | knowledge | SKIP | no remote servers
| npm.md | knowledge | SKIP | no Node.js
| azure_devops.md | knowledge | SKIP | not using Azure
| bitbucket.md | knowledge | SKIP | —
| bitbucket_data_center.md | knowledge | SKIP | —
| agent-builder.md | knowledge | SKIP | —
| agent_memory.md | knowledge | REF | Hermes memory covers
| codereview-roasted.md | knowledge | SKIP | humor
| flarglebargle.md | knowledge | SKIP | placeholder
| pdflatex.md | knowledge | SKIP | no PDF gen
| swift-linux.md | knowledge | SKIP | —

## Implementation Notes

Each converted skill becomes an executable Python script in
~/.hermes/skills/scripts/ that performs deterministic checks and
outputs structured JSON. See skills/ directory for templates.

Upstream skills stay in openhands-adaptation/reference/ for provenance
and periodic diff during sync.

End of mapping.