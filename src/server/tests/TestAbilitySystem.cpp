// [COMBAT_AGENT] Unit tests for AbilitySystem

#include <catch2/catch_test_macros.hpp>
#include "combat/AbilitySystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <string>

using namespace DarkAges;

TEST_CASE("Ability validation", "[AbilitySystem]") {
    SECTION("Valid ability passes validation") {
        AbilityDefinition ability("Fireball", 5000, 100, 30.0f, AbilityEffectType::Damage);
        REQUIRE(ability.validate() == true);
    }

    SECTION("Ability with empty name fails validation") {
        AbilityDefinition ability("", 5000, 100, 30.0f, AbilityEffectType::Damage);
        REQUIRE(ability.validate() == false);
    }
}

TEST_CASE("AbilityEffectType enum values", "[AbilitySystem]") {
    REQUIRE(static_cast<uint8_t>(AbilityEffectType::Damage) == 1);
    REQUIRE(static_cast<uint8_t>(AbilityEffectType::Heal) == 2);
    REQUIRE(static_cast<uint8_t>(AbilityEffectType::Buff) == 3);
    REQUIRE(static_cast<uint8_t>(AbilityEffectType::Debuff) == 4);
    REQUIRE(static_cast<uint8_t>(AbilityEffectType::Status) == 5);
}

TEST_CASE("AbilitySystem stub methods", "[AbilitySystem]") {
    AbilitySystem system;

    SECTION("getAbility returns dummy ability") {
        const AbilityDefinition* ability = system.getAbility(1);
        REQUIRE(ability != nullptr);
        REQUIRE(ability->name == "Dummy");
    }

    SECTION("hasMana returns true for stub") {
        Registry registry;
        AbilityDefinition ability("Fireball", 5000, 100, 30.0f, AbilityEffectType::Damage);
        REQUIRE(system.hasMana(registry, entt::null, ability) == true);
    }

    SECTION("castAbility returns false for stub") {
        Registry registry;
        AbilityDefinition ability("Fireball", 5000, 100, 30.0f, AbilityEffectType::Damage);
        REQUIRE(system.castAbility(registry, entt::null, entt::null, ability, 1000) == false);
    }
}