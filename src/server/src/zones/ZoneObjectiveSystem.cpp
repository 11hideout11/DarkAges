#include "zones/ZoneObjectiveSystem.hpp"
#include "components/PlayerComponent.hpp"

#include <entt/entity/registry.hpp>

#include <cstring>

namespace DarkAges
{
  void ZoneObjectiveSystem::Initialize(entt::registry& registry)
  {
    _registry = &registry;
  }
  
  void ZoneObjectiveSystem::OnPlayerEnterZone(entt::entity player, uint16_t zoneId, const ZoneDefinition& zoneDef)
  {
    if (!_registry) return;
    
    // Ensure player has component
    auto& component = _registry->get_or_emplace<ZoneObjectiveComponent>(player);
    component.ZoneId = zoneId;
    component.CurrentWave = 0;
    component.TotalWaves = zoneDef.waveCount;
    component.TimeRemaining = zoneDef.timeLimit;
    component.ZoneComplete = false;
    component.RewardsClaimed = false;
    
    // Store zone definition
    _playerZoneDefs[player] = zoneDef;
    
    // Initialize objectives from zone config
    auto& objectives = _playerObjectives[player];
    objectives.clear();
    
    for (const auto& obj : zoneDef.objectives)
    {
      ObjectiveProgress progress;
      progress.ObjectiveId = obj.id;
      progress.CurrentCount = 0;
      progress.RequiredCount = obj.requiredCount;
      progress.Complete = false;
      progress.CompletedAt = 0;
      objectives[obj.id] = progress;
    }
    
    // Notify client of active objectives
    ZoneObjectiveEvent event;
    event.Type = ZoneObjectiveEvent::EventType::ObjectiveStarted;
    // Note: Would serialize objectives list to client
    EmitEvent(player, event);
  }
  
  void ZoneObjectiveSystem::OnPlayerLeaveZone(entt::entity player)
  {
    if (!_registry) return;
    
    // Remove component
    if (_registry->has<ZoneObjectiveComponent>(player))
    {
      _registry->remove<ZoneObjectiveComponent>(player);
    }
    
    // Clear tracking data
    _playerZoneDefs.erase(player);
    _playerObjectives.erase(player);
  }
  
  void ZoneObjectiveSystem::OnObjectiveProgress(entt::entity player, const std::string& objectiveId, uint16_t progress)
  {
    auto it = _playerObjectives.find(player);
    if (it == _playerObjectives.end()) return;
    
    auto& objectives = it->second;
    auto objIt = objectives.find(objectiveId);
    if (objIt == objectives.end()) return;
    
    auto& objProgress = objIt->second;
    
    // Update progress
    objProgress.CurrentCount = progress;
    
    // Check complete
    if (!objProgress.Complete && progress >= objProgress.RequiredCount)
    {
      objProgress.Complete = true;
      objProgress.CompletedAt = static_cast<uint32_t>(time(nullptr));
      
      // Emit complete event
      _objectiveCompleteSink.publish(player, objectiveId);
      
      // Notify client
      ZoneObjectiveEvent event;
      event.Type = ZoneObjectiveEvent::EventType::ObjectiveComplete;
      event.ObjectiveId = objectiveId;
      EmitEvent(player, event);
    }
    else
    {
      // Emit progress event
      _objectiveProgressSink.publish(player, objectiveId, progress, objProgress.RequiredCount);
      
      // Notify client
      ZoneObjectiveEvent event;
      event.Type = ZoneObjectiveEvent::EventType::ObjectiveProgress;
      event.ObjectiveId = objectiveId;
      event.CurrentProgress = progress;
      event.RequiredProgress = objProgress.RequiredCount;
      EmitEvent(player, event);
    }
  }
  
  void ZoneObjectiveSystem::OnWaveStart(entt::entity player, uint8_t waveNumber)
  {
    auto it = _playerZoneDefs.find(player);
    if (it == _playerZoneDefs.end()) return;
    
    auto& component = _registry->get<ZoneObjectiveComponent>(player);
    component.CurrentWave = waveNumber;
    
    // Emit wave start
    _waveSink.publish(player, waveNumber, true);
    
    // Notify client
    ZoneObjectiveEvent event;
    event.Type = ZoneObjectiveEvent::EventType::WaveStarted;
    event.WaveNumber = waveNumber;
    event.Message = "Wave " + std::to_string(waveNumber) + " started!";
    EmitEvent(player, event);
  }
  
  void ZoneObjectiveSystem::OnWaveComplete(entt::entity player, uint8_t waveNumber)
  {
    auto it = _playerZoneDefs.find(player);
    if (it == _playerZoneDefs.end()) return;
    
    auto& zoneDef = it->second;
    auto& component = _registry->get<ZoneObjectiveComponent>(player);
    
    // Check if all waves complete
    if (waveNumber >= zoneDef.waveCount)
    {
      component.ZoneComplete = true;
      
      // Emit zone complete
      _zoneCompleteSink.publish(player, component.ZoneId);
      
      // Notify client
      ZoneObjectiveEvent event;
      event.Type = ZoneObjectiveEvent::EventType::ZoneComplete;
      event.Message = "Zone complete!";
      EmitEvent(player, event);
      
      // Notify rewards available
      ZoneObjectiveEvent rewardEvent;
      rewardEvent.Type = ZoneObjectiveEvent::EventType::RewardsAvailable;
      rewardEvent.Message = "Rewards available!";
      EmitEvent(player, rewardEvent);
    }
    else
    {
      // Emit wave complete
      _waveSink.publish(player, waveNumber, false);
      
      // Notify client
      ZoneObjectiveEvent event;
      event.Type = ZoneObjectiveEvent::EventType::WaveComplete;
      event.WaveNumber = waveNumber;
      event.Message = "Wave " + std::to_string(waveNumber) + " complete!";
      EmitEvent(player, event);
    }
  }
  
  bool ZoneObjectiveSystem::CanAdvanceZone(entt::entity player) const
  {
    auto it = _playerZoneDefs.find(player);
    if (it == _playerZoneDefs.end()) return false;
    
    const auto& zoneDef = it->second;
    return CheckObjectivesComplete(player, zoneDef);
  }
  
  ZoneObjectiveComponent& ZoneObjectiveSystem::GetZoneState(entt::entity player)
  {
    return _registry->get<ZoneObjectiveComponent>(player);
  }
  
  const std::unordered_map<std::string, ObjectiveProgress>& ZoneObjectiveSystem::GetObjectives(entt::entity player) const
  {
    static std::unordered_map<std::string, ObjectiveProgress> empty;
    auto it = _playerObjectives.find(player);
    return (it != _playerObjectives.end()) ? it->second : empty;
  }
  
  void ZoneObjectiveSystem::Tick(float deltaTime)
  {
    if (!_registry) return;
    
    // Update time remaining for timed zones
    for (auto& [entity, zoneDef] : _playerZoneDefs)
    {
      if (_registry->has<ZoneObjectiveComponent>(entity))
      {
        auto& component = _registry->get<ZoneObjectiveComponent>(entity);
        if (component.TimeRemaining > 0)
        {
          component.TimeRemaining -= deltaTime;
          if (component.TimeRemaining < 0)
            component.TimeRemaining = 0;
        }
      }
    }
  }
  
  bool ZoneObjectiveSystem::CheckObjectivesComplete(entt::entity player, const ZoneDefinition& zoneDef)
  {
    auto it = _playerObjectives.find(player);
    if (it == _playerObjectives.end()) return false;
    
    const auto& objectives = it->second;
    
    // Check all required objectives
    for (const auto& obj : zoneDef.objectives)
    {
      if (!obj.required) continue;
      
      auto objIt = objectives.find(obj.id);
      if (objIt == objectives.end()) return false;
      if (!objIt->second.Complete) return false;
    }
    
    return true;
  }
  
  void ZoneObjectiveSystem::EmitEvent(entt::entity player, const ZoneObjectiveEvent& event)
  {
    // TODO: Integrate with snapshot system to send to client
    // This would serialize the event and send via NetworkManager
  }
} // namespace DarkAges