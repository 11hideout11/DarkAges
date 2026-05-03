# PRD: Friend System UI

## Introduction

Implement a friend system UI that displays friends list, online status, ability to add/remove friends, whisper from UI, and view friend details. The system should integrate with the existing friend functionality.

## Goals

- Display friends list with online status
- Add/remove friends from UI
- Quick whisper from friend list
- Friend request management
- Block list management

## User Stories

### US-001: Friends list display
**Description:** As a player, I want to see my friends so I can contact them.

**Acceptance Criteria:**
- [ ] Opens via Social button or F key
- [ ] Shows all friends
- [ ] Online players green, offline gray
- [ ] Shows friend level and class
- [ ] Last seen for offline friends

### US-002: Add friend
**Description:** As a player, I want to add friends.

**Acceptance Criteria:**
- [ ] "Add Friend" button
- [ ] Type player name
- [ ] Request sent notification
- [ ] Cannot add if at max friends (100)

### US-003: Remove friend
**Description:** As a player, I want to remove friends.

**Acceptance Criteria:**
- [ ] Right-click friend shows menu
- [ ] "Remove Friend" option
- [ ] Confirmation dialog
- [ ] Removed from list immediately

### US-004: Whisper from list
**Description:** As a player, I want to whisper friends easily.

**Acceptance Criteria:**
- [ ] Double-click friend opens whisper
- [ ] Message input field
- [ ] Send button
- [ ] Returns to game

## Functional Requirements

- FR-1: FriendsListUI.tscn scene
- FR-2: AddFriendComponent
- FR-3: FriendRequestComponent
- FR-4: QuickWhisperComponent

## Success Metrics

- Friends list loads in < 500ms
- Status updates in real-time
- No duplicate friends