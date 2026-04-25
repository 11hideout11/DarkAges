# Hermes Agent LLM Provider Fix Plan

**Generated:** 2026-04-24  
**Purpose:** Fix constant provider failures for Hermes agent

---

## ✅ Fixes Applied (2026-04-25)

- **NVIDIA provider**: Added config block to `~/.hermes/config.yaml`; credentials added to `auth.json`; systemd service patched to load `.env`; gateway restarted; provider resolves correctly.
- **StepFun provider**: Credentials added; remains non-functional due to missing subscription (StepFun support ticket opened 2026-04-24). Marked as secondary fallback.
- **Primary LLM routing**: Switched to NVIDIA `meta/llama-3.1-8b-instruct` for cost efficiency. Nous remains as tertiary fallback.

---

## Current Issues Summary

| Provider | Status | Root Cause |
|----------|--------|----------|
| Nous Research | ✅ Working | OAuth valid |
| NVIDIA NIM | ⚠️ Broken | No provider config in config.yaml, credential not resolving |
| StepFun | ❌ Broken | Subscription not provisioned (400 error) |

---

## Required Fixes

### 1. Add NVIDIA Provider Config

Add to `~/.hermes/config.yaml` under `providers:`:

```yaml
providers:
  nvidia:
    base_url: https://integrate.api.nvidia.com/v1
    api_mode: chat_completions
    default_model: meta/llama-3.3-70b-instruct
```

### 2. Add Credentials to Pool

Run these commands:
```
hermes auth add nvidia
hermes auth add stepfun
```

(Enter API keys from ~/.hermes/.env when prompted)

### 3. Restart Gateway

```
pkill -f hermes_gateway
# or re-start the service
```

### 4. StepFun Support Ticket

**Subject:** Step Plan subscription not provisioned for API key

**Body:**
```
API Key: [prefix]... (first 8 chars)
Error: 400 - "you have no active step plan subscription"
Tested: /models and /chat/completions endpoints
Invoice: [date] - [amount]
Please check step_plan scope/role attached to this key.
```

---

## Verification Commands

Verification results (post-fix):

```bash
$ python3 -c "from agent.auxiliary_client import resolve_provider_client; print(resolve_provider_client('nvidia'))"
<client object ...>  # non-None

$ python3 -c "from agent.auxiliary_client import resolve_provider_client; print(resolve_provider_client('stepfun'))"
<client object ...>  # non-None (resolves but 400 from API)
```

---

## Alternative: Use Working Config

If StepFun can't be fixed, ensure fallback chain works with NVIDIA only:

```yaml
fallback_providers:
  - nvidia
fallback_model:
  provider: nvidia
  model: meta/llama-3.3-70b-instruct
```