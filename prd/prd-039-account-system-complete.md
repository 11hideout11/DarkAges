# PRD-039: Account System - Complete

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** ✅ Complete — AccountSystem.hpp/.cpp with register, auth, session token, ban/unban, statistics
**Priority:** P1-High  
**Category:** Security & Accounts - Player Identity

---

## 1. Introduction/Overview

Implement account management for player identity, authentication, and security. Currently players can connect without accounts, making it impossible to track bans, manage subscriptions, or secure player data.

### Problem Statement
- No account system - players connect with any name
- Can't ban players (no account to ban)
- No way to secure player data
- No subscription or billing integration
- No "forgot password" flow

### Why This Matters for AAA
- Accounts are required for security and banning
- Need identity for cross-server progress
- Required for billing (premium features)
- Players expect account management

---

## 2. Goals

- Account creation with username/password
- Account login flow
- Account linking to characters
- Ban system with account-level bans
- Account security (failed login lockout)
- Guest login option (demo mode)

---

## 3. User Stories

### US-039-001: Account Creation
**Description:** As a new player, I want to create an account so that I can save my progress.

**Acceptance Criteria:**
- [ ] Username: 3-16 chars, alphanumeric + underscore
- [ ] Password: min 8 chars, mixed case + number
- [ ] Email required (for recovery)
- [ ] Account created in database
- [ ] Verification email sent

### US-039-002: Account Login
**Description:** As a player, I want to log in so that I can access my characters.

**Acceptance Criteria:**
- [ ] Username + password authentication
- [ ] Session token generated
- [ ] Token valid for 7 days
- [ ] Failed login shows error
- [ ] 5 failed attempts locks for 15 min

### US-039-003: Character Binding
**Description:** As a player, I want my characters to be attached to my account.

**Acceptance Criteria:**
- [ ] Characters linked to account_id
- [ ] Per-account character limit: 10
- [ ] Can delete characters
- [ ] Characters persist across sessions
- [ ] Cannot transfer characters

### US-039-004: Account Ban
**Description:** As an admin, I want to ban players so that I can enforce rules.

**Acceptance Criteria:**
- [ ] Ban by account_id
- [ ] Ban types: temp (24h-7d), permanent
- [ ] Ban reason stored
- [ ] Banned player can't login
- [ ] Ban appears in login error

### US-039-005: Guest Mode
**Description:** As a demo user, I want to try the game without creating an account.

**Acceptance Criteria:**
- [ ] "Play as Guest" option
- [ ] Guest account created with UUID
- [ ] Guest data separate from accounts
- [ ] Guest can convert to account
- [ ] Guest characters deleted after 30 days

### US-039-006: Session Management
**Description:** As a player, I want to manage my sessions so that I can log out everywhere.

**Acceptance Criteria:**
- [ ] Logout disconnects session
- [ ] "Kick all sessions" available
- [ ] Session history visible
- [ ] Same account, max 3 sessions

---

## 4. Functional Requirements

- FR-039-1: Account database table (id, username, password_hash, email, created_at)
- FR-039-2: Password hashing (bcrypt or argon2)
- FR-039-3: Session token generation
- FR-039-4: Character-Account linking
- FR-039-5: Ban system (account_bans table)
- FR-039-6: Guest account creation
- FR-039-7: Rate limiting on auth endpoints

---

## 5. Non-Goals

- No email verification in v1 (optional)
- No "forgot password" email in v1
- No OAuth/Social login in v1
- No two-factor authentication in v1
- No account merging in v1

---

## 6. Technical Considerations

- Passwords never stored in plain text
- Sessions use JWT or similar
- Account data in production DB
- Rate limit auth endpoints
- Log all auth events

### Dependencies
- Production Database (PRD-018)
- SaveSystem (GAP-014)
- Anti-Cheat (GAP-012)

---

## 7. Success Metrics

- Account creation in <2 clicks
- Failed login shows clear error
- Ban enforcement is immediate
- No account impersonation possible
- Session handling is secure

---

## 8. Open Questions

- Password requirements strictness?
- Account merge policy?
- Family sharing?