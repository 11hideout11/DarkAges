#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <entt/entity/registry.hpp>

#include "zones/ZoneObjectiveSystem.hpp"
#include "zones/ZoneDefinition.hpp"

using namespace DarkAges;
using Catch::Approx;

TEST_CASE("ZoneObjectiveSystem", "[zones]")
{
  entt::registry registry;
  ZoneObjectiveSystem system;
  system.Initialize(registry);
  
  auto player = registry.create();
  
  SECTION("PlayerEnterZone initializes objectives")
  {
    ZoneDefinition zoneDef;
    zoneDef.zoneId = 98;
    zoneDef.timeLimit = 120.0f;
    zoneDef.waveCount = 0;
    
    ZoneDefinition::Objective obj1;
    obj1.id = "talk_trainer";
    obj1.type = ZoneDefinition::Objective::Type::Interact;
    obj1.required = true;
    obj1.requiredCount = 1;
    zoneDef.objectives.push_back(obj1);
    
    ZoneDefinition::Objective obj2;
    obj2.id = "hit_dummy";
    obj2.type = ZoneDefinition::Objective::Type::Damage;
    obj2.required = true;
    obj2.requiredCount = 5;
    zoneDef.objectives.push_back(obj2);
    
    system.OnPlayerEnterZone(player, 98, zoneDef);
    
    auto& state = system.GetZoneState(player);
    REQUIRE(state.ZoneId == 98);
    REQUIRE(state.TimeRemaining == 120.0f);
    REQUIRE_FALSE(state.ZoneComplete);
    
    const auto& objectives = system.GetObjectives(player);
    REQUIRE(objectives.size() == 2);
    REQUIRE(objectives.count("talk_trainer") == 1);
    REQUIRE(objectives.count("hit_dummy") == 1);
  }
  
  SECTION("Objective progress completes")
  {
    ZoneDefinition zoneDef;
    zoneDef.zoneId = 98;
    zoneDef.waveCount = 0;
    
    ZoneDefinition::Objective obj;
    obj.id = "kill_5";
    obj.type = ZoneDefinition::Objective::Type::Kill;
    obj.required = true;
    obj.requiredCount = 5;
    zoneDef.objectives.push_back(obj);
    
    system.OnPlayerEnterZone(player, 98, zoneDef);
    
    // Progress to completion
    system.OnObjectiveProgress(player, "kill_5", 3);
    
    auto& objectives = system.GetObjectives(player);
    REQUIRE_FALSE(objectives.at("kill_5").Complete);
    
    system.OnObjectiveProgress(player, "kill_5", 5);
    
    const auto& objectives2 = system.GetObjectives(player);
    REQUIRE(objectives2.at("kill_5").Complete);
  }
  
  SECTION("Wave progression works")
  {
    ZoneDefinition zoneDef;
    zoneDef.zoneId = 99;
    zoneDef.waveCount = 3;
    
    system.OnPlayerEnterZone(player, 99, zoneDef);
    
    auto& state = system.GetZoneState(player);
    REQUIRE(state.CurrentWave == 0);
    REQUIRE(state.TotalWaves == 3);
    
    system.OnWaveStart(player, 1);
    REQUIRE(system.GetZoneState(player).CurrentWave == 1);
    
    system.OnWaveComplete(player, 1);
    REQUIRE(system.GetZoneState(player).CurrentWave == 1);
    
    system.OnWaveComplete(player, 3);
    REQUIRE(system.GetZoneState(player).ZoneComplete);
  }
  
  SECTION("CanAdvanceZone checks completion")
  {
    ZoneDefinition zoneDef;
    zoneDef.zoneId = 98;
    zoneDef.waveCount = 0;
    
    ZoneDefinition::Objective obj;
    obj.id = "complete";
    obj.type = ZoneDefinition::Objective::Type::Custom;
    obj.required = true;
    obj.requiredCount = 1;
    zoneDef.objectives.push_back(obj);
    
    system.OnPlayerEnterZone(player, 98, zoneDef);
    
    REQUIRE_FALSE(system.CanAdvanceZone(player));
    
    system.OnObjectiveProgress(player, "complete", 1);
    REQUIRE(system.CanAdvanceZone(player));
  }
  
  SECTION("PlayerLeaveZone cleans up")
  {
    ZoneDefinition zoneDef;
    zoneDef.zoneId = 98;
    zoneDef.waveCount = 0;
    
    system.OnPlayerEnterZone(player, 98, zoneDef);
    system.OnPlayerLeaveZone(player);
    
    const auto& objectives = system.GetObjectives(player);
    REQUIRE(objectives.empty());
  }
  
  SECTION("Timer ticks")
  {
    ZoneDefinition zoneDef;
    zoneDef.zoneId = 98;
    zoneDef.timeLimit = 60.0f;
    zoneDef.waveCount = 0;
    
    system.OnPlayerEnterZone(player, 98, zoneDef);
    
    system.Tick(1.0f);
    
    auto& state = system.GetZoneState(player);
    REQUIRE(state.TimeRemaining == Approx(59.0f));
  }
}