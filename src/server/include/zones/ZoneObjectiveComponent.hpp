#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace DarkAges
{
  /**
   * ZoneObjectiveComponent - Tracks player progress on zone objectives
   * 
   * Attached to player entity when they enter a zone with objectives.
   * Tracks objective completion state and progress counters.
   */
  struct ZoneObjectiveComponent
  {
    /// Zone ID this component belongs to
    uint16_t ZoneId = 0;
    
    /// Current wave in wave-based zones (0 = not active)
    uint8_t CurrentWave = 0;
    
    /// Total waves in zone (0 = no waves)
    uint8_t TotalWaves = 0;
    
    /// Time remaining in zone (seconds, 0 = no limit)
    float TimeRemaining = 0.0f;
    
    /// Whether zone is complete
    bool ZoneComplete = false;
    
    /// Whether player has claimed rewards
    bool RewardsClaimed = false;
  };
  
  /**
   * ZoneObjectiveProgress - Tracks progress on a single zone objective
   */
  struct ZoneObjectiveProgress
  {
    /// Objective ID from zone config
    std::string ObjectiveId;
    
    /// Current count toward completion
    uint16_t CurrentCount = 0;
    
    /// Required count for completion
    uint16_t RequiredCount = 1;
    
    /// Whether objective is complete
    bool Complete = false;
    
    /// Timestamp when completed (0 = not complete)
    uint32_t CompletedAt = 0;
  };
  
  /**
   * ZoneObjectiveEvent - Event sent to client for UI display
   */
  struct ZoneObjectiveEvent
  {
    enum class EventType : uint8_t
    {
      ObjectiveStarted = 0,
      ObjectiveProgress = 1,
      ObjectiveComplete = 2,
      WaveStarted = 3,
      WaveComplete = 4,
      ZoneComplete = 5,
      RewardsAvailable = 6
    };
    
    /// Event type
    EventType Type = EventType::ObjectiveStarted;
    
    /// Objective ID (for objective events)
    std::string ObjectiveId;
    
    /// Current progress (for progress events)
    uint16_t CurrentProgress = 0;
    
    /// Required progress (for progress events)
    uint16_t RequiredProgress = 0;
    
    /// Wave number (for wave events)
    uint8_t WaveNumber = 0;
    
    /// Optional message text
    std::string Message;
  };
} // namespace DarkAges