#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>
#include <vector>

// Zone Event System — manages timed world events, world bosses, and zone-wide activities.
// Events have phases with objectives. Players participate by dealing damage, killing NPCs,
// or surviving. Rewards are distributed based on participation contribution.
//
// Event lifecycle: Register → startEvent() → Announce → Active (phases) → Complete → Cooldown

namespace DarkAges {

class ChatSystem;
class ExperienceSystem;
class ItemSystem;

class ZoneEventSystem {
public:
    ZoneEventSystem() = default;

    // --- Configuration ---

    void setConfig(const ZoneEventConfig& config) { config_ = config; }
    [[nodiscard]] const ZoneEventConfig& getConfig() const { return config_; }

    // --- Dependencies ---

    void setChatSystem(ChatSystem* cs) { chatSystem_ = cs; }
    void setExperienceSystem(ExperienceSystem* es) { experienceSystem_ = es; }
    void setItemSystem(ItemSystem* is) { itemSystem_ = is; }

    // --- Event Registry ---

    // Register an event definition (takes ownership of definition data)
    void registerEvent(const ZoneEventDefinition& definition);

    // Get event definition by ID, nullptr if not found
    const ZoneEventDefinition* getEvent(uint32_t eventId) const;

    // Get all registered event IDs
    std::vector<uint32_t> getRegisteredEventIds() const;

    // --- Event Lifecycle ---

    // Start an event (moves to Announcing state).
    // Returns true if event was started successfully.
    bool startEvent(Registry& registry, uint32_t eventId, uint32_t currentTimeMs);

    // Force-end the current event (cancel).
    // Returns true if there was an active event to end.
    bool endEvent(Registry& registry, uint32_t currentTimeMs);

    // --- Tick Update ---

    // Called each server tick to advance event state.
    // Handles: announcement → active transitions, phase advancement,
    //          objective checking, NPC spawning, timeout handling.
    void update(Registry& registry, uint32_t currentTimeMs);

    // --- Participation ---

    // Called when a player deals damage during an active event.
    // Tracks damage for participation and objective progress.
    void onDamageDealt(Registry& registry, EntityID attacker, EntityID target,
                       uint32_t damage, uint32_t currentTimeMs);

    // Called when an NPC is killed during an active event.
    // Tracks kills for participation and objective progress.
    void onNPCKilled(Registry& registry, EntityID killer, EntityID victim,
                     uint32_t npcArchetypeId, uint32_t currentTimeMs);

    // Called when a player dies during an active event.
    void onPlayerDied(Registry& registry, EntityID player, uint32_t currentTimeMs);

    // Join a player to the current active event.
    // Returns true if joined successfully.
    bool joinEvent(Registry& registry, EntityID player, uint32_t currentTimeMs);

    // Remove a player from the current event.
    // Returns true if removed.
    bool leaveEvent(Registry& registry, EntityID player);

    // --- Queries ---

    // Check if there is an active event
    [[nodiscard]] bool hasActiveEvent() const {
        return activeEvent_.state != ZoneEventState::Inactive;
    }

    // Get the active event state (read-only)
    [[nodiscard]] const ActiveZoneEvent& getActiveEvent() const { return activeEvent_; }

    // Check if a player is participating in the current event
    [[nodiscard]] bool isParticipating(uint64_t playerId) const {
        return activeEvent_.findParticipant(playerId) != nullptr;
    }

    // Get participation data for a player
    [[nodiscard]] const EventParticipation* getParticipation(uint64_t playerId) const {
        return activeEvent_.findParticipant(playerId);
    }

    // Get current event phase definition (nullptr if no active event)
    [[nodiscard]] const ZoneEventPhaseDefinition* getCurrentPhaseDef() const;

    // --- NPC Spawning Callback ---

    // Callback to spawn NPCs for event phases: (spawnX, spawnZ, npcArchetypeId, level, count)
    using SpawnCallback = std::function<void(float, float, uint32_t, uint8_t, uint32_t)>;
    void setSpawnCallback(SpawnCallback cb) { spawnCallback_ = std::move(cb); }

    // Callback to spawn a boss NPC: (spawnX, spawnZ, level, returns entity ID)
    using BossSpawnCallback = std::function<EntityID(float, float, uint8_t)>;
    void setBossSpawnCallback(BossSpawnCallback cb) { bossSpawnCallback_ = std::move(cb); }

    // --- Callbacks ---

    // Called when event state changes: (eventId, newState)
    using EventStateCallback = std::function<void(uint32_t, ZoneEventState)>;
    void setEventStateCallback(EventStateCallback cb) { eventStateCallback_ = std::move(cb); }

    // Called when a phase changes: (eventId, newPhaseIndex)
    using PhaseChangeCallback = std::function<void(uint32_t, uint32_t)>;
    void setPhaseChangeCallback(PhaseChangeCallback cb) { phaseChangeCallback_ = std::move(cb); }

    // Called when event completes with reward distribution: (eventId)
    using EventCompleteCallback = std::function<void(uint32_t)>;
    void setEventCompleteCallback(EventCompleteCallback cb) { eventCompleteCallback_ = std::move(cb); }

private:
    // Transition to a new event state
    void transitionState(Registry& registry, ZoneEventState newState, uint32_t currentTimeMs);

    // Advance to the next phase (or complete if no more phases)
    void advancePhase(Registry& registry, uint32_t currentTimeMs);

    // Spawn NPCs for the current phase
    void spawnPhaseNPCs(Registry& registry, uint32_t currentTimeMs);

    // Check if current phase objectives are met
    bool arePhaseObjectivesComplete() const;

    // Distribute rewards to all participants
    void distributeRewards(Registry& registry, uint32_t currentTimeMs);

    // Find the top contributor (highest total damage)
    const EventParticipation* findTopContributor() const;

    // Send system message about event
    void sendEventMessage(Registry& registry, const char* message);

    // Event registry
    static constexpr uint32_t MAX_EVENTS = 64;
    std::vector<ZoneEventDefinition> events_;

    // Active event state
    ActiveZoneEvent activeEvent_;

    // Configuration
    ZoneEventConfig config_;

    // Dependencies
    ChatSystem* chatSystem_{nullptr};
    ExperienceSystem* experienceSystem_{nullptr};
    ItemSystem* itemSystem_{nullptr};

    // Callbacks
    SpawnCallback spawnCallback_;
    BossSpawnCallback bossSpawnCallback_;
    EventStateCallback eventStateCallback_;
    PhaseChangeCallback phaseChangeCallback_;
    EventCompleteCallback eventCompleteCallback_;

    // Cooldown tracking: event ID -> last completion time
    uint32_t lastCompletionTime_[MAX_EVENTS]{};
};

} // namespace DarkAges
