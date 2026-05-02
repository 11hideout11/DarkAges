#include "zones/ZoneObjectiveSystem.hpp"

#include <entt/entt.hpp>

#include <ctime>
#include <cstring>
#include <iostream>

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
      ZoneObjectiveProgress progress;
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
    if (_registry->all_of<ZoneObjectiveComponent>(player))
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
      _objectiveCompleteSignal.publish(player, objectiveId);
      
      // Notify client
      ZoneObjectiveEvent event;
      event.Type = ZoneObjectiveEvent::EventType::ObjectiveComplete;
      event.ObjectiveId = objectiveId;
      EmitEvent(player, event);
    }
    else
    {
      // Emit progress event
      _objectiveProgressSignal.publish(player, objectiveId, progress, objProgress.RequiredCount);
      
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
    if (!_registry) return;
    auto it = _playerZoneDefs.find(player);
    if (it == _playerZoneDefs.end()) return;
    
    if (!_registry->all_of<ZoneObjectiveComponent>(player)) return;
    auto& component = _registry->get<ZoneObjectiveComponent>(player);
    component.CurrentWave = waveNumber;
    
    // Emit wave start
    _waveSignal.publish(player, waveNumber, true);
    
    // Notify client
    ZoneObjectiveEvent event;
    event.Type = ZoneObjectiveEvent::EventType::WaveStarted;
    event.WaveNumber = waveNumber;
    event.Message = "Wave " + std::to_string(waveNumber) + " started!";
    EmitEvent(player, event);
  }
  
  void ZoneObjectiveSystem::OnWaveComplete(entt::entity player, uint8_t waveNumber)
  {
    if (!_registry) return;
    auto it = _playerZoneDefs.find(player);
    if (it == _playerZoneDefs.end()) return;
    
    auto& zoneDef = it->second;
    if (!_registry->all_of<ZoneObjectiveComponent>(player)) return;
    auto& component = _registry->get<ZoneObjectiveComponent>(player);
    
    // Check if all waves complete
    if (waveNumber >= zoneDef.waveCount)
    {
      component.ZoneComplete = true;
      
      // Emit zone complete
      _zoneCompleteSignal.publish(player, component.ZoneId);
      
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
      _waveSignal.publish(player, waveNumber, false);
      
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
  
  const std::unordered_map<std::string, ZoneObjectiveProgress>& ZoneObjectiveSystem::GetObjectives(entt::entity player) const
  {
    static std::unordered_map<std::string, ZoneObjectiveProgress> empty;
    auto it = _playerObjectives.find(player);
    return (it != _playerObjectives.end()) ? it->second : empty;
  }
  
  void ZoneObjectiveSystem::Tick(float deltaTime)
  {
    // Guard: null registry is fatal error - log and return
    if (!_registry) {
      std::cerr << "[ZoneObjectiveSystem] FATAL: Tick called with null registry!" << std::endl;
      return;
    }
    
    // Guard: invalid deltaTime (negative or NaN) can corrupt state
    if (deltaTime < 0.0f || deltaTime > 10.0f) {
      std::cerr << "[ZoneObjectiveSystem] WARNING: Invalid deltaTime=" << deltaTime << ", clamping to 0.033s (30fps)" << std::endl;
      deltaTime = 0.033f;  // 30fps max to prevent corruption
    }
    
    // Update time remaining for timed zones
    for (auto& [entity, zoneDef] : _playerZoneDefs)
    {
      // Skip invalid entities
      if (entity == entt::null) continue;
      
      // Entity may have been destroyed
      if (!_registry->all_of<ZoneObjectiveComponent>(entity)) continue;
      
      auto& component = _registry->get<ZoneObjectiveComponent>(entity);
      if (component.TimeRemaining > 0)
      {
        component.TimeRemaining -= deltaTime;
        if (component.TimeRemaining < 0)
          component.TimeRemaining = 0;
      }
    }
  }
  
  bool ZoneObjectiveSystem::CheckObjectivesComplete(entt::entity player, const ZoneDefinition& zoneDef) const
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
  
  void ZoneObjectiveSystem::EmitEvent(entt::entity player, const ZoneObjectiveEvent& event) {
    if (!network_ || !getConnectionId_) return;

    ConnectionID connId = getConnectionId_(player);
    if (connId == INVALID_CONNECTION) return;

    ZoneObjectiveUpdatePacket pkt{};
    pkt.eventType = static_cast<uint8_t>(event.Type);
    std::strncpy(pkt.objectiveId, event.ObjectiveId.c_str(), sizeof(pkt.objectiveId) - 1);
    pkt.objectiveId[sizeof(pkt.objectiveId) - 1] = '\0';
    pkt.currentProgress = event.CurrentProgress;
    pkt.requiredProgress = event.RequiredProgress;
    pkt.waveNumber = event.WaveNumber;
    std::strncpy(pkt.message, event.Message.c_str(), sizeof(pkt.message) - 1);
    pkt.message[sizeof(pkt.message) - 1] = '\0';

    network_->sendZoneObjectiveUpdate(connId, pkt);
  }
} // namespace DarkAges