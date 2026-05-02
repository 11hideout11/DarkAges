#pragma once

#include "zones/ZoneDefinition.hpp"
#include "zones/ZoneObjectiveComponent.hpp"

#include <entt/entt.hpp>

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
   *   system.OnPlayerEnterZone(entity, zoneId, zoneDef);
   *   system.OnObjectiveProgress(entity, "kill_5_dummies", 3);
   */
  class ZoneObjectiveSystem
  {
  public:
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
     * Report objective progress (absolute count, not a delta)
     * @param player Player entity
     * @param objectiveId Objective ID from config
     * @param count Absolute current count (e.g. 3 for "3 kills so far")
     */
    void OnObjectiveProgress(entt::entity player, const std::string& objectiveId, uint16_t count);
    
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
    const std::unordered_map<std::string, ZoneObjectiveProgress>& GetObjectives(entt::entity player) const;
    
    /**
     * Tick objective timers (called each server tick)
     * @param deltaTime Time since last tick
     */
    void Tick(float deltaTime);
    
    /**
     * Get sink for objective progress events.
     * Signal signature: void(entt::entity player, const std::string& objectiveId,
     *                        uint16_t currentCount, uint16_t requiredCount)
     */
    auto OnObjectiveProgressed()
    {
      return entt::sink{_objectiveProgressSignal};
    }
    
    /**
     * Get sink for objective complete events.
     * Signal signature: void(entt::entity player, const std::string& objectiveId)
     */
    auto OnObjectiveCompleted()
    {
      return entt::sink{_objectiveCompleteSignal};
    }
    
    /**
     * Get sink for wave events.
     * Signal signature: void(entt::entity player, uint8_t waveNumber, bool starting)
     */
    auto OnWaveChanged()
    {
      return entt::sink{_waveSignal};
    }
    
    /**
     * Get sink for zone complete events.
     * Signal signature: void(entt::entity player, uint16_t zoneId)
     */
    auto OnZoneCompleted()
    {
      return entt::sink{_zoneCompleteSignal};
    }
    
  private:
    entt::registry* _registry = nullptr;
    
    /// Player entity -> zone definition
    std::unordered_map<entt::entity, ZoneDefinition> _playerZoneDefs;
    
    /// Player entity -> objective progress map
    std::unordered_map<entt::entity, std::unordered_map<std::string, ZoneObjectiveProgress>> _playerObjectives;
    
    /// Signals (publish via these; expose sinks via On*() accessors)
    entt::sigh<void(entt::entity, const std::string&, uint16_t, uint16_t)> _objectiveProgressSignal;
    entt::sigh<void(entt::entity, const std::string&)>                     _objectiveCompleteSignal;
    entt::sigh<void(entt::entity, uint8_t, bool)>                          _waveSignal;
    entt::sigh<void(entt::entity, uint16_t)>                               _zoneCompleteSignal;
    
    /**
     * Helper: Check if all required objectives complete
     */
    bool CheckObjectivesComplete(entt::entity player, const ZoneDefinition& zoneDef) const;
    
    /**
     * Helper: Emit events to player
     */
    void EmitEvent(entt::entity player, const ZoneObjectiveEvent& event);
  };
} // namespace DarkAges