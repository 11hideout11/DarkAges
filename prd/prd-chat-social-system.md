# PRD: Chat & Social System

## Introduction

Implement a comprehensive chat system and social features that allow players to communicate via multiple channels, form social connections, and receive notifications. This includes local chat, party chat, guild chat, global/say/shout, whispers/DMs, and friend lists.

## Goals

- Implement all standard MMO chat channels (Say, Shout, Party, Guild, Whisper, System)
- Add friend system with online status and add/remove functionality
- Add ignore/block list to filter unwanted communications
- Add chat commands (/party, /guild, /whisper, /friend, /ignore)
- Integrate with existing PartyComponent and GuildComponent
- Add chat history and quick-reply functionality

## User Stories

### US-001: Multi-channel chat
**Description:** As a player, I want to communicate through different channels so I can talk to nearby players, my party, guild, or send private messages.

**Acceptance Criteria:**
- [ ] ChatChannel enum: Say (15m), Shout (100m), Party, Guild, Whisper, System
- [ ] ChatComponent storing message history per channel (last 50 messages)
- [ ] Server-side message routing to appropriate recipients
- [ ] ChatCommandComponent parsing / commands
- [ ] Chat color coding per channel (local=white, party=green, guild=blue, whisper=yellow)

### US-001: Basic mob AI behaviors
- Create reusable AI behavior trees that can be composed for different enemy types

### US-002: Friend system
**Description:** As a player, I want to add friends and see when they're online so I can play together.

**Acceptance Criteria:**
- [ ] FriendComponent with friend list (max 100), pending requests
- [ ] Online/offline status tracking via presence system
- [ ] Friend request sent → pending → accepted flow
- [ ] In-game notification on friend login
- [ ] /friend add <name> and /friend list commands

### US-003: Ignore/block list
**Description:** As a player, I want to block other players so I don't receive their messages.

**Acceptance Criteria:**
- [ ] IgnoreListComponent storing blocked player IDs
- [ ] Blocked players cannot send whispers
- [ ] Blocked players cannot see blocked player in /who
- [ ] /ignore add <name> and /ignore list commands
- [ ] Muted players show "(muted)" prefix in chat

### US-004: Guild and party chat integration
**Description:** As a party/guild member, I want dedicated party and guild chat channels so I can coordinate with my team.

**Acceptance Criteria:**
- [ ] Party chat shows party members only
- [ ] Guild chat shows guild members across zones
- [ ] Client-side channel switching via Tab or /commands
- [ ] Chat history persists per session

### US-005: Whispers/Direct messages
**Description:** As a player, I want to send private messages to other players so I can communicate one-on-one.

**Acceptance Criteria:**
- [ ] Whisper format: /w <player> <message> or /whisper <player> <message>
- [ ] Recipient must be online to receive
- [ ] "Player not found" feedback if offline
- [ ] Reply shortcut: /r <message> responds to last whisper
- [ ] Conversation view showing whisper history

## Functional Requirements

- FR-1: ChatChannel enum and ChatComponent with history buffers
- FR-2: ChatMessage serialization with channel, sender, content, timestamp
- FR-3: ChatCommandComponent parsing / commands
- FR-4: FriendComponent and FriendRequestComponent
- FR-5: IgnoreListComponent and filtering logic
- FR-6: Party/Guild chat routing via member lists
- FR-7: Whisper routing via online player lookup
- FR-8: Quick-reply state for /r command

## Non-Goals

- No cross-realm or global chat (region-only)
- No chat moderation/auto-moderation (manual only)
- No voice chat (outside scope)
- No chat emojis or rich media (text-only)
- No offline message storage (require online)

## Technical Considerations

- Chat messages use existing Protocol serialization
- Server acts as message broker for all routes
- Presence tracked via PlayerConnect/Disconnect events
- Friend status updates on login/logout

## Success Metrics

- Chat latency < 100ms end-to-end
- 1000+ messages/minute capacity
- Friend list loads < 500ms on login

## Open Questions

- Should chat be stored in database for history retrieval?
- Maximum message length? (current: 255 chars)
- Profanity filter or manual reports only?