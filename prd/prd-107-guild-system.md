# PRD-107: Guild System Implementation

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** Proposed  
**Priority:** Medium  
**Prerequisite:** None - requires basic server infrastructure

---

## 1. Problem Statement

No guild functionality exists. The PRD document exists at `prd-guild-system.md` but no implementation has been started. Guilds are a key social feature for MMO games.

### Current State
- ⚠️ Not implemented
- 📄 prd-guild-system.md exists (design only)

### Impact
- No player organizations
- No guild social features
- Missing endgame content

---

## 2. Goals

### Primary Goals
1. Guild creation/disbanding
2. Member management (invite/kick/promote)
3. Guild chat
4. Guild storage
5. Guild bank (items)

### Success Criteria
- [ ] Guilds can be created
- [ ] Members can join/leave
- [ ] Guild chat works
- [ ] Guild storage works

---

## 3. Technical Specification

### Guild Data

```csharp
public class Guild {
    public uint32_t Id;
    public string Name;
    public string Tag;  // [TAG]
    public uint32_t LeaderId;
    public List<GuildMember> Members;
    public GuildRank[] Ranks;
    public GuildStorage Storage;
    public int Level;
    public int Experience;
}

public enum GuildPermission {
    INVITE,
    KICK,
    PROMOTE,
    EDIT_NOTES,
    WITHDRAW_STORAGE,
    DEPOSIT_STORAGE,
    CREATE_EVENT
}
```

### Guild Ranks

| Rank | Default Permissions |
|------|---------------------|
| Guild Master | All |
| Officer | Invite, Kick, Storage |
| Veteran | Invite |
| Member | None special |
| Recruit | Chat only |

---

## 4. Implementation Plan

### Week 1: Core

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Guild data structures | Database schema |
| 3-4 | Create/disband logic | Commands work |
| 5-6 | Member management | Invite/kick |
| 7 | Rank permissions | Permissions |

### Week 2: Features

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Guild chat | Chat works |
| 10-11 | Guild storage | Storage works |
| 12-13 | Guild UI | Panel works |
| 14 | Integration test | Tests pass |

### Dependencies
- ChatSystem (needs to exist or be created)
- InventorySystem (PRD-021)

---

## 5. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2 weeks |
| Skills | C#, UI, Database |

---

**PRD Status:** Proposed - Awaiting Implementation