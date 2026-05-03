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
| NVIDIA NIM | ✅ Active (primary) | Fixed: config+yaml + credentials + service restart |
| Nous Research | ✅ Standby | OAuth valid; tertiary fallback |
| StepFun | ⚠️ Disabled | Subscription not provisioned (400 error); support ticket open 2026-04-24 |

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

Verification results (post-fix, 2026-04-25):

```bash
$ python3 -c "from agent.auxiliary_client import resolve_provider_client; print(resolve_provider_client('nvidia'))"
<client nvidia meta/llama-3.1-8b-instruct>  ✅ RESOLVED

$ python3 -c "from agent.auxiliary_client import resolve_provider_client; print(resolve_provider_client('stepfun'))"
<client stepfun step-3.5-flash>  ⚠️ RESOLVES-but-400 (no step_plan scope)

$ hermes ask "test" --model meta/llama-3.1-8b-instruct
[USE NVIDIA]
test  ✅ WORKS (NVIDIA primary)

$ hermes ask "test" --model step-3.5-flash
[USE STEPFUN]
Error: 400 - no active step plan subscription  ⚠️ DISABLED
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