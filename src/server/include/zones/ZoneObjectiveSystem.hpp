#pragma once

#include "zones/ZoneDefinition.hpp"
#include "zones/ZoneObjectiveComponent.hpp"

#include <entt/entity/fwd.hpp>
#include <entt/core/signal.hpp>

#include <vector>
#include <unordered_map>
#include <string>

namespace DarkAges
{
  /**
   * ZoneObjectiveSystem - Server-side objective tracking for zones
   * 
   * Monitors player progress on zone objectives and sends
   * updates to clients via the snapshot system.
   * 
   * Usage:
   *   ZoneObjectiveSystem system;
   *   system.Initialize(registry);
   *   system.OnPlayerEnterZone(entity, zoneId);
   *   system.OnObjectiveProgress(entity, "kill_5_dummies", 1);
   */
  class ZoneObjectiveSystem
  {
  public:
    using EventEmitter = entt::emitter<ZoneObjectiveSystem>;
    
    /// Signal emitted when objective progress changes
    using ObjectiveProgressSignal = entt::delegate<void(entt::entity, const std::string&, uint16_t, uint16_t)>;
    
    /// Signal emitted when objective completes
    using ObjectiveCompleteSignal = entt::delegate<void(entt::entity, const std::string&)>;
    
    /// Signal emitted when wave starts/completes
    using WaveSignal = entt::delegate<void(entt::entity, uint8_t, bool)>;
    
    /// Signal emitted when zone completes
    using ZoneCompleteSignal = entt::delegate<void(entt::entity, uint16_t)>;
    
    ZoneObjectiveSystem() = default;
    ~ZoneObjectiveSystem() = default;
    
    /**
     * Initialize the system with ECS registry
     */
    void Initialize(entt::registry& registry);
    
    /**
     * Handle player entering a zone with objectives
     * @param player Player entity
     * @param zoneId Zone ID from config
     * @param zoneDef Zone definition with objectives/events
     */
    void OnPlayerEnterZone(entt::entity player, uint16_t zoneId, const ZoneDefinition& zoneDef);
    
    /**
     * Handle player leaving a zone
     * @param player Player entity
     */
    void OnPlayerLeaveZone(entt::entity player);
    
    /**
     * Report objective progress
     * @param player Player entity
     * @param objectiveId Objective ID from config
     * @param progress Current progress count (+1 for kills, etc.)
     */
    void OnObjectiveProgress(entt::entity player, const std::string& objectiveId, uint16_t progress);
    
    /**
     * Handle wave start in wave-defense zones
     * @param player Player entity  
     * @param waveNumber Wave number (1-indexed)
     */
    void OnWaveStart(entt::entity player, uint8_t waveNumber);
    
    /**
     * Handle wave complete (all enemies defeated)
     * @param player Player entity
     * @param waveNumber Wave that completed
     */
    void OnWaveComplete(entt::entity player, uint8_t waveNumber);
    
    /**
     * Check if player can advance to next zone
     * @param player Player entity
     * @return true if all required objectives complete
     */
    bool CanAdvanceZone(entt::entity player) const;
    
    /**
     * Get current zone state for snapshot serialization
     * @param player Player entity
     * @return Reference to component
     */
    ZoneObjectiveComponent& GetZoneState(entt::entity player);
    
    /**
     * Get objective progress map for UI
     * @param player Player entity
     * @return Map of objective ID to progress
     */
    const std::unordered_map<std::string, ObjectiveProgress>& GetObjectives(entt::entity player) const;
    
    /**
     * Tick objective timers (called each server tick)
     * @param deltaTime Time since last tick
     */
    void Tick(float deltaTime);
    
    /**
     * Get signal for objective progress events
     */
    entt::Sink<ObjectiveProgressSignal> OnObjectiveProgressed()
    {
      return _objectiveProgressSink;
    }
    
    /**
     * Get signal for objective complete events
     */
    entt::Sink<ObjectiveCompleteSignal> OnObjectiveCompleted()
    {
      return _objectiveCompleteSink;
    }
    
    /**
     * Get signal for wave events
     */
    entt::Sink<WaveSignal> OnWaveChanged()
    {
      return _waveSink;
    }
    
    /**
     * Get signal for zone complete events
     */
    entt::Sink<ZoneCompleteSignal> OnZoneCompleted()
    {
      return _zoneCompleteSink;
    }
    
  private:
    entt::registry* _registry = nullptr;
    
    /// Player entity -> zone definition
    std::unordered_map<entt::entity, ZoneDefinition> _playerZoneDefs;
    
    /// Player entity -> objective progress map
    std::unordered_map<entt::entity, std::unordered_map<std::string, ObjectiveProgress>> _playerObjectives;
    
    /// Signals
    entt::sigh_forward<ObjectiveProgressSignal> _objectiveProgressSignal;
    entt::sigh_forward<ObjectiveCompleteSignal> _objectiveCompleteSignal;
    entt::sigh_forward<WaveSignal> _waveSignal;
    entt::sigh_forward<ZoneCompleteSignal> _zoneCompleteSignal;
    
    /// Sinks (connected to handlers)
    ObjectiveProgressSignal _objectiveProgressSink{_objectiveProgressSignal};
    ObjectiveCompleteSignal _objectiveCompleteSink{_objectiveCompleteSignal};
    WaveSignal _waveSink{_waveSignal};
    ZoneCompleteSignal _zoneCompleteSink{_zoneCompleteSignal};
    
    /**
     * Helper: Check if all required objectives complete
     */
    bool CheckObjectivesComplete(entt::entity player, const ZoneDefinition& zoneDef);
    
    /**
     * Helper: Emit events to player
     */
    void EmitEvent(entt::entity player, const ZoneObjectiveEvent& event);
  };
} // namespace DarkAges