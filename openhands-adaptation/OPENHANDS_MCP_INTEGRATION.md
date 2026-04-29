# OpenHands MCP Integration
## Calling OpenHands Agents via MCP from Hermes

---

## Discovery

OpenHands includes an MCP server exposing tools:

```
openhands/app_server/mcp/mcp_router.py
```

**Exposed tools:**
- `create_pr(repo_name, ...)` → create GitHub PR
- `create_mr(...)` → GitLab merge request
- `create_bitbucket_pr(...)` → Bitbucket PR
- `create_azure_devops_pr(...)` → Azure DevOps PR
- `get_conversation_link(...)` → link OpenHands conversation to PR

**How:** OpenHands runs as a server (or as MCP stdio process). Hermes's native MCP client can connect and call these tools as if they were local.

---

## Integration Architecture

```
Hermes Session
   └── native-mcp tool connects to
         OpenHands MCP server (Stdio or SSE)
               └── forwards tool calls to OpenHands runtime
                     └── OpenHands agent (CodeActAgent) executes
```

**Benefits:**
- No need to reinvent CodeActAgent — use OpenHands's agent itself
- Hermes agents can call `create_pr`, `create_mr` as native tools
- OpenHands handles the reasoning loop; Hermes gets back structured results
- Seamless: two systems cooperate via MCP protocol

---

## Setup: Run OpenHands MCP Server

```bash
# Start OpenHands (full server)
cd /opt/openhands
docker compose up -d  # or uvicorn openhands.server.listen:app --port 3000

# In another terminal, launch the OpenHands MCP server as stdio process:
# Option A: Via fastmcp directly (if installed)
fastmcp run openhands.app_server.mcp.mcp_router:mcp_server

# Option B: Via openhands CLI (when available)
openhands mcp serve

# Option C: As a daemon exposing SSE
# Add to Hermes config.yaml:
mcp:
  servers:
    openhands:
      transport: sse
      url: http://localhost:3000/mcp
```

---

## Hermes MCP Configuration

Hermes loads MCP servers from `~/.config/hermes/mcp_config.yaml` (or similar).

Add OpenHands entry:

```yaml
mcp:
  servers:
    openhands:
      command: python3  # or full path
      args:
        - -m
        - openhands.app_server.mcp.mcp_router
      env:
        OPENHANDS_TOKEN: $OPENHANDS_TOKEN  # if needed
        OPENHANDS_SERVER_URL: http://localhost:3000
      transport: stdio  # or sse
```

Hermes will then expose tools:
- `create_pr`
- `create_mr`
- etc.

Use like:
```
User: Create a PR for the combat FSM branch
Hermes: (tools: create_pr) → calls OpenHands via MCP → OpenHands agent
        uses its skills to write PR description, labels, reviewers
```

---

## Custom MCP Server for DarkAges-Specific Automation

We can also add our own MCP server that implements OpenHands-style skills for DarkAges:

**File: `openhands-adaptation/mcp_server_darkages.py`**
```python
from mcp import StdioServer
from mcp.types import Tool, TextContent

@mcp.tool()
def docker_compose(action: str, service: str = None) -> str:
    """Control DarkAges demo Docker Compose services."""
    # similar logic to docker-manage.py
    ...

@mcp.tool()
def security_audit(path: str = ".") -> dict:
    """Scan codebase for security issues (secrets, CVEs)."""
    result = subprocess.run(['python3', '~/.hermes/skills/scripts/security-audit.py'], ...)
    return json.loads(result.stdout)

@mcp.tool()
def run_test_selection(changed: bool = True) -> str:
    """Run only tests affected by recent changes."""
    # call testing-pipeline.py
    ...
```

Register in Hermes config (serializable, directly includes values, never external template):

```yaml
mcp:
  servers:
    darkages:
      command: python3
      args:
        - /root/projects/DarkAges/openhands-adaptation/mcp_server_darkages.py
      transport: stdio
```

Now Hermes has native tools: `docker_compose`, `security_audit`, `run_test_selection`.

---

## Alternative: Direct HTTP API (future)

OpenHands V1 REST API will expose sessions, agent control.
When available, a thin Hermes skill can call:

```python
import httpx
resp = httpx.post('http://localhost:3000/api/v1/sessions', json={...})
```

Until then, MCP is the recommended integration surface.

---

## Decision Matrix

| Need | Approach |
|------|----------|
| PR automation | Connect to OpenHands MCP server (uses CodeActAgent) |
| Docker management | Convert skill to script OR custom MCP tool |
| Security scanning | Convert to script OR custom MCP tool |
| Browser automation | Run OpenHands service with Playwright, possibly via MCP |
| Long-running orchestration | Submit task to OpenHands session, poll for result |

**Recommendation:** Use MCP for anything that OpenHands already exposes (PR creation, etc.). Use converted skill scripts for lightweight operations. Use OpenHands service directly for browser automation only if MCP lacks that tool.

---

## Implementation Steps (MCP Bridge)

1. Ensure OpenHands service runs with MCP enabled (check `mcp_router` is imported)
2. Start MCP server: `fastmcp run openhands.app_server.mcp.mcp_router:mcp_server`
3. Add entry to Hermes MCP config
4. Test from Hermes: `execute_code("mcp list-tools")` should show openhands tools
5. Call `create_pr` via MCP — should create a PR on GitHub (requires token)
6. (Optional) Build custom DarkAges MCP server in Python for our specific skills
7. Update `AGENTS.md` to list new tools

---

## Caveats

**Authentication:** OpenHands MCP tools require GitHub token. Set `GITHUB_TOKEN` in environment.

**Service lifecycle:** Must keep OpenHands running for MCP stdio to connect. Use systemd or background process.

**Tool discrepancies:** OpenHands `create_pr` creates descriptive PRs using CodeActAgent. That's good. Ensure PR base branch is correct (default main, but DarkAges uses main). Set via `repo_name` format: "owner/repo".

**Error handling:** If OpenHands MCP server crashes, Hermes tools disappear. Restart strategy needed.

---

## Conclusion

MCP integration is the most natural fit: reuse OpenHands agent as a **tool provider** rather than replicating its skills manually. Assign privileges accordingly and only integrate core tools.

When OpenHands V1 releases with refined MCP, this integration will only get stronger.
