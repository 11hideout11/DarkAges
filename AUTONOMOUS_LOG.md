# DarkAges Autonomous Iteration Log

All autonomous improvements tracked here. Most recent first.

---

### ✅ 2026-04-16 — AbilitySystem Full Implementation
- **Task:** Replace AbilitySystem stubs with real casting, cooldown, mana logic; wire into CombatSystem
- **Status:** SUCCESS
- **Branch:** `feature/ability-system-implementation`
- **Changes:** 5 files, +332/-25 lines.
  1. castAbility(): validates ability, checks mana/range/cooldown, applies damage/heal effects
  2. getAbility(): real lookup by abilityId from registered abilities
  3. hasMana(): checks Mana component on caster entity
  4. Wire CombatSystem ABILITY case (replaces TODO stub)
  5. Expands tests from 3 to 6 cases with 23 assertions
- **Pitfall:** Agent added abilityId to AbilityDefinition constructor but tests used old 5-arg signature — fixed by adding 5-arg overload.
- **Validation:** Build PASS, Tests PASS (127 cases, 829 assertions)

---

### ✅ 2026-04-16 — Anti-Cheat No-Clip Collision Detection
- **Task:** Implement detectNoClip() using SpatialHash for world geometry collision
- **Status:** SUCCESS
- **Branch:** `feature/ability-system-implementation`
- **Changes:** 4 files, +297/-5 lines.
  1. Added SpatialHash* member to AntiCheatSystem with setSpatialHash() setter
  2. detectNoClip(): queries spatial hash, checks XZ circle + Y-axis overlap with StaticTag+BoudingVolume
  3. Wired spatial hash in ZoneServer::initializeAntiCheat
  4. 5 test cases, 21 assertions
- **Pitfalls:**
  - EnTT tag components use `registry.has<T>()` not `try_get<T>()` — wrong EnTT API used
  - ViolationSeverity::HIGH doesn't exist — should use CRITICAL
  - Test distances too fast for speed hack detection — needed 1000ms dtMs
  - Friend class approach for private method broke — rewrote tests to use public validateMovement()
- **Validation:** Build PASS, Tests PASS (130 cases, 841 assertions)

---

### ✅ 2026-04-16 — sendCombatEvent Packet Sending
- **Task:** Send EventPacket to clients in ZoneServer::sendCombatEvent
- **Status:** SUCCESS
- **Branch:** `feature/ability-system-implementation`
- **Changes:** 1 file, +18/-4 lines. Uses ProtobufProtocol::createDamageEvent pattern from validateAndApplyInput.
- **Validation:** Build PASS, Tests PASS (130 cases, 841 assertions)

---

### ✅ 2026-04-15 — PacketValidator Test Fixes
- **Task:** Fix 4 test failures in TestPacketValidator.cpp
- **Status:** SUCCESS
- **Branch:** `autonomous/respawn-timer`
- **Changes:** 1 file, +17/-9 lines.
  1. ClampPosition: compared fixed-point values against fixed-point-converted bounds (1000x offset); now uses float world bounds directly
  2. ValidateSpeed: missing negative speed rejection
  3. ValidateAbilityId: rejected ID 0 (basic attack); now allows it
  4. ValidatePlayerName: auto-fixed empty names; now rejects them
- **Validation:** Build PASS, Tests PASS (104 cases, 789 assertions)

---

### ✅ 2026-04-14 — Respawn Timer Implementation
- **Task:** Implement respawn timer (replaces TODO at ZoneServer.cpp:639)
- **Status:** SUCCESS
- **Branch:** `autonomous/respawn-timer`
- **PR:** https://github.com/Aycrith/DarkAges/pull/2
- **Changes:** 2 files, +48 lines. Added PendingRespawn queue, 5s delay, health restore, spawn teleport.
- **Validation:** Build PASS, Tests PASS (96 cases, 742 assertions)

---

### ✅ 2026-04-14 — Linux Build Fix
- **Task:** Fix compilation on Linux (g++ C++20)
- **Status:** SUCCESS
- **Branch:** `autonomous/fix-linux-build`
- **PR:** https://github.com/Aycrith/DarkAges/pull/1
- **Changes:** 3 files, +45/-5 lines. Missing includes, conditional test compilation, incomplete Redis stub.
- **Validation:** Build PASS, Tests PASS (96 cases, 742 assertions)

---


---

### ✅ 2026-04-15 — Anti-Cheat Event Logging + Ban Persistence
- **Task:** Implement anti-cheat event logging to ScyllaDB and ban persistence to Redis
- **Status:** SUCCESS
- **Branch:** `feature/security-anticheat-logging`
- **Changes:** 4 files, +130/-7 lines. Added AntiCheatEvent struct, logAntiCheatEvent/logAntiCheatEventsBatch methods in ScyllaManager, implemented in ZoneServer::onCheatDetected, Redis ban storage.
- **Validation:** Build PASS, Tests PASS (124 cases, 817 assertions)

---

### ✅ 2026-04-15 — Damage/Hit Packet Sending
- **Task:** Send damage events and hit confirmations to clients
- **Status:** SUCCESS
- **Branch:** `feature/network-damage-packet-sending`
- **Changes:** 1 file, +21/-5 lines. Used ProtobufProtocol::createDamageEvent() to send to target and attacker.
- **Validation:** Build PASS, Tests PASS (124 cases, 817 assertions)

---

### ✅ 2026-04-15 — RedisManager Connection Pool Extraction
- **Task:** Extract connection pool from RedisManager.cpp (1655 lines)
- **Status:** SUCCESS
- **Branch:** `feature/refactor-connection-pool`
- **Changes:** 7 files, +313/-204 lines. Created ConnectionPool.hpp/cpp (289 lines), reduced RedisManager by 206 lines.
- **Pitfall:** Agent modified src/server/CMakeLists.txt instead of root CMakeLists.txt — fixed manually.
- **Validation:** Build PASS, Tests PASS (124 cases, 817 assertions)

