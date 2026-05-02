// [ZONE_AGENT] Main zone server implementation
// Orchestrates all systems for a single game zone

#include "zones/ZoneServer.hpp"
#include "zones/ReplicationOptimizer.hpp"
#include "zones/EntityMigration.hpp"
 #include "combat/LagCompensatedCombat.hpp"
 #include "combat/TargetLockSystem.hpp"
#ifdef DARKAGES_HAS_PROTOBUF
#include "netcode/ProtobufProtocol.hpp"
#endif
#include "netcode/Protocol.hpp"
#include "ecs/CoreTypes.hpp"
#include "profiling/PerfettoProfiler.hpp"
#include "profiling/PerformanceMonitor.hpp"
#include "monitoring/MetricsExporter.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <ctime>
#include <glm/glm.hpp>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

// Profiling macros that compile out when profiling is disabled
#ifdef ENABLE_PROFILING
    #define ZONE_TRACE_EVENT(name, cat) TRACE_EVENT(name, cat)
    #define ZONE_TRACE_BEGIN(name, cat) TRACE_BEGIN(name, cat)
    #define ZONE_TRACE_END(name, cat) TRACE_END(name, cat)
    #define ZONE_TRACE_INSTANT(name, cat) TRACE_INSTANT(name, cat)
    #define ZONE_TRACE_COUNTER(name, val) TRACE_COUNTER(name, val)
#else
    #define ZONE_TRACE_EVENT(name, cat)
    #define ZONE_TRACE_BEGIN(name, cat)
    #define ZONE_TRACE_END(name, cat)
    #define ZONE_TRACE_INSTANT(name, cat)
    #define ZONE_TRACE_COUNTER(name, val)
#endif

namespace DarkAges {

ZoneServer::ZoneServer()
    : instrumentationExporter_(std::make_unique<DarkAges::Instrumentation::ServerStateExporter>("/tmp/darkages_snapshots/server")),
      smallPool_(std::make_unique<Memory::SmallPool>()),
      mediumPool_(std::make_unique<Memory::MediumPool>()),
      largePool_(std::make_unique<Memory::LargePool>()),
      tempAllocator_(std::make_unique<Memory::StackAllocator>(1024 * 1024)),
      auraManager_(1),  // Default zone ID, will be updated in initialize
      combatEventHandler_(*this),
      auraZoneHandler_(*this),
      inputHandler_(*this),
      performanceHandler_(*this),
      antiCheatHandler_(*this) {
}
ZoneServer::~ZoneServer() = default;

bool ZoneServer::initialize(const ZoneConfig& config) {
    config_ = config;
    instrumentationExporter_->setEnabled(config.enableInstrumentation);

    std::cout << "[ZONE " << config_.zoneId << "] Initializing..." << std::endl;

    // [PHASE 4C] Initialize entity migration manager
    migrationManager_ = std::make_unique<EntityMigrationManager>(config_.zoneId, redis_.get());
    migrationManager_->setZonePortLookup([this](uint32_t zoneId) -> uint16_t {
        // Simple port calculation: base port + zoneId
        // In production, this would query a service registry
        return Constants::DEFAULT_SERVER_PORT + static_cast<uint16_t>(zoneId) - 1;
    });

    std::cout << "[ZONE " << config_.zoneId << "] Entity migration initialized" << std::endl;

    // [ZONE_AGENT] Initialize aura zone handler
    auraManager_ = AuraProjectionManager(config_.zoneId);
    auraZoneHandler_.setRedis(redis_.get());
    auraZoneHandler_.setAuraManager(&auraManager_);
    auraZoneHandler_.setMigrationManager(migrationManager_.get());
    auraZoneHandler_.setConnectionMappings(&connectionToEntity_, &entityToConnection_);
    auraZoneHandler_.initializeAuraManager();

    // [ZONE_AGENT] Initialize combat event handler
    combatEventHandler_.setConnectionMappings(&connectionToEntity_, &entityToConnection_);

    // [ZONE_AGENT] Initialize player manager
    playerManager_.setZoneServer(this);
    playerManager_.setDatabaseConnections(redis_.get(), scylla_.get());
    playerManager_.setZoneId(config_.zoneId);
    std::cout << "[ZONE " << config_.zoneId << "] Player manager initialized" << std::endl;

    // [PRD-009] Initialize zone objective system
    zoneObjectiveSystem_.Initialize(registry_);
    std::cout << "[ZONE " << config_.zoneId << "] Zone objective system initialized" << std::endl;

    // Initialize network
    network_ = std::make_unique<NetworkManager>();
    if (!network_->initialize(config_.port)) {
        std::cerr << "[ZONE " << config_.zoneId << "] Failed to initialize network!" << std::endl;
        return false;
    }

    // Set up network callbacks
    network_->setOnClientConnected([this](ConnectionID connId) {
        onClientConnected(connId);
    });
    network_->setOnClientDisconnected([this](ConnectionID connId) {
        onClientDisconnected(connId);
    });

    // [COMBAT_AGENT] Server-authoritative combat RPC: enqueue remote actions
    network_->setOnCombatAction([this](const CombatActionPacket& action) {
        pendingRemoteCombatActions_.push_back(action);
    });

    // [COMBAT_AGENT] Server-authoritative lock-on targeting: enqueue requests
        network_->setOnLockOnRequest([this](const LockOnRequestPacket& request) {
            pendingLockOnRequests_.push_back(request);
        });

        // Chat: receive inbound chat packets from network
        network_->setOnChatReceived([this](ConnectionID connId, const ChatMessage& msg) {
            pendingRemoteChatMessages_.emplace_back(connId, msg);
        });

        // Quest: receive inbound quest action packets from network
        network_->setOnQuestActionReceived([this](const QuestActionPacket& action) {
            pendingQuestActions_.push_back(action);
        });
        network_->setOnDialogueResponseReceived([this](const Protocol::DialogueResponsePacket& response) {
            pendingDialogueResponses_.push_back(response);
        });

    // Initialize Redis
    redis_ = std::make_unique<RedisManager>();
    if (!redis_->initialize(config_.redisHost, config_.redisPort)) {
        std::cerr << "[ZONE " << config_.zoneId << "] Failed to connect to Redis!" << std::endl;
        // Non-fatal for Phase 0 - can run without Redis
        std::cout << "[ZONE " << config_.zoneId << "] Continuing without Redis..." << std::endl;
    }

    // Initialize ScyllaDB for combat logging
    scylla_ = std::make_unique<ScyllaManager>();
    if (!scylla_->initialize(config_.scyllaHost, config_.scyllaPort)) {
        std::cerr << "[ZONE " << config_.zoneId << "] Failed to connect to ScyllaDB" << std::endl;
        // Non-fatal - can run without logging
        std::cout << "[ZONE " << config_.zoneId << "] Continuing without ScyllaDB..." << std::endl;
    } else {
        std::cout << "[ZONE " << config_.zoneId << "] ScyllaDB connected for combat logging" << std::endl;
    }

    // Initialize ECS - nothing special needed for EnTT

    // Record start time
    startTime_ = std::chrono::steady_clock::now();
    lastTickTime_ = startTime_;

    // Set up combat callbacks - delegate to CombatEventHandler
    combatSystem_.setOnDeath([this](EntityID victim, EntityID killer) {
        // Handle death via CombatEventHandler (respawn, logging, etc.)
        combatEventHandler_.onEntityDied(victim, killer);

        // [GAMEPLAY_AGENT] Award XP to killer if victim is an NPC
        experienceSystem_.awardKillXP(registry_, killer, victim);

        // [GAMEPLAY_AGENT] Generate loot drops if victim has a loot table
        lootSystem_.generateLoot(registry_, victim, killer);

        // [GAMEPLAY_AGENT] Track kill for quest objectives
        if (const NPCStats* stats = registry_.try_get<NPCStats>(victim)) {
            questSystem_.onNPCKilled(registry_, killer,
                                     static_cast<uint32_t>(stats->archetype));
            // [GAMEPLAY_AGENT] Track kill for zone event objectives
            zoneEventSystem_.onNPCKilled(registry_, killer, victim,
                                         static_cast<uint32_t>(stats->archetype),
                                         getCurrentTimeMs());
            // [ZONES_AGENT] Track kill for zone objective progress
            if (registry_.all_of<ZoneObjectiveComponent>(killer)) {
                const auto& objectives = zoneObjectiveSystem_.GetObjectives(killer);
                for (const auto& [objId, progress] : objectives) {
                    if (!progress.Complete) {
                        uint16_t newCount = progress.CurrentCount + 1;
                        zoneObjectiveSystem_.OnObjectiveProgress(killer, objId, newCount);
                    }
                }
            }
        }
    });

    combatSystem_.setOnDamage([this](EntityID attacker, EntityID target,
                                     int16_t damage, const Position& location) {
        combatEventHandler_.sendCombatEvent(attacker, target, damage, location);
        // Track damage for zone event participation
        zoneEventSystem_.onDamageDealt(registry_, attacker, target,
                                       static_cast<uint32_t>(std::abs(damage)),
                                       getCurrentTimeMs());
    });

    // [COMBAT_AGENT] Wire status effect system into combat system for Buff/Debuff/Status abilities
    combatSystem_.setStatusEffectSystem(&statusEffectSystem_);

    // [PHYSICS_AGENT] Wire status effect system into movement system for crowd control
    movementSystem_.setStatusEffectSystem(&statusEffectSystem_);

    // [COMBAT_AGENT] Wire status effect system into regen systems
    healthRegenSystem_.setStatusEffectSystem(&statusEffectSystem_);
    manaRegenSystem_.setStatusEffectSystem(&statusEffectSystem_);

    // [COMBAT_AGENT] Initialize projectile system with spatial hash and damage callback
    projectileSystem_.setSpatialHash(&spatialHash_);

    // [GAMEPLAY_AGENT] Initialize navigation grid for NPC A* pathfinding
    // Grid covers the zone's world bounds with 5-meter resolution
    const float zoneCenterX = (config_.minX + config_.maxX) * 0.5f;
    const float zoneCenterZ = (config_.minZ + config_.maxZ) * 0.5f;
    const float zoneWidth = config_.maxX - config_.minX;
    const float zoneHeight = config_.maxZ - config_.minZ;
    constexpr float NAV_GRID_RESOLUTION = 5.0f; // 5 meters per cell
    navigationGrid_ = NavigationGrid(zoneCenterX, zoneCenterZ, zoneWidth, zoneHeight, NAV_GRID_RESOLUTION);
    npcAISystem_.setNavigationGrid(&navigationGrid_);
    projectileSystem_.setDamageCallback([this](Registry& reg, EntityID target, EntityID attacker,
                                                int16_t damage, uint32_t timeMs) -> bool {
        return combatSystem_.applyDamage(reg, target, attacker, damage, timeMs);
    });

    // [GAMEPLAY_AGENT] Wire NPC AI system with combat system and damage callback
    npcAISystem_.setCombatSystem(&combatSystem_);
    npcAISystem_.setDamageCallback([this](EntityID npc, EntityID target, int16_t damage) {
        Position loc{0, 0, 0, 0};
        if (const auto* pos = registry_.try_get<Position>(target)) {
            loc = *pos;
        }
        combatEventHandler_.sendCombatEvent(npc, target, damage, loc);
    });

    // [GAMEPLAY_AGENT] Wire SpawnSystem with navigation grid for pathfinding-aware spawning
    spawnSystem_.setNavigationGrid(&navigationGrid_);
    spawnSystem_.setZoneId(config_.zoneId);
    spawnSystem_.setSpawnCallback([this](EntityID entity, uint32_t spawnGroupId) {
        // NPC spawned from spawn group — could notify nearby players
    });
    spawnSystem_.setDeathCallback([this](EntityID entity, uint32_t spawnGroupId) {
        // NPC from spawn group died — could track group kill stats
    });

    // [GAMEPLAY_AGENT] Wire loot system callbacks for logging
    lootSystem_.setLootDropCallback([this](EntityID lootEntity, uint32_t itemId,
                                            uint32_t quantity, float gold) {
        // Loot dropped — could send notification to nearby players
    });
    lootSystem_.setLootPickupCallback([this](EntityID player, uint32_t itemId,
                                              uint32_t quantity, float gold) {
        // Loot picked up — could send notification
    });

    // [ZONE_AGENT] Initialize anti-cheat handler
    antiCheatHandler_.setConnectionMappings(&connectionToEntity_, &entityToConnection_);
    antiCheatHandler_.setNetwork(network_.get());
    antiCheatHandler_.setRedis(redis_.get());
    antiCheatHandler_.setScylla(scylla_.get());
    antiCheatHandler_.setAntiCheat(&antiCheat_);
    antiCheatHandler_.initialize();

    // [ZONE_AGENT] Set up extracted handler references
    combatEventHandler_.setNetwork(network_.get());
    combatEventHandler_.setScylla(scylla_.get());
    combatEventHandler_.setCombatSystem(&combatSystem_);
    combatEventHandler_.setLagCompensator(&lagCompensator_);
    combatEventHandler_.setAntiCheat(&antiCheat_);

    auraZoneHandler_.setNetwork(network_.get());
    auraZoneHandler_.setHandoffController(handoffController_.get());

    // [ZONE_AGENT] Initialize input handler
    inputHandler_.setPlayerManager(&playerManager_);
    inputHandler_.setAntiCheat(&antiCheat_);
    inputHandler_.setMovementSystem(&movementSystem_);
    inputHandler_.setNetwork(network_.get());
    inputHandler_.setCombatEventHandler(&combatEventHandler_);
    inputHandler_.setAbilitySystem(&abilitySystem_);
    inputHandler_.setItemSystem(&itemSystem_);
    inputHandler_.setChatSystem(&chatSystem_);

    // [GAMEPLAY_AGENT] Initialize item system with default item database
    itemSystem_.initializeDefaults();

    // [GAMEPLAY_AGENT] Initialize quest system with default quests
    questSystem_.initializeDefaults();
    questSystem_.setItemSystem(&itemSystem_);

    // [QUEST_AGENT] Wire quest progress/completion updates to network layer
    questSystem_.setQuestProgressCallback([this](EntityID player, uint32_t questId,
                                                  uint32_t objectiveIndex, uint32_t current, uint32_t required) {
        auto it = entityToConnection_.find(player);
        if (it != entityToConnection_.end()) {
            QuestUpdatePacket update{};
            update.questId = questId;
            update.objectiveIndex = objectiveIndex;
            update.current = current;
            update.required = required;
            update.status = 0; // in_progress
            network_->sendQuestUpdate(it->second, update);
        }
    });
    questSystem_.setQuestCompleteCallback([this](EntityID player, uint32_t questId) {
        auto it = entityToConnection_.find(player);
        if (it != entityToConnection_.end()) {
            QuestUpdatePacket update{};
            update.questId = questId;
            update.objectiveIndex = 0;
            update.current = 1;
            update.required = 1;
            update.status = 1; // complete
            network_->sendQuestUpdate(it->second, update);
        }
    });

    // [GAMEPLAY_AGENT] Initialize chat system with connection lookup
    chatSystem_.setConnectionLookupCallback([this](EntityID entity) -> ConnectionID {
        auto it = entityToConnection_.find(entity);
        return (it != entityToConnection_.end()) ? it->second : 0;
    });

    // [CHAT_AGENT] Wire chat delivery to network layer
    chatSystem_.setMessageDeliveryCallback([this](ConnectionID connId, const ChatMessage& msg) {
        // Use sendChatMessage to dispatch UDP packet
        network_->sendChatMessage(connId, msg);
    });

    // [GAMEPLAY_AGENT] Initialize crafting system
    craftingSystem_.initializeDefaults();
    craftingSystem_.setItemSystem(&itemSystem_);
    inputHandler_.setCraftingSystem(&craftingSystem_);

    // [GAMEPLAY_AGENT] Initialize trade system
    tradeSystem_.setItemSystem(&itemSystem_);
    tradeSystem_.setChatSystem(&chatSystem_);
    inputHandler_.setTradeSystem(&tradeSystem_);
    inputHandler_.setDialogueSystem(&dialogueSystem_);

    // [GAMEPLAY_AGENT] Initialize zone event system
    zoneEventSystem_.setChatSystem(&chatSystem_);
    zoneEventSystem_.setExperienceSystem(&experienceSystem_);
    zoneEventSystem_.setItemSystem(&itemSystem_);
    zoneEventSystem_.setBossSpawnCallback([this](float x, float z, uint8_t level) -> EntityID {
        Position spawnPos = Position::fromVec3(glm::vec3(x, 0, z));
        return spawnNPC(spawnPos, level, 20, 25.0f, 50.0f, 3.0f, 500, 0);
    });

    // [GAMEPLAY_AGENT] Initialize dialogue system
    dialogueSystem_.setQuestSystem(&questSystem_);
    dialogueSystem_.setItemSystem(&itemSystem_);
    dialogueSystem_.setChatSystem(&chatSystem_);
    
    // Wire dialogue text delivery to chat system ( NPC dialogue appears in chat )
    dialogueSystem_.setDialogueTextCallback([this](EntityID player, const char* npcName, const char* text, bool isEnd) {
        // Send DialogueStart packet to client for non-terminal nodes
        if (!isEnd) {
            const DialogueComponent* dc = registry_.try_get<DialogueComponent>(player);
            if (dc) {
                Protocol::DialogueStartPacket pkt{};
                pkt.npcId = dc->npcEntity;
                pkt.dialogueId = dc->activeTreeId;
                std::strncpy(pkt.npcName, npcName, sizeof(pkt.npcName) - 1);
                std::strncpy(pkt.dialogueText, text, sizeof(pkt.dialogueText) - 1);

                // Collect available response options
                auto available = dialogueSystem_.getAvailableResponses(registry_, player);
                const DialogueTree* tree = dialogueSystem_.getTree(pkt.dialogueId);
                if (tree) {
                    const DialogueNode* currentNode = tree->findNode(dc->currentNodeId);
                    if (currentNode) {
                        for (uint32_t idx : available) {
                            if (idx < currentNode->responseCount) {
                                pkt.options.emplace_back(currentNode->responses[idx].text);
                            }
                        }
                    }
                }

                // Cap option count and set final count
                if (pkt.options.size() > 6) {
                    pkt.options.resize(6);
                }
                pkt.optionCount = static_cast<uint8_t>(pkt.options.size());

                // Resolve connection and send
                auto it = entityToConnection_.find(player);
                if (it != entityToConnection_.end()) {
                    network_->sendDialogueStart(it->second, pkt);
                }
            }
        }

        // Local system message (unchanged)
        std::string msg = std::string(npcName) + ": " + text;
        chatSystem_.sendSystemMessage(registry_, player, msg.c_str(), getCurrentTimeMs());
    });

    // [GAMEPLAY_AGENT] Wire level-up into quest tracking
    experienceSystem_.setLevelUpCallback([this](EntityID player, uint32_t newLevel) {
        questSystem_.onLevelUp(registry_, player, newLevel);
    });

    // [GAMEPLAY_AGENT] Register starter abilities
    {
        AbilityDefinition fireball;
        fireball.abilityId = 1;
        fireball.name = "Fireball";
        fireball.cooldownMs = 3000;
        fireball.manaCost = 15;
        fireball.range = 20.0f;
        fireball.effectType = AbilityEffectType::Damage;
        abilitySystem_.registerAbility(1, fireball);

        AbilityDefinition heal;
        heal.abilityId = 2;
        heal.name = "Heal";
        heal.cooldownMs = 5000;
        heal.manaCost = 20;
        heal.range = 15.0f;
        heal.effectType = AbilityEffectType::Heal;
        abilitySystem_.registerAbility(2, heal);

        AbilityDefinition powerStrike;
        powerStrike.abilityId = 3;
        powerStrike.name = "Power Strike";
        powerStrike.cooldownMs = 2000;
        powerStrike.manaCost = 10;
        powerStrike.range = 3.0f;
        powerStrike.effectType = AbilityEffectType::Damage;
        abilitySystem_.registerAbility(3, powerStrike);
    }

    // [GAMEPLAY_AGENT] Wire loot pickup into inventory
    lootSystem_.setLootPickupCallback([this](EntityID player, uint32_t itemId,
                                              uint32_t quantity, float gold) {
        // Add items to player inventory
        if (itemId > 0) {
            uint32_t overflow = itemSystem_.addToInventory(registry_, player, itemId, quantity);
            if (overflow > 0) {
                std::cout << "[LOOT] Player " << static_cast<uint32_t>(player)
                          << " inventory full — " << overflow << " items lost" << std::endl;
            }
            // Track item collection for quest objectives
            questSystem_.onItemCollected(registry_, player, itemId, quantity);
        }
        // Add gold
        if (gold > 0.0f) {
            if (!registry_.all_of<Inventory>(player)) {
                registry_.emplace<Inventory>(player);
            }
            Inventory& inv = registry_.get<Inventory>(player);
            inv.gold += gold;
        }
    });

    // [ZONE_AGENT] Initialize performance handler
    performanceHandler_.setNetwork(network_.get());
    performanceHandler_.setReplicationOptimizer(&replicationOptimizer_);

    // [ZONE_AGENT] Initialize zone handoff controller via AuraZoneHandler
    auraZoneHandler_.initializeHandoffController();

    // [PERFORMANCE_AGENT] Initialize profiler if enabled
#ifdef ENABLE_PROFILING
    profilingEnabled_ = true;
    std::string tracePath = "zone_" + std::to_string(config_.zoneId) + "_trace.json";
    Profiling::PerfettoProfiler::instance().initialize(tracePath);

    // Start performance monitor
    perfMonitor_ = std::make_unique<Profiling::PerformanceMonitor>();
    perfMonitor_->start(30000);  // 30 second report interval

    std::cout << "[ZONE " << config_.zoneId << "] Profiling enabled, trace: " << tracePath << std::endl;
#endif

    // [DEVOPS_AGENT] Initialize metrics exporter
    uint16_t metricsPort = static_cast<uint16_t>(8080 + config_.zoneId - 1);
    if (!Monitoring::MetricsExporter::Instance().Initialize(metricsPort)) {
        std::cerr << "[ZONE " << config_.zoneId << "] Warning: Failed to start metrics exporter" << std::endl;
        // Non-fatal - server can run without metrics
    } else {
        std::cout << "[ZONE " << config_.zoneId << "] Metrics exporter on port " << metricsPort << std::endl;
    }

    std::cout << "[ZONE " << config_.zoneId << "] Initialization complete" << std::endl;

    // [GAMEPLAY_AGENT] Auto-populate zone with NPCs if configured
    if (config_.autoPopulateNPCs) {
        if (config_.demoMode && !config_.zoneConfigPath.empty()) {
            // Load demo configuration from JSON file
            if (loadDemoConfig(config_.zoneConfigPath)) {
                populateNPCsFromDemoConfig();
            } else {
                std::cerr << "[ZONE " << config_.zoneId << "] Failed to load demo config, falling back to default NPCs" << std::endl;
                populateNPCs();
            }
        } else if (config_.demoMode) {
            // Demo mode without config file — use default demo settings
            std::cout << "[ZONE " << config_.zoneId << "] Demo mode active — spawning default demo NPCs" << std::endl;
            populateNPCs();
        } else {
            populateNPCs();
        }
    }

    initialized_ = true;
    return true;
}

void ZoneServer::run() {
 std::cout << "[ZONE " << config_.zoneId << "] Starting main loop..." << std::endl;

 setupSignalHandlers();
 running_ = true;
 shutdownRequested_ = false;

 // Main game loop at 60Hz (16.67ms per tick)
 constexpr auto tickInterval = std::chrono::microseconds(16667);
 auto nextTick = std::chrono::steady_clock::now();
 uint32_t tickCount = 0;

 std::cout << "[ZONE " << config_.zoneId << "] Server running at 60Hz on port " << config_.port << std::endl;

 // [DEMO_AGENT] In demo mode, start zone event immediately for continuous play
 if (config_.demoMode) {
 // Register the showcase world boss event (Event ID 99)
 ZoneEventDefinition bossEvent{};
 bossEvent.eventId = 99;
 std::strncpy(bossEvent.name, "World Boss: Shadow Ogre", sizeof(bossEvent.name) - 1);
 std::strncpy(bossEvent.description, "A powerful ogre has emerged!", sizeof(bossEvent.description) - 1);
 bossEvent.type = ZoneEventType::WorldBoss;
 bossEvent.requiredPlayers = 1; // Min players to auto-start
 bossEvent.maxParticipants = 100;
 bossEvent.phaseCount = 1;
 bossEvent.phases[0].phaseId = 1;
 std::strncpy(bossEvent.phases[0].name, "The Battle Begins", sizeof(bossEvent.phases[0].name) - 1);
 bossEvent.phases[0].durationMs = 300000; // 5 minute phase
 bossEvent.phases[0].bossNpcArchetypeId = static_cast<uint32_t>(NPCArchetype::Boss); // Boss archetype
 bossEvent.phases[0].bossLevel = 5;
 bossEvent.phases[0].objectiveCount = 1;
 bossEvent.phases[0].objectives[0].type = EventObjectiveType::KillBoss;
 bossEvent.phases[0].objectives[0].requiredCount = 1;
 std::strncpy(bossEvent.phases[0].objectives[0].description, "Defeat the Shadow Ogre",
 sizeof(bossEvent.phases[0].objectives[0].description) - 1);
 bossEvent.cooldownMs = 0; // No cooldown for demo
 bossEvent.spawnX = 0.0f;
 bossEvent.spawnZ = 0.0f;
 bossEvent.spawnRadius = 10.0f;
 bossEvent.reward.xpReward = 500;
 bossEvent.reward.goldReward = 100;
 bossEvent.reward.bonusItemId = 3; // Iron Longsword
 bossEvent.reward.bonusItemQuantity = 1;
 zoneEventSystem_.registerEvent(bossEvent);

 // Start the event immediately
 uint32_t currentTime = getCurrentTimeMs();
 zoneEventSystem_.startEvent(registry_, 99, currentTime);
 chatSystem_.broadcastSystemMessage(registry_, "[EVENT] World Boss Event: Shadow Ogre has appeared!",
 currentTime);

 std::cout << "[ZONE " << config_.zoneId << "] [DEMO] World boss event started" << std::endl;
 }

 while (running_) {
 auto frameStart = std::chrono::steady_clock::now();

        // Execute one game tick
        tick();
        tickCount++;

        // Frame rate limiting
        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);

        if (elapsed < tickInterval) {
            // Sleep to maintain 60Hz
            std::this_thread::sleep_for(tickInterval - elapsed);
        } else if (elapsed > tickInterval * 2) {
            // Frame overrun warning (only log every 60 ticks to avoid spam)
            if (tickCount % 60 == 0) {
                std::cerr << "[ZONE " << config_.zoneId << "] Tick overrun: " << elapsed.count()
                          << " us (budget: " << tickInterval.count() << " us)" << std::endl;
            }
        }

        nextTick += tickInterval;
    }

    std::cout << "[ZONE " << config_.zoneId << "] Main loop ended after " << tickCount << " ticks" << std::endl;
    shutdown();
}

// Global pointer for signal handler access
static ZoneServer* g_zoneServerInstance = nullptr;

void ZoneServer::setupSignalHandlers() {
    // Store global pointer for signal handlers
    g_zoneServerInstance = this;

    #ifdef _WIN32
    // Windows signal handling
    std::signal(SIGINT, [](int) {
        std::cout << "[SIGNAL] SIGINT received, requesting shutdown..." << std::endl;
        if (g_zoneServerInstance) {
            g_zoneServerInstance->requestShutdown();
        }
    });
    std::signal(SIGTERM, [](int) {
        std::cout << "[SIGNAL] SIGTERM received, requesting shutdown..." << std::endl;
        if (g_zoneServerInstance) {
            g_zoneServerInstance->requestShutdown();
        }
    });
    #else
    // Unix signal handling
    struct sigaction sa;
    sa.sa_handler = [](int sig) {
        std::cout << "[SIGNAL] Signal " << sig << " received, requesting shutdown..." << std::endl;
        if (g_zoneServerInstance) {
            g_zoneServerInstance->requestShutdown();
        }
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    #endif
}

void ZoneServer::requestShutdown() {
    if (!shutdownRequested_) {
        shutdownRequested_ = true;
        running_ = false;
        initialized_ = false;
        std::cout << "[ZONE " << config_.zoneId << "] Shutdown requested" << std::endl;
    }
}

void ZoneServer::shutdown() {
    std::cout << "[ZONE " << config_.zoneId << "] Shutting down ZoneServer..." << std::endl;

    // [PERFORMANCE_AGENT] Shutdown profiler
    if (profilingEnabled_) {
        if (perfMonitor_) {
            perfMonitor_->stop();
        }
        Profiling::PerfettoProfiler::instance().shutdown();
    }

    // [ZONE_AGENT] Save all player states via PlayerManager
    playerManager_.saveAllPlayerStates();

    // Flush database writes
    if (scylla_ && scylla_->isConnected()) {
        std::cout << "[ZONE " << config_.zoneId << "] Processing pending database writes..." << std::endl;
        // Process any pending async operations
        scylla_->update();
    }

    // Stop network
    if (network_) {
        network_->shutdown();
    }

    // [DEVOPS_AGENT] Shutdown metrics exporter
    Monitoring::MetricsExporter::Instance().Shutdown();

    std::cout << "[ZONE " << config_.zoneId << "] ZoneServer shutdown complete" << std::endl;
}

void ZoneServer::stop() {
    requestShutdown();
}

void ZoneServer::savePlayerState(EntityID entity) {
    // Get player info
    const PlayerInfo* info = registry_.try_get<PlayerInfo>(entity);
    if (!info) return;

    // Get current position
    const Position* pos = registry_.try_get<Position>(entity);

    uint32_t currentTimeMs = getCurrentTimeMs();

    // [DATABASE_AGENT] Save to Redis for hot state (fast recovery)
    if (redis_ && redis_->isConnected()) {
        PlayerSession session;
        session.playerId = info->playerId;
        session.zoneId = config_.zoneId;
        session.connectionId = info->connectionId;
        if (pos) {
            session.position = *pos;
        }
        // Get health from combat state if available
        if (const CombatState* combat = registry_.try_get<CombatState>(entity)) {
            session.health = combat->health;
        } else {
            session.health = Constants::DEFAULT_HEALTH;
        }
        session.lastActivity = currentTimeMs;
        std::strncpy(session.username, info->username, sizeof(session.username) - 1);
        session.username[sizeof(session.username) - 1] = '\0';

        // Save session with 1 hour TTL
        redis_->savePlayerSession(session, [playerId = info->playerId](bool success) {
            if (!success) {
                std::cerr << "[REDIS] Failed to save session for player " << playerId << std::endl;
            }
        });

        // Also update position separately for quick lookups
        if (pos) {
            redis_->updatePlayerPosition(info->playerId, *pos, currentTimeMs);
        }

        // Update zone player set
        redis_->addPlayerToZone(config_.zoneId, info->playerId);
    }

    // [DATABASE_AGENT] Save to ScyllaDB for persistence (async, fire-and-forget)
    if (scylla_ && scylla_->isConnected()) {
        scylla_->savePlayerState(info->playerId, config_.zoneId, currentTimeMs,
            [playerId = info->playerId](bool success) {
                if (!success) {
                    std::cerr << "[SCYLLA] Failed to persist state for player " << playerId << std::endl;
                }
            });
    }
}

bool ZoneServer::tick() {
    // If zone is neither initialized nor running, gracefully do nothing (used in uninitialized/tests)
    if (!initialized_ && !running_) {
        return false;
    }

    ZONE_TRACE_EVENT("ZoneServer::tick", Profiling::TraceCategory::TOTAL);

    // [PERFORMANCE_AGENT] Reset temporary allocator at start of tick
    // This ensures zero heap allocations during tick processing
    memoryStats_.tempBytesUsed = tempAllocator_->getUsed();
    memoryStats_.peakTempBytesUsed = std::max(memoryStats_.peakTempBytesUsed, memoryStats_.tempBytesUsed);
    tempAllocator_->reset();
    memoryStats_.tempAllocationsPerTick = 0;

    auto tickStart = std::chrono::steady_clock::now();
    uint32_t currentTimeMs = getCurrentTimeMs();

    // Update all systems in order
    {
        ZONE_TRACE_EVENT("updateNetwork", Profiling::TraceCategory::NETWORK);
        updateNetwork();
    }

    {
        ZONE_TRACE_EVENT("updatePhysics", Profiling::TraceCategory::PHYSICS);
        updatePhysics();
    }

    {
        ZONE_TRACE_EVENT("updateGameLogic", Profiling::TraceCategory::GAME_LOGIC);
        updateGameLogic();
    }

    {
        ZONE_TRACE_EVENT("updateReplication", Profiling::TraceCategory::REPLICATION);
        updateReplication();
    }

    {
        ZONE_TRACE_EVENT("updateDatabase", Profiling::TraceCategory::DATABASE);
        updateDatabase();
    }

    // [INSTRUMENTATION] Export snapshot if enabled
    if (instrumentationExporter_ && instrumentationExporter_->isEnabled()) {
        instrumentationExporter_->maybeExport(registry_, currentTick_, getCurrentTimeMs());
    }

    // Calculate tick time
    auto tickEnd = std::chrono::steady_clock::now();
    auto tickTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
        tickEnd - tickStart).count();

    // Update metrics
    metrics_.tickCount++;
    metrics_.totalTickTimeUs += tickTimeUs;
    metrics_.maxTickTimeUs = std::max<uint64_t>(metrics_.maxTickTimeUs, tickTimeUs);

    // Update performance monitor
    if (perfMonitor_) {
        perfMonitor_->recordTickTime(tickTimeUs);
    }

    // [DEVOPS_AGENT] Update Prometheus metrics
    auto& metrics = Monitoring::MetricsExporter::Instance();
    double tickTimeMs = tickTimeUs / 1000.0;
    std::string zoneIdStr = std::to_string(config_.zoneId);
    std::unordered_map<std::string, std::string> zoneLabel = {{"zone_id", zoneIdStr}};

    metrics.TicksTotal().Increment(zoneLabel);
    metrics.TickDurationMs().Set(tickTimeMs, zoneLabel);
    metrics.TickDurationHistogram().Observe(tickTimeMs);

    // Player population metrics
    double playerCount = static_cast<double>(connectionToEntity_.size());
    metrics.PlayerCount().Set(playerCount, zoneLabel);
    metrics.PlayerCapacity().Set(1000.0, zoneLabel);  // Default capacity

    // Memory metrics
    size_t memoryUsed = tempAllocator_->getUsed();
    metrics.MemoryUsedBytes().Set(static_cast<double>(memoryUsed), zoneLabel);
    metrics.MemoryTotalBytes().Set(1024.0 * 1024.0 * 1024.0, zoneLabel);  // 1GB assumption

    // Database connection status
    bool dbConnected = redis_ && redis_->isConnected();
    metrics.DbConnected().Set(dbConnected ? 1.0 : 0.0, zoneLabel);

    // Network metrics - aggregate from all connections
    performanceHandler_.updateNetworkMetrics(metrics, zoneIdStr);

    // Trace counters
    ZONE_TRACE_COUNTER("tick_time_us", static_cast<int64_t>(tickTimeUs));
    ZONE_TRACE_COUNTER("entity_count", 0);
    ZONE_TRACE_COUNTER("player_count", static_cast<int64_t>(connectionToEntity_.size()));

    // Check performance budgets
    performanceHandler_.checkPerformanceBudgets(tickTimeUs);

    // Increment tick counter
    currentTick_++;

    // Frame rate limiting - sleep to maintain 60Hz
    auto targetTickTime = std::chrono::microseconds(Constants::TICK_BUDGET_US);
    auto elapsed = tickEnd - tickStart;
    if (elapsed < targetTickTime) {
        std::this_thread::sleep_for(targetTickTime - elapsed);
    }

    return running_;
}

uint32_t ZoneServer::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count()
    );
}

void ZoneServer::updateNetwork() {
    if (!network_) return;

    auto start = std::chrono::steady_clock::now();

    ZONE_TRACE_EVENT("NetworkManager::update", Profiling::TraceCategory::NETWORK);
    network_->update(getCurrentTimeMs());

    // Process pending inputs
    auto inputs = network_->getPendingInputs();
    // Deduplicate: keep only the latest input per connection per tick
    std::unordered_map<ConnectionID, ClientInputPacket> latestInputs;
    for (const auto& input : inputs) {
        latestInputs[input.connectionId] = input;
    }
    for (const auto& [connId, input] : latestInputs) {
        inputHandler_.onClientInput(input);
    }

    // Process pending respawn requests
    auto respawnRequests = network_->getPendingRespawnRequests();
    for (EntityID entity : respawnRequests) {
        // Default spawn at origin (MVP). In future, use spawn points.
        Position spawnPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        combatSystem_.respawnEntity(registry_, entity, spawnPos);
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    metrics_.networkTimeUs += std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

void ZoneServer::updatePhysics() {
    auto start = std::chrono::steady_clock::now();

    uint32_t currentTimeMs = getCurrentTimeMs();

    // Update spatial hash
    ZONE_TRACE_EVENT("BroadPhase::update", Profiling::TraceCategory::PHYSICS);
    broadPhaseSystem_.update(registry_, spatialHash_);

    // Update movement for all entities
    ZONE_TRACE_EVENT("MovementSystem::update", Profiling::TraceCategory::PHYSICS);
    movementSystem_.update(registry_, currentTimeMs);

    // Collision detection and resolution
    ZONE_TRACE_EVENT("Collision::resolve", Profiling::TraceCategory::PHYSICS);
    std::vector<std::pair<EntityID, EntityID>> collisionPairs;
    broadPhaseSystem_.findPotentialPairs(registry_, spatialHash_, collisionPairs);

    // Resolve collisions
    for (const auto& [entityA, entityB] : collisionPairs) {
        Position* posA = registry_.try_get<Position>(entityA);
        Position* posB = registry_.try_get<Position>(entityB);
        BoundingVolume* bvA = registry_.try_get<BoundingVolume>(entityA);
        BoundingVolume* bvB = registry_.try_get<BoundingVolume>(entityB);

        if (posA && posB && bvA && bvB) {
            movementSystem_.resolveSoftCollision(*posA, *posB, bvA->radius, bvB->radius);
            movementSystem_.resolveSoftCollision(*posB, *posA, bvB->radius, bvA->radius);
        }
    }

    // [COMBAT_AGENT] Record positions for lag compensation (2-second history buffer)
    // This enables server-side rewind hit validation for combat
    lagCompensator_.updateAllEntities(registry_, currentTimeMs);

    // [COMBAT_AGENT] Update projectiles — move, check collisions, expire
    projectileSystem_.update(registry_, currentTimeMs);

    // [GAMEPLAY_AGENT] Update NPC AI — behavior tree tick
    npcAISystem_.update(registry_, currentTimeMs);

    auto elapsed = std::chrono::steady_clock::now() - start;
    metrics_.physicsTimeUs += std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

void ZoneServer::updateGameLogic() {
    auto start = std::chrono::steady_clock::now();

    // Process combat inputs (attacks triggered by client input)
    ZONE_TRACE_EVENT("Combat::process", Profiling::TraceCategory::GAME_LOGIC);
    processPendingCombatActions();
    processPendingLockOnRequests();
    processPendingChatMessages();
    processPendingQuestActions();
    processPendingDialogueResponses();
    combatEventHandler_.processCombat();

    // [COMBAT] Update FSM for all entities (batch)
    combatSystem_.updateFSM(registry_, Constants::DT_SECONDS, getCurrentTimeMs());

    // Health regeneration
    healthRegenSystem_.update(registry_, getCurrentTimeMs());

    // Mana regeneration
    manaRegenSystem_.update(registry_, getCurrentTimeMs());

    // [COMBAT_AGENT] Update status effects (buffs, debuffs, DoTs, HoTs, CC)
    statusEffectSystem_.update(registry_, getCurrentTimeMs());

    // [GAMEPLAY_AGENT] Update loot system — despawn expired loot
    lootSystem_.update(registry_, getCurrentTimeMs());

    // [GAMEPLAY_AGENT] Update trade system — handle timeouts
    tradeSystem_.update(registry_, getCurrentTimeMs());

    // [GAMEPLAY_AGENT] Update zone event system — handle event lifecycle
    zoneEventSystem_.update(registry_, getCurrentTimeMs());

    // Process pending respawns (simple health-restore respawn for unmanaged NPCs)
    combatEventHandler_.processRespawns();

    // [GAMEPLAY_AGENT] Update spawn system — handle spawn group respawns, spawn due NPCs
    spawnSystem_.update(registry_, getCurrentTimeMs());

    // [PHASE 4] Zone transitions and migration
    auraZoneHandler_.checkEntityZoneTransitions();

    // [PHASE 4C] Update entity migrations
    if (migrationManager_) {
        migrationManager_->update(registry_, getCurrentTimeMs());
    }

    // [PHASE 4E] Update zone handoffs
    auraZoneHandler_.updateZoneHandoffs();

    // [PRD-009] Update zone objective system
    zoneObjectiveSystem_.Tick(Constants::DT_SECONDS);

    auto elapsed = std::chrono::steady_clock::now() - start;
    metrics_.gameLogicTimeUs += std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}


void ZoneServer::updateReplication() {
    if (!network_) return;

    auto start = std::chrono::steady_clock::now();

    // [PHASE 4B] Sync aura state with adjacent zones
    ZONE_TRACE_EVENT("Aura::syncState", Profiling::TraceCategory::REPLICATION);
    auraZoneHandler_.syncAuraState();

    // Only send snapshots at snapshot rate (20Hz default)
    static uint32_t lastSnapshotTick = 0;
    uint32_t ticksPerSnapshot = Constants::TICK_RATE_HZ / reducedUpdateRate_;

    if (currentTick_ - lastSnapshotTick >= ticksPerSnapshot) {
        lastSnapshotTick = currentTick_;

        // [ZONE_AGENT] Store full world state in history for delta compression
        // This is the authoritative state for this tick
        std::vector<Protocol::EntityStateData> allEntities;
        auto view = registry_.view<Position, Velocity, Rotation>(entt::exclude<StaticTag>);
        // Note: EnTT view doesn't have size(), we'll grow dynamically

        for (auto entity : view) {
            Protocol::EntityStateData state{};
            state.entity = entity;
            state.position = view.get<Position>(entity);
            state.velocity = view.get<Velocity>(entity);
            state.rotation = view.get<Rotation>(entity);

            if (const CombatState* combat = registry_.try_get<CombatState>(entity)) {
                state.healthPercent = static_cast<uint8_t>(combat->healthPercent());
            } else {
                state.healthPercent = 100;
            }

            state.animState = 0;

            if (registry_.all_of<PlayerTag>(entity)) {
                state.entityType = 0;
            } else if (registry_.all_of<ProjectileTag>(entity)) {
                state.entityType = 1;
            } else if (registry_.all_of<NPCTag>(entity)) {
                state.entityType = 3;
            } else {
                state.entityType = 2;
            }

            allEntities.push_back(state);
        }

        // Store snapshot in history
        SnapshotHistory history;
        history.tick = currentTick_;
        history.entities = std::move(allEntities);
        history.timestamp = std::chrono::steady_clock::now();
        snapshotHistory_.push_back(std::move(history));

        // Clean up old snapshots (keep 1 second worth)
        while (snapshotHistory_.size() > MAX_SNAPSHOT_HISTORY) {
            snapshotHistory_.pop_front();
        }

        // [ZONE_AGENT] Send personalized snapshots to each player using spatial optimization
        for (const auto& [connId, entityId] : connectionToEntity_) {
            const Position* viewerPos = registry_.try_get<Position>(entityId);
            if (!viewerPos) continue;

            // Calculate priorities for all visible entities using spatial hash
            auto priorities = replicationOptimizer_.calculatePriorities(
                registry_, spatialHash_, entityId, *viewerPos, currentTick_);

            // Filter by update rate based on priority (near=20Hz, mid=10Hz, far=5Hz)
            auto entitiesToUpdate = replicationOptimizer_.filterByUpdateRate(priorities, currentTick_);

            // Always include the viewer's own entity so the client knows its state
            entitiesToUpdate.push_back(entityId);

            if (entitiesToUpdate.empty() && destroyedEntities_.empty()) {
                continue;  // Nothing to send this tick
            }

            // Build entity states for this player's visible entities
            auto entityStates = replicationOptimizer_.buildEntityStates(
                registry_, entitiesToUpdate, currentTick_);

            // Apply data culling based on priority to reduce bandwidth
            for (auto& state : entityStates) {
                // Find priority for this entity
                int priority = 2;  // Default to far
                for (const auto& p : priorities) {
                    if (p.entity == state.entity) {
                        priority = p.priority;
                        break;
                    }
                }
                ReplicationOptimizer::cullNonEssentialFields(state, priority);
            }

            // Get client snapshot state for delta compression
            auto& clientState = clientSnapshotState_[connId];

            // Find baseline for delta compression
            uint32_t baselineTick = 0;
            std::vector<Protocol::EntityStateData> baselineEntities;

            if (clientState.lastAcknowledgedTick > 0 &&
                clientState.lastAcknowledgedTick >= currentTick_ - MAX_SNAPSHOT_HISTORY) {
                for (const auto& hist : snapshotHistory_) {
                    if (hist.tick == clientState.lastAcknowledgedTick) {
                        baselineTick = clientState.lastAcknowledgedTick;
                        baselineEntities = hist.entities;
                        break;
                    }
                }
            }

            // Create and send full snapshot in client-compatible format
            uint32_t lastProcessedInput = 0;
            if (NetworkState* netState = registry_.try_get<NetworkState>(entityId)) {
                lastProcessedInput = netState->lastInputSequence;
            }

            auto snapshotData = Protocol::createFullSnapshot(
                currentTick_,
                lastProcessedInput,
                entityStates
            );

            network_->sendSnapshot(connId, snapshotData);

            // Mark entities as updated for this client
            for (EntityID e : entitiesToUpdate) {
                replicationOptimizer_.markEntityUpdated(connId, e, currentTick_);
            }

            clientState.lastSentTick = currentTick_;
            clientState.baselineTick = baselineTick > 0 ? baselineTick : currentTick_;
            clientState.snapshotSequence++;
        }

        destroyedEntities_.clear();
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    metrics_.replicationTimeUs += std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

void ZoneServer::updateDatabase() {
    ZONE_TRACE_EVENT("Database::update", Profiling::TraceCategory::DATABASE);

    // Update Redis async operations
    if (redis_ && redis_->isConnected()) {
        // Process async callbacks and pending commands
        redis_->update();
        // Process pub/sub messages for cross-zone communication
        redis_->processSubscriptions();
    }

    // Update ScyllaDB async operations (process pending writes)
    if (scylla_ && scylla_->isConnected()) {
        scylla_->update();
    }
}

void ZoneServer::onClientConnected(ConnectionID connectionId) {
    std::cout << "[ZONE " << config_.zoneId << "] Client connected: " << connectionId << std::endl;

    // [ZONE_AGENT] Use PlayerManager to register player
    Position spawnPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), getCurrentTimeMs());
    EntityID entity = playerManager_.registerPlayer(
        connectionId,
        static_cast<uint64_t>(connectionId),
        "Player" + std::to_string(connectionId),
        spawnPos
    );

    // [GAMEPLAY_AGENT] Initialize player progression
    registry_.emplace<PlayerProgression>(entity);

    // [GAMEPLAY_AGENT] Initialize chat state
    registry_.emplace<ChatComponent>(entity);

    // [GAMEPLAY_AGENT] Initialize trade state
    registry_.emplace<TradeComponent>(entity);

    // [GAMEPLAY_AGENT] Initialize zone event participation state
    registry_.emplace<ZoneEventComponent>(entity);

    // [GAMEPLAY_AGENT] Initialize dialogue state
    registry_.emplace<DialogueComponent>(entity);

    // [ZONE_AGENT] Update ZoneServer connection mappings for replication
    connectionToEntity_[connectionId] = entity;
    entityToConnection_[entity] = connectionId;

    // [GAMEPLAY_AGENT] Give player their starter kit (inventory, abilities, gear)
    itemSystem_.giveStarterKit(registry_, entity);

 // [GAMEPLAY_AGENT] Give player their starter quests
 questSystem_.giveStarterQuests(registry_, entity, getCurrentTimeMs());

 // [DEMO_AGENT] In demo mode, also give the showcase quest
 if (config_.demoMode) {
 questSystem_.giveDemoQuest(registry_, entity, getCurrentTimeMs());
 }

    // [GAMEPLAY_AGENT] Give player their starter crafting recipes
    craftingSystem_.giveStarterRecipes(registry_, entity);

    // [NETWORK_AGENT] Inform NetworkManager of the assigned entity ID
    // so the connection response contains the correct entity mapping
    network_->setConnectionEntityId(connectionId, entity);

    // [PRD-009] Notify zone objective system that the player has entered this zone
    zoneObjectiveSystem_.OnPlayerEnterZone(
        entity, static_cast<uint16_t>(config_.zoneId), buildZoneDefinition());

    std::cout << "[ZONE " << config_.zoneId << "] Spawned entity " << static_cast<uint32_t>(entity)
              << " for connection " << connectionId << std::endl;
}

void ZoneServer::onClientDisconnected(ConnectionID connectionId) {
    std::cout << "[ZONE " << config_.zoneId << "] Client disconnected: " << connectionId << std::endl;

    // [ZONE_AGENT] Get entity via PlayerManager
    EntityID entity = playerManager_.getEntityByConnection(connectionId);

    if (entity != entt::null) {
        // [PRD-009] Remove player from zone objective tracking before cleanup
        zoneObjectiveSystem_.OnPlayerLeaveZone(entity);

        // [GAMEPLAY_AGENT] Cancel any active trade on disconnect
        if (tradeSystem_.isTrading(registry_, entity)) {
            tradeSystem_.cancelTrade(registry_, entity);
        }

        // [SECURITY_AGENT] Clean up anti-cheat behavior profile
        if (const PlayerInfo* info = registry_.try_get<PlayerInfo>(entity)) {
            antiCheat_.removeProfile(info->playerId);
        }

        // Save state via PlayerManager
        playerManager_.savePlayerState(entity);

        // [ZONE_AGENT] Full cleanup via despawnEntity (destroyedEntities_, replication, lagCompensator)
        despawnEntity(entity);

        // [ZONE_AGENT] Remove mappings via PlayerManager
        playerManager_.removeConnectionMapping(entity);

        // [COMBAT_AGENT] Clear target locks that other entities have on this disconnecting entity
        auto lockView = registry_.view<TargetLock>();
        for (auto [other, lock] : lockView.each()) {
            if (lock.lockedTarget == entity) {
                TargetLockSystem::clearLock(registry_, other);
                lock.lastLockAttempt = 0;
            }
        }
    }

    // Clean up client snapshot state
    clientSnapshotState_.erase(connectionId);

    // [ZONE_AGENT] Clean up replication optimizer tracking for disconnected player
    replicationOptimizer_.removeClientTracking(connectionId);
}

// Input handling delegated to InputHandler (see InputHandler.cpp)

// Anti-cheat initialization and event handling delegated to AntiCheatHandler (see AntiCheatHandler.cpp)

void ZoneServer::processAttackInput(EntityID entity, const ClientInputPacket& input) {
    // Build attack input from client data
    AttackInput attackInput;
    attackInput.type = AttackInput::MELEE;  // Default to melee, could be determined by input
    attackInput.sequence = input.input.sequence;
    attackInput.timestamp = input.input.timestamp_ms;
    attackInput.aimDirection = glm::vec3(
        std::sin(input.input.yaw),
        0.0f,  // Ignore pitch for horizontal aim
        std::cos(input.input.yaw)
    );

     // Integrate confirmed lock-on target as primary attack target
     attackInput.targetEntity = static_cast<uint32_t>(TargetLockSystem::getLockedTarget(registry_, entity));

    // Get RTT for lag compensation
    uint32_t rttMs = 100;  // Default fallback
    if (const NetworkState* netState = registry_.try_get<NetworkState>(entity)) {
        rttMs = netState->rttMs;
        if (rttMs == 0) {
            rttMs = 100;  // Assume 100ms if not measured yet
        }
    }

    // Create lag-compensated attack
    // clientTimestamp = serverReceiveTime - oneWayLatency
    uint32_t oneWayLatency = rttMs / 2;
    uint32_t clientTimestamp = (input.receiveTimeMs > oneWayLatency)
        ? input.receiveTimeMs - oneWayLatency
        : 0;

    LagCompensatedAttack lagAttack;
    lagAttack.attacker = entity;
    lagAttack.input = attackInput;
    lagAttack.clientTimestamp = clientTimestamp;
    lagAttack.serverTimestamp = getCurrentTimeMs();
    lagAttack.rttMs = rttMs;

    // Process attack with lag compensation
    // This rewinds all potential targets to their positions at attack time
    LagCompensatedCombat lagCombat(combatSystem_, lagCompensator_);
    auto hits = lagCombat.processAttackWithRewind(registry_, lagAttack);

    // Send hit results to relevant clients
    for (const auto& hit : hits) {
        if (hit.hit) {
            // [SECURITY_AGENT] Validate combat action for anti-cheat
            Position attackerPos{0, 0, 0, 0};
            if (const Position* pos = registry_.try_get<Position>(entity)) {
                attackerPos = *pos;
            }

            auto combatValidation = antiCheat_.validateCombat(entity, hit.target,
                                                               hit.damageDealt, hit.hitLocation,
                                                               attackerPos, registry_);

            if (combatValidation.detected) {
                std::cerr << "[ANTICHEAT] Combat validation failed: "
                          << combatValidation.description << std::endl;

                // Skip this hit - don't apply damage
                if (combatValidation.severity == Security::ViolationSeverity::CRITICAL ||
                    combatValidation.severity == Security::ViolationSeverity::BAN) {
                    // Kick/ban the player
                    auto connIt = entityToConnection_.find(entity);
                    if (connIt != entityToConnection_.end()) {
                        network_->disconnect(connIt->second, combatValidation.description);
                    }
                    return;
                }
                continue;  // Skip applying this hit
            }

            // [NETWORK_AGENT] Send damage event to target
            auto targetConnIt = entityToConnection_.find(hit.target);
            if (targetConnIt != entityToConnection_.end()) {
#ifdef DARKAGES_HAS_PROTOBUF
                auto damageEvent = Netcode::ProtobufProtocol::createDamageEvent(
                    static_cast<uint32_t>(entity),
                    static_cast<uint32_t>(hit.target),
                    static_cast<int32_t>(hit.damageDealt)
                );
                damageEvent.set_timestamp(getCurrentTimeMs());
                auto eventData = Netcode::ProtobufProtocol::serializeEvent(damageEvent);
                network_->sendEvent(targetConnIt->second, eventData);
#endif
                std::cerr << "[NETWORK] Sent damage event: " << hit.damageDealt
                          << " to entity " << static_cast<uint32_t>(hit.target) << std::endl;
            }

            // [NETWORK_AGENT] Send hit confirmation to attacker
            auto attackerConnIt = entityToConnection_.find(entity);
            if (attackerConnIt != entityToConnection_.end()) {
#ifdef DARKAGES_HAS_PROTOBUF
                auto hitConfirm = Netcode::ProtobufProtocol::createDamageEvent(
                    static_cast<uint32_t>(entity),
                    static_cast<uint32_t>(hit.target),
                    static_cast<int32_t>(hit.damageDealt)
                );
                hitConfirm.set_timestamp(getCurrentTimeMs());
                auto eventData = Netcode::ProtobufProtocol::serializeEvent(hitConfirm);
                network_->sendEvent(attackerConnIt->second, eventData);
#endif
                std::cerr << "[NETWORK] Sent hit confirmation: " << hit.damageDealt
                          << " to attacker entity " << static_cast<uint32_t>(entity) << std::endl;
            }

            // [DATABASE_AGENT] Log combat event for analytics
            // redis_->logCombatEvent(entity, hit.target, hit.damageDealt, lagAttack.serverTimestamp);
        }
    }
}

void ZoneServer::processPendingCombatActions() {
    if (pendingRemoteCombatActions_.empty()) return;

    uint32_t currentTimeMs = getCurrentTimeMs();
    for (const auto& action : pendingRemoteCombatActions_) {
        combatEventHandler_.processCombatAction(action);
    }
    pendingRemoteCombatActions_.clear();
}

void ZoneServer::processPendingLockOnRequests() {
    if (pendingLockOnRequests_.empty()) return;

    uint32_t now = getCurrentTimeMs();
    Registry& registry = getRegistry();

    for (const auto& request : pendingLockOnRequests_) {
        // Resolve player entity from connection
        auto it = connectionToEntity_.find(request.connectionId);
        if (it == connectionToEntity_.end()) continue;
        EntityID playerEntity = it->second;

        // Ensure we have a TargetLock component for rate limiting
        TargetLock* lockComp = registry.try_get<TargetLock>(playerEntity);
        if (!lockComp) {
            lockComp = &registry.emplace<TargetLock>(playerEntity);
        }

        // Rate limiting: reject if last attempt was too recent
        if (lockComp->lastLockAttempt != 0 && now - lockComp->lastLockAttempt < TargetLockSystem::MIN_LOCK_INTERVAL_MS) {
            lockComp->lastLockAttempt = now; // still record this attempt
            network_->sendLockOnFailed(request.connectionId, request.targetEntity, 4); // BUSY
            continue;
        }

        // Record this attempt (successful rate-limit clearance)
        lockComp->lastLockAttempt = now;

        // Validate target exists and has Position
        if (!registry.valid(request.targetEntity) || !registry.try_get<Position>(request.targetEntity)) {
            network_->sendLockOnFailed(request.connectionId, request.targetEntity, 3); // INVALID_TARGET
            continue;
        }

        // Check if target is alive (CombatState health > 0 and not dead)
        if (auto* combat = registry.try_get<CombatState>(request.targetEntity)) {
            if (combat->health <= 0 || combat->isDead) {
                network_->sendLockOnFailed(request.connectionId, request.targetEntity, 2); // NOT_ALIVE
                continue;
            }
        }

        // Range check (~50m max lock range)
        constexpr float MAX_LOCK_RANGE = 50.0f;
        Position* playerPos = registry.try_get<Position>(playerEntity);
        Position* targetPos = registry.try_get<Position>(request.targetEntity);
        if (playerPos && targetPos) {
            float dx = playerPos->x - targetPos->x;
            float dy = playerPos->y - targetPos->y;
            float dz = playerPos->z - targetPos->z;
            float distSq = dx*dx + dy*dy + dz*dz;
            if (distSq > MAX_LOCK_RANGE * MAX_LOCK_RANGE) {
                network_->sendLockOnFailed(request.connectionId, request.targetEntity, 0); // OUT_OF_RANGE
                continue;
            }
        }

        // Success: apply confirmed lock
        lockComp->lockedTarget = request.targetEntity;
        lockComp->lockState = LockState::Confirmed;
        lockComp->lockTimeMs = now;

        network_->sendLockOnConfirmed(request.connectionId, request.targetEntity);
    }

    pendingLockOnRequests_.clear();
}

void ZoneServer::processPendingChatMessages() {
    if (pendingRemoteChatMessages_.empty()) return;

    Registry& registry = getRegistry();
    uint32_t now = getCurrentTimeMs();

    for (const auto& [connId, chat] : pendingRemoteChatMessages_) {
        // Resolve sender entity from connection
        auto it = connectionToEntity_.find(connId);
        if (it == connectionToEntity_.end()) continue;
        EntityID senderEntity = it->second;

        // Submit to ChatSystem for validation, routing, and delivery
        // Note: chat.senderId is 0; ChatSystem will fill from PlayerInfo
        // Content is null-terminated already
        chatSystem_.sendMessage(
            registry,
            senderEntity,
            static_cast<ChatChannel>(chat.channel),
            chat.content,
            now,
            chat.targetId
        );
    }

    pendingRemoteChatMessages_.clear();
}

void ZoneServer::processPendingQuestActions() {
    if (pendingQuestActions_.empty()) return;

    Registry& registry = getRegistry();
    uint32_t now = getCurrentTimeMs();

    for (const auto& action : pendingQuestActions_) {
        // Resolve player entity from connection (connectionId stored in action)
        auto it = connectionToEntity_.find(action.connectionId);
        if (it == connectionToEntity_.end()) continue;
        EntityID player = it->second;

        if (action.actionType == 0) {
            // Accept quest
            questSystem_.acceptQuest(registry, player, action.questId, now);
        } else if (action.actionType == 1) {
            // Attempt to complete quest
            questSystem_.completeQuest(registry, player, action.questId);
        }
    }

    pendingQuestActions_.clear();
}

void ZoneServer::processPendingDialogueResponses() {
    if (pendingDialogueResponses_.empty()) return;

    Registry& registry = getRegistry();

    for (const auto& response : pendingDialogueResponses_) {
        // Resolve player entity from connection ID
        auto connIt = connectionToEntity_.find(response.connectionId);
        if (connIt == connectionToEntity_.end()) continue;
        EntityID player = connIt->second;

        // Advance dialogue
        dialogueSystem_.selectResponse(registry, player, response.selectedOption);
    }

    pendingDialogueResponses_.clear();
}

EntityID ZoneServer::spawnPlayer(ConnectionID connectionId, uint64_t playerId,
                                const std::string& username, const Position& spawnPos) {
    EntityID entity = registry_.create();

    // Add components
    registry_.emplace<Position>(entity, spawnPos);
    registry_.emplace<Velocity>(entity);
    registry_.emplace<Rotation>(entity);
    registry_.emplace<BoundingVolume>(entity);
    registry_.emplace<InputState>(entity);
    registry_.emplace<CombatState>(entity);
    registry_.emplace<NetworkState>(entity);
    registry_.emplace<AntiCheatState>(entity);

    PlayerInfo& info = registry_.emplace<PlayerInfo>(entity);
    info.playerId = playerId;
    info.connectionId = connectionId;
    std::strncpy(info.username, username.c_str(), sizeof(info.username) - 1);
    info.username[sizeof(info.username) - 1] = '\0';
    info.sessionStart = getCurrentTimeMs();

    registry_.emplace<PlayerTag>(entity);

    // [GAMEPLAY_AGENT] Initialize player progression
    registry_.emplace<PlayerProgression>(entity);

    // [GAMEPLAY_AGENT] Initialize chat state
    registry_.emplace<ChatComponent>(entity);

    // [GAMEPLAY_AGENT] Initialize trade state
    registry_.emplace<TradeComponent>(entity);

    // [GAMEPLAY_AGENT] Initialize zone event participation state
    registry_.emplace<ZoneEventComponent>(entity);

    // [GAMEPLAY_AGENT] Initialize dialogue state
    registry_.emplace<DialogueComponent>(entity);

    // Update mappings
    connectionToEntity_[connectionId] = entity;
    entityToConnection_[entity] = connectionId;

    return entity;
}

EntityID ZoneServer::spawnNPC(const Position& spawnPos, uint8_t level, uint16_t baseDamage,
                               float aggroRange, float leashRange, float attackRange,
                               uint32_t xpReward, uint32_t respawnTimeMs) {
    EntityID entity = registry_.create();

    // Core components
    registry_.emplace<Position>(entity, spawnPos);
    registry_.emplace<Velocity>(entity);
    registry_.emplace<Rotation>(entity);
    registry_.emplace<BoundingVolume>(entity);

    // Combat state — scaled by level
    registry_.emplace<CombatState>(entity);
    auto& combat = registry_.get<CombatState>(entity);
    combat.health = static_cast<int16_t>(1000 + (level - 1) * 200);
    combat.maxHealth = combat.health;
    combat.classType = 0;

    // Mana (NPCs don't use mana but the component is expected by some systems)
    Mana mana;
    mana.current = 0;
    mana.max = 0;
    mana.regenerationRate = 0;
    registry_.emplace<Mana>(entity, mana);

    // NPC tag
    registry_.emplace<NPCTag>(entity);

    // Collision layer
    registry_.emplace<CollisionLayer>(entity, CollisionLayer::makeNPC());

    // NPC AI state
    NPCAIState ai;
    ai.aggroRange = aggroRange;
    ai.leashRange = leashRange;
    ai.attackRange = attackRange;
    ai.spawnPoint = spawnPos;
    ai.attackCooldownMs = 1500;
    ai.wanderCooldownMs = 3000;
    registry_.emplace<NPCAIState>(entity, ai);

    // NPC stats
    NPCStats stats;
    stats.level = level;
    stats.baseDamage = baseDamage;
    stats.attackSpeed = 1.0f;
    stats.xpReward = xpReward;
    stats.respawnTimeMs = respawnTimeMs;
    registry_.emplace<NPCStats>(entity, stats);

    // Interaction component — enables player dialogue
    auto& interact = registry_.emplace<Interactable>(entity);
    if (interact.dialogueTreeId > 0) {
        dialogueSystem_.setNPCTree(entity, interact.dialogueTreeId);
    }

    return entity;
}

EntityID ZoneServer::spawnFromGroup(const Position& spawnPos, uint8_t level,
                                    uint16_t baseDamage, float aggroRange, float leashRange,
                                    float attackRange, uint32_t xpReward, uint32_t respawnTimeMs,
                                    uint32_t spawnGroupId, uint32_t npcTemplateId) {
    EntityID entity = registry_.create();

    // Core components
    registry_.emplace<Position>(entity, spawnPos);
    registry_.emplace<Velocity>(entity);
    registry_.emplace<Rotation>(entity);
    registry_.emplace<BoundingVolume>(entity);

    // Combat state — scaled by level
    registry_.emplace<CombatState>(entity);
    auto& combat = registry_.get<CombatState>(entity);
    combat.health = static_cast<int16_t>(1000 + (level - 1) * 200);
    combat.maxHealth = combat.health;
    combat.classType = 0;

    // Mana
    Mana mana;
    mana.current = 0;
    mana.max = 0;
    mana.regenerationRate = 0;
    registry_.emplace<Mana>(entity, mana);

    // NPC tag
    registry_.emplace<NPCTag>(entity);

    // Collision layer
    registry_.emplace<CollisionLayer>(entity, CollisionLayer::makeNPC());

    // NPC AI state
    NPCAIState ai;
    ai.aggroRange = aggroRange;
    ai.leashRange = leashRange;
    ai.attackRange = attackRange;
    ai.spawnPoint = spawnPos;
    ai.attackCooldownMs = 1500;
    ai.wanderCooldownMs = 3000;
    registry_.emplace<NPCAIState>(entity, ai);

    // NPC stats
    NPCStats stats;
    stats.level = level;
    stats.baseDamage = baseDamage;
    stats.attackSpeed = 1.0f;
    stats.xpReward = xpReward;
    stats.respawnTimeMs = respawnTimeMs;
    registry_.emplace<NPCStats>(entity, stats);

    // Interaction component — enables player dialogue
    auto& interact = registry_.emplace<Interactable>(entity);
    if (interact.dialogueTreeId > 0) {
        dialogueSystem_.setNPCTree(entity, interact.dialogueTreeId);
    }

    // SpawnableComponent — marks this NPC as managed by SpawnSystem for respawn tracking
    SpawnableComponent spawnable;
    spawnable.spawnGroupId = spawnGroupId;
    spawnable.templateId = npcTemplateId;
    spawnable.respawnTimeMs = respawnTimeMs;
    spawnable.spawnPosition = spawnPos;
    spawnable.level = level;
    spawnable.isSpawned = true;
    spawnable.shouldRespawn = true;
    registry_.emplace<SpawnableComponent>(entity, spawnable);

    // Fire spawn callback
    spawnSystem_.notifySpawned(entity, spawnGroupId);

    return entity;
}

void ZoneServer::populateNPCs() {
    float centerX = (config_.minX + config_.maxX) / 2.0f;
    float centerZ = (config_.minZ + config_.maxZ) / 2.0f;

    for (uint32_t i = 0; i < config_.npcCount; ++i) {
        // Random position within spawn radius
        float angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * 3.14159f;
        float radius = static_cast<float>(std::rand()) / RAND_MAX * config_.npcSpawnRadius;
        float x = centerX + std::cos(angle) * radius;
        float z = centerZ + std::sin(angle) * radius;

        // Clamp to zone bounds
        x = std::max(config_.minX + 1.0f, std::min(config_.maxX - 1.0f, x));
        z = std::max(config_.minZ + 1.0f, std::min(config_.maxZ - 1.0f, z));

        Position spawnPos = Position::fromVec3(glm::vec3(x, 0, z));

        // Vary level slightly
        uint8_t level = config_.npcBaseLevel;
        if (i > config_.npcCount / 2) {
            level = static_cast<uint8_t>(std::min<uint32_t>(level + 1, level + config_.npcCount / 4));
        }

        // Determine archetype based on position in the spawn list
        // Distribution: 60% melee, 20% ranged, 15% caster, 5% boss
        NPCArchetype archetype;
        float roll = static_cast<float>(std::rand()) / RAND_MAX;
        if (roll < 0.60f) {
            archetype = NPCArchetype::Melee;
        } else if (roll < 0.80f) {
            archetype = NPCArchetype::Ranged;
        } else if (roll < 0.95f) {
            archetype = NPCArchetype::Caster;
        } else {
            archetype = NPCArchetype::Boss;
        }

        // Configure based on archetype
        float aggroRange = 15.0f;
        float leashRange = 30.0f;
        float attackRange = 2.0f;
        float preferredRange = 2.0f;
        float retreatRange = 0.0f;
        uint16_t baseDamage = config_.npcBaseDamage;
        float fleeHP = 20.0f;
        uint32_t xpReward = config_.npcXpReward;

        switch (archetype) {
            case NPCArchetype::Melee:
                // Standard melee config (already set as defaults)
                break;

            case NPCArchetype::Ranged:
                attackRange = 20.0f;
                preferredRange = 15.0f;
                retreatRange = 5.0f;
                baseDamage = static_cast<uint16_t>(baseDamage * 0.8f);
                xpReward = static_cast<uint32_t>(xpReward * 1.2f);
                break;

            case NPCArchetype::Caster:
                attackRange = 25.0f;
                preferredRange = 18.0f;
                retreatRange = 8.0f;
                baseDamage = static_cast<uint16_t>(baseDamage * 0.6f);
                fleeHP = 15.0f;
                xpReward = static_cast<uint32_t>(xpReward * 1.5f);
                break;

            case NPCArchetype::Boss:
                aggroRange = 25.0f;
                leashRange = 50.0f;
                baseDamage = static_cast<uint16_t>(baseDamage * 2.0f);
                fleeHP = 5.0f;  // Bosses rarely flee
                xpReward = static_cast<uint32_t>(xpReward * 5.0f);
                level = static_cast<uint8_t>(level + 5); // Bosses are higher level
                break;
        }

        EntityID npc = spawnNPC(spawnPos, level, baseDamage,
                 aggroRange, leashRange, attackRange,
                 xpReward, 10000);

        // Apply archetype-specific configuration to spawned NPC
        if (auto* ai = registry_.try_get<NPCAIState>(npc)) {
            ai->preferredRange = preferredRange;
            ai->retreatRange = retreatRange;
            ai->fleeHealthPercent = fleeHP;
        }
        if (auto* stats = registry_.try_get<NPCStats>(npc)) {
            stats->archetype = archetype;

            // Scale health for bosses and casters
            if (auto* combat = registry_.try_get<CombatState>(npc)) {
                switch (archetype) {
                    case NPCArchetype::Boss:
                        combat->maxHealth = static_cast<int16_t>(combat->maxHealth * 5);
                        combat->health = combat->maxHealth;
                        break;
                    case NPCArchetype::Caster:
                        combat->maxHealth = static_cast<int16_t>(combat->maxHealth * 0.7f);
                        combat->health = combat->maxHealth;
                        break;
                    default:
                        break;
                }
            }
        }

        // Add loot tables to NPCs
        if (!registry_.all_of<LootTable>(npc)) {
            LootTable loot;
            // Basic loot: always drop gold
            loot.goldDropMin = 1.0f;
            loot.goldDropMax = 5.0f * level;

            // Add item drops based on archetype
            if (archetype == NPCArchetype::Boss) {
                // Bosses drop better loot
                loot.entries[0] = LootEntry{3, 1, 1, 0.3f};   // Iron Longsword (30%)
                loot.entries[1] = LootEntry{8, 1, 1, 0.2f};   // Lucky Amulet (20%)
                loot.entries[2] = LootEntry{22, 1, 3, 0.5f};  // Ancient Relic Shard (50%)
                loot.entries[3] = LootEntry{30, 1, 1, 0.05f}; // Vorpal Blade (5%)
                loot.count = 4;
                loot.goldDropMin = 50.0f;
                loot.goldDropMax = 200.0f;
            } else if (archetype == NPCArchetype::Caster) {
                loot.entries[0] = LootEntry{11, 1, 3, 0.4f};  // Mana Potion (40%)
                loot.entries[1] = LootEntry{4, 1, 1, 0.1f};   // Flame Staff (10%)
                loot.count = 2;
            } else {
                // Melee/Ranged: common drops
                loot.entries[0] = LootEntry{10, 1, 2, 0.3f};  // Health Potion (30%)
                loot.entries[1] = LootEntry{20, 1, 1, 0.2f};  // Wolf Pelt (20%)
                loot.entries[2] = LootEntry{21, 1, 2, 0.15f}; // Iron Ore (15%)
                loot.count = 3;
            }

            registry_.emplace<LootTable>(npc, loot);
        }
    }

    std::cout << "[ZONE " << config_.zoneId << "] Populated " << config_.npcCount
              << " NPCs (level " << static_cast<int>(config_.npcBaseLevel) << ")" << std::endl;
}

void ZoneServer::despawnEntity(EntityID entity) {
    // Track destroyed entity for removal notifications
    destroyedEntities_.push_back(entity);

    // Remove from connection mapping
    auto it = entityToConnection_.find(entity);
    if (it != entityToConnection_.end()) {
        connectionToEntity_.erase(it->second);
        entityToConnection_.erase(it);
    }

    // [ZONE_AGENT] Clean up replication tracking for destroyed entity
    replicationOptimizer_.removeEntityTracking(entity);

    // [COMBAT_AGENT] Remove from lag compensator history
    lagCompensator_.removeEntity(entity);

    // Destroy entity
    registry_.destroy(entity);
}


void ZoneServer::onEntityMigrationComplete(EntityID entity, bool success) {
    if (success) {
        std::cout << "[ZONE " << config_.zoneId
                  << "] Entity migration completed successfully for entity "
                  << static_cast<uint32_t>(entity) << std::endl;

        // Update connection mapping
        auto it = entityToConnection_.find(entity);
        if (it != entityToConnection_.end()) {
            ConnectionID connId = it->second;
            connectionToEntity_.erase(connId);
            entityToConnection_.erase(it);

            // Notify client of zone change if it's a player
            // In production, this would redirect the connection to the new zone
        }

        // Clean up replication tracking
        replicationOptimizer_.removeEntityTracking(entity);

        // Remove from lag compensator
        lagCompensator_.removeEntity(entity);
    } else {
        std::cerr << "[ZONE " << config_.zoneId
                  << "] Entity migration failed for entity "
                  << static_cast<uint32_t>(entity) << std::endl;
        // Player stays in current zone, migration was rolled back
    }
}

// [PHASE 4E] Initialize zone handoff controller
void ZoneServer::initializeHandoffController() {
    handoffController_ = std::make_unique<ZoneHandoffController>(
        config_.zoneId,
        migrationManager_.get(),
        &auraManager_,
        network_.get()
    );

    // Set up zone definition for distance calculations
    handoffController_->setMyZoneDefinition(buildZoneDefinition());

    // Set up zone lookup callbacks
    handoffController_->setZoneLookupCallbacks(
        [this](uint32_t zoneId) -> ZoneDefinition* { return lookupZone(zoneId); },
        [this](float x, float z) -> uint32_t { return findZoneByPosition(x, z); }
    );

    // Set up handoff callbacks
    handoffController_->setOnHandoffStarted(
        [this](uint64_t playerId, uint32_t sourceZone, uint32_t targetZone, bool success) {
            onHandoffStarted(playerId, sourceZone, targetZone, success);
        }
    );
    handoffController_->setOnHandoffCompleted(
        [this](uint64_t playerId, uint32_t sourceZone, uint32_t targetZone, bool success) {
            onHandoffCompleted(playerId, sourceZone, targetZone, success);
        }
    );

    // Initialize with default config
    if (!handoffController_->initialize()) {
        std::cerr << "[ZONE " << config_.zoneId << "] Failed to initialize handoff controller!" << std::endl;
    } else {
        std::cout << "[ZONE " << config_.zoneId << "] Zone handoff controller initialized" << std::endl;
    }
}

// [PHASE 4E] Update zone handoffs - check all players for potential handoffs
void ZoneServer::updateZoneHandoffs() {
    if (!handoffController_) return;

    uint32_t currentTimeMs = getCurrentTimeMs();

    // Check all players for potential handoffs
    auto view = registry_.view<PlayerInfo, Position, Velocity>();

    view.each([this, currentTimeMs](EntityID entity, const PlayerInfo& info,
                                   const Position& pos, const Velocity& vel) {
        auto connIt = entityToConnection_.find(entity);
        if (connIt == entityToConnection_.end()) return;

        handoffController_->checkPlayerPosition(info.playerId, entity, connIt->second,
                                               pos, vel, currentTimeMs);
    });

    // Update handoff controller state machine
    handoffController_->update(registry_, currentTimeMs);
}

// [PHASE 4E] Handoff started callback
void ZoneServer::onHandoffStarted(uint64_t playerId, uint32_t sourceZone,
                                   uint32_t targetZone, bool success) {
    std::cout << "[ZONE " << config_.zoneId << "] Handoff started for player "
              << playerId << " from zone " << sourceZone << " to zone " << targetZone << std::endl;

    // Could log to ScyllaDB for analytics
    // Could notify other systems (e.g., party system, guild system)
}

// [PHASE 4E] Handoff completed callback
void ZoneServer::onHandoffCompleted(uint64_t playerId, uint32_t sourceZone,
                                     uint32_t targetZone, bool success) {
    if (success) {
        std::cout << "[ZONE " << config_.zoneId << "] Handoff completed for player "
                  << playerId << " to zone " << targetZone << std::endl;
    } else {
        std::cerr << "[ZONE " << config_.zoneId << "] Handoff failed for player "
                  << playerId << " to zone " << targetZone << std::endl;
    }

    // Update metrics, logging, etc.
}

// [ZONES_AGENT] Build ZoneDefinition from zone config and loaded demo JSON.
// Enhanced: parses objectives, wave count, and time limits from demo config JSON.
ZoneDefinition ZoneServer::buildZoneDefinition() const {
    ZoneDefinition def;
    def.zoneId  = config_.zoneId;
    def.zoneName = "Zone " + std::to_string(config_.zoneId);
    def.shape   = ZoneShape::RECTANGLE;
    def.minX    = config_.minX;
    def.maxX    = config_.maxX;
    def.minY    = 0.0f;
    def.maxY    = 100.0f;
    def.minZ    = config_.minZ;
    def.maxZ    = config_.maxZ;
    def.centerX = (config_.minX + config_.maxX) * 0.5f;
    def.centerZ = (config_.minZ + config_.maxZ) * 0.5f;
    def.host    = "";
    def.port    = config_.port;

    // Parse zone objectives from demo config JSON if available
    if (!demoConfigJson_.empty()) {
        try {
            nlohmann::json j = nlohmann::json::parse(demoConfigJson_);

            if (j.contains("objectives") && j["objectives"].is_array()) {
                for (const auto& obj : j["objectives"]) {
                    ZoneDefinition::Objective zoneObj;
                    zoneObj.id = obj.value("id", "");
                    zoneObj.required = obj.value("required", true);
                    zoneObj.requiredCount = obj.value("count", 1);

                    std::string typeStr = obj.value("type", "custom");
                    if (typeStr == "kill" || typeStr == "eliminate") {
                        zoneObj.type = ZoneDefinition::Objective::Type::Kill;
                    } else if (typeStr == "interact") {
                        zoneObj.type = ZoneDefinition::Objective::Type::Interact;
                    } else if (typeStr == "damage") {
                        zoneObj.type = ZoneDefinition::Objective::Type::Damage;
                    } else {
                        zoneObj.type = ZoneDefinition::Objective::Type::Custom;
                    }

                    if (!zoneObj.id.empty()) {
                        def.objectives.push_back(zoneObj);
                    }
                }
            }

            if (j.contains("wave_defense") && j["wave_defense"].contains("waves")) {
                def.waveCount = static_cast<uint8_t>(j["wave_defense"]["waves"].size());
            } else if (j.contains("wave_count")) {
                def.waveCount = j["wave_count"].get<uint8_t>();
            }

            if (j.contains("time_limit")) {
                def.timeLimit = j["time_limit"].get<float>();
            } else if (j.contains("timeLimit")) {
                def.timeLimit = j["timeLimit"].get<float>();
            }
        } catch (const std::exception& e) {
            std::cerr << "[ZONE " << config_.zoneId << "] Failed to parse zone definition from demo config: " << e.what() << std::endl;
        }
    }

    return def;
}

// [PHASE 4E] Zone lookup callback - returns zone definition by ID
ZoneDefinition* ZoneServer::lookupZone(uint32_t zoneId) {
    // In production, this would query a zone registry or service discovery
    // For now, construct a basic definition based on known zone layout

    // Simple grid layout: zones are adjacent rectangles
    static ZoneDefinition tempDef;
    tempDef.zoneId = zoneId;

    // Calculate approximate position based on zone ID
    // Assuming 2x2 grid for simple case
    uint32_t zoneX = (zoneId - 1) % 2;
    uint32_t zoneZ = (zoneId - 1) / 2;

    float zoneWidth = (Constants::WORLD_MAX_X - Constants::WORLD_MIN_X) / 2.0f;
    float zoneDepth = (Constants::WORLD_MAX_Z - Constants::WORLD_MIN_Z) / 2.0f;

    tempDef.minX = Constants::WORLD_MIN_X + zoneX * zoneWidth;
    tempDef.maxX = tempDef.minX + zoneWidth;
    tempDef.minZ = Constants::WORLD_MIN_Z + zoneZ * zoneDepth;
    tempDef.maxZ = tempDef.minZ + zoneDepth;
    tempDef.centerX = (tempDef.minX + tempDef.maxX) / 2.0f;
    tempDef.centerZ = (tempDef.minZ + tempDef.maxZ) / 2.0f;
    tempDef.port = Constants::DEFAULT_SERVER_PORT + static_cast<uint16_t>(zoneId) - 1;
    tempDef.host = "127.0.0.1";  // Localhost for testing

    return &tempDef;
}

// [PHASE 4E] Find zone by position callback - returns zone ID containing position
uint32_t ZoneServer::findZoneByPosition(float x, float z) {
    // Simple grid-based zone lookup
    // In production, this would use a spatial index or query a world partition

    float zoneWidth = (Constants::WORLD_MAX_X - Constants::WORLD_MIN_X) / 2.0f;
    float zoneDepth = (Constants::WORLD_MAX_Z - Constants::WORLD_MIN_Z) / 2.0f;

    uint32_t zoneX = static_cast<uint32_t>((x - Constants::WORLD_MIN_X) / zoneWidth);
    uint32_t zoneZ = static_cast<uint32_t>((z - Constants::WORLD_MIN_Z) / zoneDepth);

    // Clamp to valid range
    zoneX = std::min(zoneX, 1u);
    zoneZ = std::min(zoneZ, 1u);

    return zoneZ * 2 + zoneX + 1;  // 1-indexed zone IDs
}

// [DEMO_AGENT] Load demo zone configuration from JSON file
bool ZoneServer::loadDemoConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "[ZONE " << config_.zoneId << "] Failed to open demo config: " << configPath << std::endl;
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;
        demoConfigJson_ = j.dump();

        // Validate required fields
        if (!j.contains("zone_id") || !j.contains("npc_presets")) {
            std::cerr << "[ZONE " << config_.zoneId << "] Invalid demo config: missing zone_id or npc_presets" << std::endl;
            return false;
        }

        // Update zone ID from config if present
        if (j.contains("zone_id")) {
            config_.zoneId = j["zone_id"].get<uint32_t>();
        }

        std::cout << "[ZONE " << config_.zoneId << "] Loaded demo config: " << j.value("name", "Unnamed") << std::endl;
        std::cout << "[ZONE " << config_.zoneId << "] Description: " << j.value("description", "") << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ZONE " << config_.zoneId << "] Failed to parse demo config: " << e.what() << std::endl;
        return false;
    }
}

// [DEMO_AGENT] Populate NPCs from loaded demo configuration
void ZoneServer::populateNPCsFromDemoConfig() {
    if (demoConfigJson_.empty()) {
        std::cerr << "[ZONE " << config_.zoneId << "] No demo config loaded, falling back to default" << std::endl;
        populateNPCs();
        return;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(demoConfigJson_);
        const auto& npcPresets = j["npc_presets"];

        uint32_t totalSpawned = 0;
        float centerX = (config_.minX + config_.maxX) / 2.0f;
        float centerZ = (config_.minZ + config_.maxZ) / 2.0f;

        for (const auto& preset : npcPresets) {
            std::string archetypeName = preset.value("archetype", "wolf");
            uint32_t count = preset.value("count", 1);
            float radius = preset.value("radius", 25.0f);
            uint8_t level = preset.value("level", 1);

            // Map archetype string to enum
            NPCArchetype archetype = NPCArchetype::Melee;
            if (archetypeName == "bandit" || archetypeName == "ranged") {
                archetype = NPCArchetype::Ranged;
            } else if (archetypeName == "caster" || archetypeName == "mage") {
                archetype = NPCArchetype::Caster;
            } else if (archetypeName == "boss" || archetypeName == "boss_ogre") {
                archetype = NPCArchetype::Boss;
            }

            // Spawn NPCs for this preset
            for (uint32_t i = 0; i < count; ++i) {
                float angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * 3.14159f;
                float dist = static_cast<float>(std::rand()) / RAND_MAX * radius;
                float x = centerX + std::cos(angle) * dist;
                float z = centerZ + std::sin(angle) * dist;

                x = std::max(config_.minX + 1.0f, std::min(config_.maxX - 1.0f, x));
                z = std::max(config_.minZ + 1.0f, std::min(config_.maxZ - 1.0f, z));

                Position spawnPos = Position::fromVec3(glm::vec3(x, 0, z));

                // Configure based on archetype
                float aggroRange = 15.0f;
                float leashRange = 30.0f;
                float attackRange = 2.0f;
                float preferredRange = 2.0f;
                float retreatRange = 0.0f;
                float fleeHP = 20.0f;
                uint16_t baseDamage = config_.npcBaseDamage;
                uint32_t xpReward = config_.npcXpReward;

                switch (archetype) {
                    case NPCArchetype::Melee:
                        break;
                    case NPCArchetype::Ranged:
                        attackRange = 20.0f;
                        preferredRange = 15.0f;
                        retreatRange = 5.0f;
                        baseDamage = static_cast<uint16_t>(baseDamage * 0.8f);
                        xpReward = static_cast<uint32_t>(xpReward * 1.2f);
                        break;
                    case NPCArchetype::Caster:
                        attackRange = 25.0f;
                        preferredRange = 18.0f;
                        retreatRange = 8.0f;
                        baseDamage = static_cast<uint16_t>(baseDamage * 0.6f);
                        fleeHP = 15.0f;
                        xpReward = static_cast<uint32_t>(xpReward * 1.5f);
                        break;
                    case NPCArchetype::Boss:
                        aggroRange = 25.0f;
                        leashRange = 50.0f;
                        baseDamage = static_cast<uint16_t>(baseDamage * 2.0f);
                        fleeHP = 5.0f;
                        xpReward = static_cast<uint32_t>(xpReward * 5.0f);
                        level = static_cast<uint8_t>(level + 5);
                        break;
                }

                EntityID npc = spawnNPC(spawnPos, level, baseDamage,
                                       aggroRange, leashRange, attackRange,
                                       xpReward, 10000);

                // Apply archetype-specific configuration
                if (auto* ai = registry_.try_get<NPCAIState>(npc)) {
                    ai->preferredRange = preferredRange;
                    ai->retreatRange = retreatRange;
                    ai->fleeHealthPercent = fleeHP;
                }
                if (auto* stats = registry_.try_get<NPCStats>(npc)) {
                    stats->archetype = archetype;

                    if (auto* combat = registry_.try_get<CombatState>(npc)) {
                        switch (archetype) {
                            case NPCArchetype::Boss:
                                combat->maxHealth = static_cast<int16_t>(combat->maxHealth * 5);
                                combat->health = combat->maxHealth;
                                break;
                            case NPCArchetype::Caster:
                                combat->maxHealth = static_cast<int16_t>(combat->maxHealth * 0.7f);
                                combat->health = combat->maxHealth;
                                break;
                            default:
                                break;
                        }
                    }
                }

                // Add loot tables
                if (!registry_.all_of<LootTable>(npc)) {
                    LootTable loot;
                    loot.goldDropMin = 1.0f;
                    loot.goldDropMax = 5.0f * level;

                    if (archetype == NPCArchetype::Boss) {
                        loot.entries[0] = LootEntry{3, 1, 1, 0.3f};
                        loot.entries[1] = LootEntry{8, 1, 1, 0.2f};
                        loot.entries[2] = LootEntry{22, 1, 3, 0.5f};
                        loot.entries[3] = LootEntry{30, 1, 1, 0.05f};
                        loot.count = 4;
                        loot.goldDropMin = 50.0f;
                        loot.goldDropMax = 200.0f;
                    } else if (archetype == NPCArchetype::Caster) {
                        loot.entries[0] = LootEntry{11, 1, 3, 0.4f};
                        loot.entries[1] = LootEntry{4, 1, 1, 0.1f};
                        loot.count = 2;
                    } else {
                        loot.entries[0] = LootEntry{10, 1, 2, 0.3f};
                        loot.entries[1] = LootEntry{20, 1, 1, 0.2f};
                        loot.entries[2] = LootEntry{21, 1, 2, 0.15f};
                        loot.count = 3;
                    }

                    registry_.emplace<LootTable>(npc, loot);
                }

                totalSpawned++;
            }
        }

        std::cout << "[ZONE " << config_.zoneId << "] Demo config: spawned " << totalSpawned << " NPCs" << std::endl;

        // Load demo quest if present
        if (j.contains("demo_quest")) {
            const auto& quest = j["demo_quest"];
            std::cout << "[ZONE " << config_.zoneId << "] Demo quest available: "
                      << quest.value("title", "Untitled") << std::endl;
        }

        // Load zone event if present
        if (j.contains("zone_event")) {
            const auto& event = j["zone_event"];
            std::cout << "[ZONE " << config_.zoneId << "] Zone event configured: "
                      << event.value("id", "none") << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "[ZONE " << config_.zoneId << "] Failed to populate from demo config: " << e.what() << std::endl;
        populateNPCs();
    }
}

} // namespace DarkAges
