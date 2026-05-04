     1|#pragma once
     2|
     3|#include "ecs/CoreTypes.hpp"
     4|#include "physics/SpatialHash.hpp"
     5|#include "physics/MovementSystem.hpp"
     6|#include "physics/NavigationGrid.hpp"
     7|#include "netcode/NetworkManager.hpp"
     8|#include "db/RedisManager.hpp"
     9|#include "db/ScyllaManager.hpp"
    10|#include "zones/AreaOfInterest.hpp"
    11|#include "zones/AuraProjection.hpp"
    12|#include "zones/EntityMigration.hpp"
    13|#include "zones/PlayerManager.hpp"
    14|#include "zones/ZoneHandoff.hpp"
    15|#include "zones/ReplicationOptimizer.hpp"
    16|#include "zones/CombatEventHandler.hpp"
    17|#include "zones/AuraZoneHandler.hpp"
    18|#include "zones/InputHandler.hpp"
    19|#include "zones/PerformanceHandler.hpp"
    20|#include "zones/AntiCheatHandler.hpp"
    21|#include "zones/ZoneObjectiveSystem.hpp"
    22|#include "zones/ZoneDifficultySystem.hpp"
    23|#include "combat/PositionHistory.hpp"
    24|#include "combat/LagCompensatedCombat.hpp"
    25|#include "combat/CombatSystem.hpp"
    26|#include "combat/StatusEffectSystem.hpp"
    27|#include "combat/ProjectileSystem.hpp"
    28|#include "combat/TargetLockSystem.hpp"
    29|#include "combat/NPCAISystem.hpp"
    30|#include "combat/ExperienceSystem.hpp"
    31|#include "combat/AbilitySystem.hpp"
    32|#include "combat/ItemSystem.hpp"
    33|#include "combat/ChatSystem.hpp"
    34|#include "combat/QuestSystem.hpp"
    35|#include "combat/CraftingSystem.hpp"
    36|#include "combat/TradeSystem.hpp"
    37|#include "combat/ZoneEventSystem.hpp"
    38|#include "combat/WorldProgressionSystem.hpp"
    39|#include "combat/ProgressionCalculator.hpp"
    40|#include "combat/DialogueSystem.hpp"
    41|#include "combat/SpawnSystem.hpp"
    42|#include "security/AntiCheat.hpp"
#include "combat/DailyChallengeSystem.hpp"
#include "combat/NewGamePlusSystem.hpp"
    43|#include "profiling/PerfettoProfiler.hpp"
    44|#include "profiling/PerformanceMonitor.hpp"
    45|#include "monitoring/MetricsExporter.hpp"
    46|#include "memory/MemoryPool.hpp"
    47|#include "memory/FixedVector.hpp"
    48|#include <memory>
    49|#include <atomic>
    50|#include <chrono>
    51|#include <deque>
    52|#include <unordered_map>
    53|#include <vector>
    54|#include <cstddef>
    55|#include <cstdint>
    56|#include <string>
    57|#include <csignal>
    58|
    59|// [ZONE_AGENT] Main zone server class
    60|// Manages all systems for a single zone/shard
    61|
    62|#include "instrumentation/ServerStateExporter.hpp"
    63|
    64|namespace DarkAges {
    65|
    66|// Snapshot history entry for delta compression
    67|struct SnapshotHistory {
    68|    uint32_t tick;
    69|    std::vector<Protocol::EntityStateData> entities;
    70|    std::chrono::steady_clock::time_point timestamp;
    71|};
    72|
    73|// Client snapshot acknowledgment tracking
    74|struct ClientSnapshotState {
    75|    uint32_t lastAcknowledgedTick{0};      // Last tick client confirmed received
    76|    uint32_t lastSentTick{0};              // Last tick we sent
    77|    uint32_t baselineTick{0};              // Current baseline for delta compression
    78|    uint32_t snapshotSequence{0};          // Monotonic sequence counter
    79|    std::vector<EntityID> pendingRemovals; // Entities to remove in next snapshot
    80|};
    81|
    82|struct ZoneConfig {
    83|    uint32_t zoneId{1};
    84|    uint16_t port{Constants::DEFAULT_SERVER_PORT};
    85|
    86|    // World bounds for this zone
    87|    float minX{Constants::WORLD_MIN_X};
    88|    float maxX{Constants::WORLD_MAX_X};
    89|    float minZ{Constants::WORLD_MIN_Z};
    90|    float maxZ{Constants::WORLD_MAX_Z};
    91|
    92|    // Database connections
    93|    std::string redisHost{"localhost"};
    94|    uint16_t redisPort{Constants::REDIS_DEFAULT_PORT};
    95|    std::string scyllaHost{"localhost"};
    96|    uint16_t scyllaPort{Constants::SCYLLA_DEFAULT_PORT};
    97|
    98|    // Aura projection buffer (overlap with adjacent zones)
    99|    float auraBuffer{Constants::AURA_BUFFER_METERS};
   100|
   101|    // NPC population — if true, zone auto-spawns default NPCs
   102|    bool autoPopulateNPCs{false};
   103|    uint32_t npcCount{10};           // Number of NPCs to spawn
   104|    float npcSpawnRadius{50.0f};     // Spawn radius from zone center
   105|    uint8_t npcBaseLevel{1};         // Base level for spawned NPCs
   106|    uint16_t npcBaseDamage{10};      // Base damage
   107|    uint32_t npcXpReward{50};        // XP per kill
   108|
   109|    // Demo mode configuration
   110|    bool demoMode{false};            // Enable curated demo zone
   111|    std::string zoneConfigPath;      // Path to JSON zone configuration file
   112|    bool enableInstrumentation{false};  // Enable server tick-state export
   113|
   114|    // Difficulty multiplier (1.0f = normal, 1.5f = hard, 2.0f = nightmare).
   115|    // Applied to NPC health, damage, and XP rewards at spawn time.
   116|    float difficultyMultiplier{1.0f};
   117|};
   118|
   119|// [COMBAT_AGENT] Boss encounter configuration loaded from zone JSON
   120|struct BossEncounterConfig {
   121|    uint32_t bossArchetypeId{0};                    // NPC archetype ID for the boss
   122|    uint8_t bossLevel{10};                          // Boss level
   123|    std::string bossName;                           // Display name
   124|    std::vector<uint32_t> abilityIds;               // All abilities the boss can use
   125|    struct PhaseConfig {
   126|        std::string name;                          // Phase display name
   127|        float healthThreshold{0.0f};                // Health % to ENTER this phase (0.0-1.0)
   128|        float damageMultiplier{1.0f};               // Damage scaling in this phase
   129|        std::vector<uint32_t> abilityIds;           // Abilities active in this phase
   130|    };
   131|    std::vector<PhaseConfig> phases;                // Phase progression (max 4)
   132|};
   133|
   134|struct TickMetrics {
   135|    uint64_t tickCount{0};
   136|    uint64_t totalTickTimeUs{0};
   137|    uint64_t maxTickTimeUs{0};
   138|    uint64_t overruns{0};
   139|
   140|    // Component times (microseconds)
   141|    uint64_t networkTimeUs{0};
   142|    uint64_t physicsTimeUs{0};
   143|    uint64_t gameLogicTimeUs{0};
   144|    uint64_t replicationTimeUs{0};
   145|
   146|    void reset() {
   147|        *this = TickMetrics{};
   148|    }
   149|};
   150|
   151|// [ZONE_AGENT] Main server class
   152|class ZoneServer {
   153|public:
   154|    ZoneServer();
   155|    ~ZoneServer();
   156|
   157|    // Initialize all systems
   158|    bool initialize(const ZoneConfig& config);
   159|
   160|    // Run main loop (blocking)
   161|    void run();
   162|
   163|    // Request shutdown (can be called from signal handlers)
   164|    void requestShutdown();
   165|
   166|    // Stop server (internal use)
   167|    void stop();
   168|
   169|    // Check if server is running
   170|    [[nodiscard]] bool isRunning() const { return running_; }
   171|
   172|    // Check if shutdown was requested
   173|    [[nodiscard]] bool isShutdownRequested() const { return shutdownRequested_; }
   174|
   175|    // Single tick update (for external loop control)
   176|    bool tick();
   177|
   178|    // Get current server time in milliseconds
   179|    [[nodiscard]] uint32_t getCurrentTimeMs() const;
   180|
   181|    // Get current tick number
   182|    [[nodiscard]] uint32_t getCurrentTick() const { return currentTick_; }
   183|
   184|    // Access subsystems
   185|    [[nodiscard]] Registry& getRegistry() { return registry_; }
   186|    [[nodiscard]] NetworkManager& getNetwork() { return *network_; }
   187|    [[nodiscard]] SpatialHash& getSpatialHash() { return spatialHash_; }
   188|    [[nodiscard]] NavigationGrid& getNavigationGrid() { return navigationGrid_; }
   189|    [[nodiscard]] MovementSystem& getMovementSystem() { return movementSystem_; }
   190|    [[nodiscard]] RedisManager& getRedis() { return *redis_; }
   191|
   192|    // Get metrics
   193|    [[nodiscard]] const TickMetrics& getMetrics() const { return metrics_; }
   194|
   195|    // Get configuration
   196|    [[nodiscard]] const ZoneConfig& getConfig() const { return config_; }
   197|
   198|    // Spawn player entity
   199|    EntityID spawnPlayer(ConnectionID connectionId, uint64_t playerId,
   200|                        const std::string& username, const Position& spawnPos);
   201|
   202|    // Spawn NPC entity
   203|    EntityID spawnNPC(const Position& spawnPos, uint8_t level, uint16_t baseDamage,
   204|                      float aggroRange, float leashRange, float attackRange,
   205|                      uint32_t xpReward, uint32_t respawnTimeMs);
   206|
   207|    // Spawn NPC from a spawn group (managed respawn via SpawnSystem)
   208|    EntityID spawnFromGroup(const Position& spawnPos, uint8_t level,
   209|                            uint16_t baseDamage, float aggroRange, float leashRange,
   210|                            float attackRange, uint32_t xpReward, uint32_t respawnTimeMs,
   211|                            uint32_t spawnGroupId, uint32_t npcTemplateId);
   212|
   213|    // Populate zone with NPCs based on config
   214|    void populateNPCs();
   215|
   216|    // Load demo zone configuration from JSON file
   217|    bool loadDemoConfig(const std::string& configPath);
   218|
   219|    // Populate NPCs from loaded demo configuration
   220|    void populateNPCsFromDemoConfig();
   221|
   222|    // Despawn entity
   223|    void despawnEntity(EntityID entity);
   224|
   225|    // AOI debugging
   226|    [[nodiscard]] AreaOfInterestSystem& getAOISystem() { return areaOfInterestSystem_; }
   227|
   228|    // [ZONE_AGENT] Accessors for extracted handler classes\n    [[nodiscard]] std::unordered_map<ConnectionID, EntityID>* getConnectionToEntityPtr() { return &connectionToEntity_; }\n    [[nodiscard]] std::unordered_map<EntityID, ConnectionID>* getEntityToConnectionPtr() { return &entityToConnection_; }\n    [[nodiscard]] TickMetrics& getMetricsRef() { return metrics_; }\n    [[nodiscard]] CombatSystem* getCombatSystemPtr() { return &combatSystem_; }\n    [[nodiscard]] StatusEffectSystem& getStatusEffectSystem() { return statusEffectSystem_; }\n    [[nodiscard]] ProjectileSystem& getProjectileSystem() { return projectileSystem_; }\n    [[nodiscard]] NPCAISystem& getNPCAISystem() { return npcAISystem_; }\n    [[nodiscard]] ExperienceSystem& getExperienceSystem() { return experienceSystem_; }\n    [[nodiscard]] LootSystem& getLootSystem() { return lootSystem_; }\n    [[nodiscard]] AbilitySystem& getAbilitySystem() { return abilitySystem_; }\n    [[nodiscard]] ItemSystem& getItemSystem() { return itemSystem_; }\n    [[nodiscard]] QuestSystem& getQuestSystem() { return questSystem_; }\n    [[nodiscard]] ChatSystem& getChatSystem() { return chatSystem_; }\n    [[nodiscard]] CraftingSystem& getCraftingSystem() { return craftingSystem_; }\n    [[nodiscard]] TradeSystem& getTradeSystem() { return tradeSystem_; }\n    [[nodiscard]] ZoneEventSystem& getZoneEventSystem() { return zoneEventSystem_; }\n    [[nodiscard]] DialogueSystem& getDialogueSystem() { return dialogueSystem_; }\n    [[nodiscard]] SpawnSystem& getSpawnSystem() { return spawnSystem_; }\n    [[nodiscard]] ZoneObjectiveSystem& getZoneObjectiveSystem() { return zoneObjectiveSystem_; }\n    [[nodiscard]] ZoneDifficultySystem& getDifficultySystem() { return difficultySystem_; }\n    [[nodiscard]] DailyChallengeSystem& getDailyChallengeSystem() { return dailyChallengeSystem_; }\n    [[nodiscard]] LagCompensator* getLagCompensatorPtr() { return &lagCompensator_; }
   229|    [[nodiscard]] MovementSystem& getMovementSystemRef() { return movementSystem_; }
   230|    [[nodiscard]] Security::AntiCheatSystem& getAntiCheatRef() { return antiCheat_; }
   231|    [[nodiscard]] bool isQoSDegraded() const { return qosDegraded_; }
   232|    void setQoSDegraded(bool degraded) { qosDegraded_ = degraded; }
   233|    void setReducedUpdateRate(uint32_t rate) { reducedUpdateRate_ = rate; }
   234|    [[nodiscard]] ReplicationOptimizer& getReplicationOptimizerRef() { return replicationOptimizer_; }
   235|
   236|    // Signal handling setup
   237|    void setupSignalHandlers();
   238|
   239|private:
   240|    // System update phases
   241|    void updateNetwork();
   242|    void updatePhysics();
   243|    void updateGameLogic();
   244|    void updateReplication();
   245|    void updateDatabase();
   246|
   247|    // Client connection handlers
   248|    void onClientConnected(ConnectionID connectionId);
   249|    void onClientDisconnected(ConnectionID connectionId);
   250|    void sendInventorySyncToClient(ConnectionID connectionId, EntityID entity);
   251|
   252|    // [SECURITY_AGENT] Anti-cheat event handling delegated to AntiCheatHandler
   253|
   254|    // [PHASE 3C] Combat processing with lag compensation (delegated to InputHandler)
   255|
   256|    // Combat processing
   257|    void processCombat();
   258|    void onEntityDied(EntityID victim, EntityID killer);
   259|    void sendCombatEvent(EntityID attacker, EntityID target, int16_t damage, const Position& location);
   260|    void logCombatEvent(const HitResult& hit, EntityID victim, EntityID killer);
   261|    void processAttackInput(EntityID entity, const ClientInputPacket& input);
   262|    void processPendingCombatActions();
   263|    void processPendingLockOnRequests();
   264|    void processPendingChatMessages();
   265|    void processPendingQuestActions();
   266|    void processPendingDialogueResponses();
   267|    // Zone advancement — trigger migration when objectives complete
   268|    void onZoneComplete(entt::entity player, uint16_t zoneId);
   269|    uint32_t getNextZoneId(uint32_t currentZone) const;
   270|
   271|    // [COMBAT_AGENT] Boss encounter configuration
   272|    bool loadBossEncounterConfig();
   273|    ZoneEventDefinition buildZoneEventFromBossConfig() const;
   274|
   275|    // Performance monitoring (delegated to PerformanceHandler)
   276|
   277|    // Save player state to database
   278|    void savePlayerState(EntityID entity);
   279|
   280|private:
   281|    // ECS registry
   282|    Registry registry_;
   283|
   284|    // Subsystems
   285|    std::unique_ptr<NetworkManager> network_;
   286|    std::unique_ptr<RedisManager> redis_;
   287|    std::unique_ptr<ScyllaManager> scylla_;
   288|    SpatialHash spatialHash_;
   289|    NavigationGrid navigationGrid_;
   290|    MovementSystem movementSystem_;
   291|    MovementValidator movementValidator_;
   292|    BroadPhaseSystem broadPhaseSystem_;
   293|    ReplicationOptimizer replicationOptimizer_;
   294|    CombatSystem combatSystem_;
   295|    HealthRegenSystem healthRegenSystem_;
   296|    ManaRegenSystem manaRegenSystem_;
   297|    StatusEffectSystem statusEffectSystem_;
   298|    ProjectileSystem projectileSystem_;
   299|    NPCAISystem npcAISystem_;
   300|    ExperienceSystem experienceSystem_;
   301|    LootSystem lootSystem_;
   302|    AbilitySystem abilitySystem_;
   303|    ItemSystem itemSystem_;
   304|    ChatSystem chatSystem_;
   305|    QuestSystem questSystem_;
   306|    CraftingSystem craftingSystem_;
   307|    TradeSystem tradeSystem_;
   308|    ZoneEventSystem zoneEventSystem_;
   309|    DialogueSystem dialogueSystem_;
   310|    SpawnSystem spawnSystem_;
   311|
   312|    // [PROGRESSION_AGENT] World progression tracking (zone locks, completion)
   313|    WorldProgressionSystem worldProgressionSystem_;
   314|
   315|    // [PROGRESSION_AGENT] Equipment/level-based stat recalculation
   316|    ProgressionCalculator progressionCalculator_;
   317|
   318|    // [COMBAT_AGENT] Boss encounter state for zone-based boss events
   319|    BossEncounterConfig currentBossConfig_;
   320|    bool bossEventActive_{false};
   321|    std::string pendingBossZoneEventId_;  // Zone event ID to autostart after NPC population
   322|
   323|    // Pending dialogue data for network transmission (filled by text callback, consumed by responses callback)
   324|    struct PendingDialogue {
   325|        EntityID npcId;
   326|        std::string npcName;
   327|        std::string text;
   328|        bool isEnd;
   329|    };
   330|    std::unordered_map<EntityID, PendingDialogue> pendingDialogueStarts_;
   331|
   332|    LagCompensator lagCompensator_;
   333|
   334|    // [SECURITY_AGENT] Anti-cheat system
   335|    Security::AntiCheatSystem antiCheat_;
   336|
   337|    // Configuration
   338|    ZoneConfig config_;
   339|
   340|    // Loaded demo zone configuration (JSON)
   341|    std::string demoConfigJson_;
   342|
   343|    // State
   344|    std::atomic<bool> running_{false};
   345|    std::atomic<bool> initialized_{false};
   346|    std::atomic<bool> shutdownRequested_{false};
   347|    uint32_t currentTick_{0};
   348|
   349|    // Graceful shutdown implementation
   350|    void shutdown();
   351|    std::chrono::steady_clock::time_point startTime_;
   352|    std::chrono::steady_clock::time_point lastTickTime_;
   353|
   354|    // Metrics
   355|    TickMetrics metrics_;
   356|
   357|    // [PERFORMANCE_AGENT] Profiling and monitoring
   358|    std::unique_ptr<Profiling::PerformanceMonitor> perfMonitor_;
   359|    bool profilingEnabled_{false};
   360|
   361|    // [INSTRUMENTATION_AGENT] Server state snapshot export
   362|    std::unique_ptr<DarkAges::Instrumentation::ServerStateExporter> instrumentationExporter_;
   363|
   364|    // [PERFORMANCE_AGENT] Memory pools for zero-allocation tick processing
   365|    // Heap-allocated to avoid stack overflow (pools are 640KB - 1.25MB each)
   366|    std::unique_ptr<Memory::SmallPool> smallPool_;
   367|    std::unique_ptr<Memory::MediumPool> mediumPool_;
   368|    std::unique_ptr<Memory::LargePool> largePool_;
   369|
   370|    // Per-tick stack allocator for temporary data (1MB) - heap allocated to avoid stack overflow
   371|    std::unique_ptr<Memory::StackAllocator> tempAllocator_;
   372|
   373|    // Memory stats tracking
   374|    struct MemoryStats {
   375|        size_t tempAllocationsPerTick{0};
   376|        size_t tempBytesUsed{0};
   377|        size_t peakTempBytesUsed{0};
   378|    } memoryStats_;
   379|
   380|    // [COMBAT_AGENT] Remote combat actions awaiting processing (RPC)
   381|    std::vector<CombatActionPacket> pendingRemoteCombatActions_;
   382|    // [COMBAT_AGENT] Pending lock-on requests awaiting validation
   383|    std::vector<LockOnRequestPacket> pendingLockOnRequests_;
   384|    std::vector<std::pair<ConnectionID, ChatMessage>> pendingRemoteChatMessages_;
   385|    std::vector<QuestActionPacket> pendingQuestActions_;
   386|    std::vector<Protocol::DialogueResponsePacket> pendingDialogueResponses_;
   387|
   388|public:
   389|    std::unordered_map<ConnectionID, EntityID> connectionToEntity_;
   390|    std::unordered_map<EntityID, ConnectionID> entityToConnection_;
   391|
   392|    // QoS state
   393|    bool qosDegraded_{false};
   394|    uint32_t reducedUpdateRate_{Constants::SNAPSHOT_RATE_HZ};
   395|
   396|    // Snapshot history for delta compression (last ~1 second)
   397|    std::deque<SnapshotHistory> snapshotHistory_;
   398|    static constexpr size_t MAX_SNAPSHOT_HISTORY = 60;  // 1 second at 60Hz
   399|
   400|    // Per-client snapshot state
   401|    std::unordered_map<ConnectionID, ClientSnapshotState> clientSnapshotState_;
   402|
   403|    // Entities that were destroyed this tick (for removal notifications)
   404|    std::vector<EntityID> destroyedEntities_;
   405|
   406|    // Area of Interest system for entity filtering
   407|    AreaOfInterestSystem areaOfInterestSystem_;
   408|
   409|    // [PHASE 4B] Aura projection buffer for zone handoffs
   410|    AuraProjectionManager auraManager_;
   411|
   412|    // Aura sync interval (20Hz)
   413|    static constexpr uint32_t AURA_SYNC_INTERVAL_MS = 50;
   414|    uint32_t lastAuraSyncTime_{0};
   415|
   416|    // Aura integration methods
   417|    void syncAuraState();
   418|    void handleAuraEntityMigration();
   419|    void checkEntityZoneTransitions();
   420|
   421|    // [PHASE 4C] Entity migration
   422|    void onEntityMigrationComplete(EntityID entity, bool success);
   423|
   424|    // [ZONE_AGENT] Player management
   425|    PlayerManager playerManager_;
   426|
   427|    // [ZONE_AGENT] Combat event handling
   428|    CombatEventHandler combatEventHandler_;
   429|
   430|    // [ZONE_AGENT] Aura and zone migration handling
   431|    AuraZoneHandler auraZoneHandler_;
   432|
   433|    // [ZONE_AGENT] Input validation and processing
   434|    InputHandler inputHandler_;
   435|
   436|    // [ZONE_AGENT] Performance monitoring and QoS
   437|    PerformanceHandler performanceHandler_;
   438|
   439|    // [ZONE_AGENT] Anti-cheat initialization and event handling
   440|    AntiCheatHandler antiCheatHandler_;
   441|
   442|    // Entity migration manager
   443|    std::unique_ptr<EntityMigrationManager> migrationManager_;
   444|
   445|    // [PHASE 4E] Zone handoff controller for seamless transitions
   446|    std::unique_ptr<ZoneHandoffController> handoffController_;
   447|
   448|    // [PRD-009] Zone objective system for tracking player objectives
   449|    ZoneObjectiveSystem zoneObjectiveSystem_;
   450|
   451|    // [PHASE 3] Zone difficulty system for Hard Mode scaling
   452|    ZoneDifficultySystem difficultySystem_;
   453|
   454|    // [PRD-040] Daily challenge system for player engagement
   455|    DailyChallengeSystem dailyChallengeSystem_;
   456|
   457|    // Handoff integration methods
   458|    void initializeHandoffController();
   459|    void updateZoneHandoffs();
   460|    void onHandoffStarted(uint64_t playerId, uint32_t sourceZone, uint32_t targetZone, bool success);
   461|    void onHandoffCompleted(uint64_t playerId, uint32_t sourceZone, uint32_t targetZone, bool success);
   462|
   463|    // Respawn timer
   464|    struct PendingRespawn {
   465|        EntityID entity;
   466|        uint64_t respawnTimeMs;  // Absolute time when respawn should occur
   467|        Position spawnPos;
   468|    };
   469|    std::vector<PendingRespawn> pendingRespawns_;
   470|    static constexpr uint32_t RESPAWN_DELAY_MS = 5000;  // 5 seconds
   471|
   472|    // Zone lookup callbacks for handoff controller
   473|    ZoneDefinition* lookupZone(uint32_t zoneId);
   474|    uint32_t findZoneByPosition(float x, float z);
   475|
   476|    // Build a ZoneDefinition from the current server config (avoids repetitive field copying)
   477|    ZoneDefinition buildZoneDefinition() const;
   478|
   479|    void processRespawns();
   480|};
   481|
   482|} // namespace DarkAges
   483|