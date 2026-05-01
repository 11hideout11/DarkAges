# PRD: Friend System Implementation

## Introduction

DarkAges has no friend system. Players cannot add friends, see their online status, or message them directly. This PRD implements a basic friend system.

## Goals

- Send/receive friend requests
- View online friends
- Friend chat/messaging

## User Stories

### FRD-001: Add Friend
**Description:** As a player, I want to add another player as a friend.

**Acceptance Criteria:**
- [ ] Search by username
- [ ] Send friend request
- [ ] Other player receives request
- [ ] Accept/decline

### FRD-002: Friend List
**Description:** As a player, I want to see my friends.

**Acceptance Criteria:**
- [ ] List of all friends
- [ ] Online status shown
- [ ] Current zone shown
- [ ] Click to whisper

### FRD-003: Friend Chat
**Description:** As a friend, I want to message directly.

**Acceptance Criteria:**
- [ ] /w or /whisper command
- [ ] Friend receives message
- [ ] Online friends only

## Functional Requirements

- FR-1: Friend request packet
- FR-2: Friend list stored in database
- FR-3: Online status via Redis

## Non-Goals

- Block list
- Friend groups
- Party invite from friend

## Technical Considerations

### Storage
- Friends stored in ScyllaDB player_profiles
- Online status in Redis session

## Success Metrics

- Friend can be added
- Status displays correctly

## Open Questions

- Max friends limit?
- Cross-zone friends?