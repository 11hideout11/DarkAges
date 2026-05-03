# PRD: Account & Login System

## Introduction

DarkAges has minimal account management. Players connect directly without account creation, authentication, or profile management. This PRD implements a proper account system.

## Goals

- Account registration
- Secure login
- Player profile per account
- Character selection

## User Stories

### ACT-001: Create Account
**Description:** As a new player, I want to create an account.

**Acceptance Criteria:**
- [ ] Username and password
- [ ] Email verification (optional)
- [ ] Account created
- [ ] Auto-login

### ACT-002: Login
**Description:** As a player, I want to login securely.

**Acceptance Criteria:**
- [ ] Username/password
- [ ] Session token returned
- [ ] Failed attempt notification
- [ ] Save credentials option

### ACT-003: Character Selection
**Description:** As a logged-in player, I want to choose a character.

**Acceptance Criteria:**
- [ ] List of characters
- [ ] Create new character
- [ ] Delete character
- [ ] Select and play

### ACT-004: Profile Management
**Description:** As a player, I want to manage my profile.

**Acceptance Criteria:**
- [ ] Change password
- [ ] Manage characters
- [ ] View stats

## Functional Requirements

- FR-1: Account database in ScyllaDB
- FR-2: Login packet
- FR-3: Character slot system

## Non-Goals

- Real email verification
- Social login
- Cloud save

## Technical Considerations

### Account Table
```sql
CREATE TABLE accounts (
    account_id bigint PRIMARY KEY,
    username text UNIQUE,
    password_hash text,
    email text,
    created_at timestamp
);
```

### Character Slots
- Max 3 characters per account
- Store in player_profiles

## Success Metrics

- Account can be created
- Login works with stored credentials

## Open Questions

- Password requirements?
- Character name restrictions?