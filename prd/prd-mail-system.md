# PRD: Mail System Implementation

## Introduction

DarkAges has no mail system. Players cannot send items or messages to offline players. This PRD implements an in-game mail system for asynchronous communication and item trading.

## Goals

- Send mail to players (including offline)
- Attach items to mail
- Gold attachments
- Inbox/outbox UI

## User Stories

### MAIL-001: Send Mail
**Description:** As a player, I want to send mail.

**Acceptance Criteria:**
- [ ] Recipient by username
- [ ] Message text
- [ ] Optional item attachment
- [ ] Optional gold attachment

### MAIL-002: Receive Mail
**Description:** As a player, I want to receive mail.

**Acceptance Criteria:**
- [ ] New mail notification
- [ ] Inbox with mail list
- [ ] Read mail content
- [ ] Collect attachments

### MAIL-003: Mail UI
**Description:** As a player, I want to manage mail.

**Acceptance Criteria:**
- [ ] Inbox tab
- [ ] Outbox tab
- [ ] Delete mail
- [ ] Mark as read

## Functional Requirements

- FR-1: Mail table in ScyllaDB
- FR-2: Mail packets
- FR-3: MailPanel UI

## Non-Goals

- Read receipts
- Mail attachments to players (future)
- Mail taxes

## Technical Considerations

### Database Table
```sql
CREATE TABLE mail (
    mail_id uuid,
    sender_id bigint,
    recipient_name text,
    subject text,
    body text,
    item_id int,
    item_quantity int,
    gold bigint,
    is_read boolean,
    sent_at timestamp,
    PRIMARY KEY (recipient_name, sent_at)
);
```

## Success Metrics

- Mail sent and received
- Attachments collected

## Open Questions

- Mail expiration?
- Max mail size?