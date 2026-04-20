// Input handling implementation
// Extracted from ZoneServer.cpp to improve code organization

#include "zones/InputHandler.hpp"
#include "zones/ZoneServer.hpp"
#include "zones/PlayerManager.hpp"
#include "zones/CombatEventHandler.hpp"
#include "combat/AbilitySystem.hpp"
#include "netcode/NetworkManager.hpp"
#include "netcode/ProtobufProtocol.hpp"
#include "security/AntiCheat.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

namespace DarkAges {

InputHandler::InputHandler(ZoneServer& server)
    : server_(server) {
}

void InputHandler::onClientInput(const ClientInputPacket& input) {
    // Use PlayerManager to find entity for connection
    EntityID entity = playerManager_->getEntityByConnection(input.connectionId);
    if (entity == entt::null) {
        return;  // Unknown connection
    }

    validateAndApplyInput(entity, input);
}

void InputHandler::validateAndApplyInput(EntityID entity, const ClientInputPacket& input) {
    auto& registry = server_.getRegistry();
    uint32_t currentTimeMs = server_.getCurrentTimeMs();
    const auto& config = server_.getConfig();

    // [SECURITY_AGENT] Rate limiting check - prevent packet flooding
    auto rateResult = antiCheat_->checkRateLimit(entity, currentTimeMs, registry);
    if (rateResult.detected) {
        std::cerr << "[ZONE " << config.zoneId << "] Rate limit exceeded for entity "
                  << static_cast<uint32_t>(entity) << std::endl;

        auto* entityToConnection = server_.getEntityToConnectionPtr();
        auto connIt = entityToConnection->find(entity);
        if (connIt != entityToConnection->end()) {
            network_->disconnect(connIt->second, "Rate limit exceeded");
        }
        return;
    }

    // [SECURITY_AGENT] Input validation - check for invalid input values
    auto inputResult = antiCheat_->validateInput(entity, input.input, currentTimeMs, registry);
    if (inputResult.detected) {
        std::cerr << "[ZONE " << config.zoneId << "] Input validation failed: "
                  << inputResult.description << std::endl;

        // Kick for critical input manipulation
        if (inputResult.severity == Security::ViolationSeverity::CRITICAL) {
            auto* entityToConnection = server_.getEntityToConnectionPtr();
            auto connIt = entityToConnection->find(entity);
            if (connIt != entityToConnection->end()) {
                network_->disconnect(connIt->second, inputResult.description);
            }
            return;
        }
        // Otherwise continue with sanitized input (logged as warning)
    }

    // Store old position for anti-cheat comparison
    Position oldPos{0, 0, 0, 0};
    if (Position* pos = registry.try_get<Position>(entity)) {
        oldPos = *pos;
    }

    // Apply input with physics validation
    auto result = movementSystem_->applyInput(registry, entity, input.input, currentTimeMs);

    // [SECURITY_AGENT] Comprehensive anti-cheat validation
    if (!result.valid) {
        // Basic movement validation failed (movement system level)
        std::cout << "[ZONE " << config.zoneId << "] Movement validation failed: "
                  << result.violationReason << std::endl;

        // Apply basic correction
        if (Position* pos = registry.try_get<Position>(entity)) {
            *pos = result.correctedPosition;
        }
        if (Velocity* vel = registry.try_get<Velocity>(entity)) {
            *vel = result.correctedVelocity;
        }
    }

    // [SECURITY_AGENT] Advanced anti-cheat validation
    if (Position* newPos = registry.try_get<Position>(entity)) {
        // Calculate delta time from input timestamp
        uint32_t dtMs = 16;  // Default to one tick
        if (NetworkState* netState = registry.try_get<NetworkState>(entity)) {
            if (netState->lastInputTime > 0) {
                dtMs = currentTimeMs - netState->lastInputTime;
                // Clamp to reasonable range to prevent exploits
                dtMs = std::min(dtMs, 1000u);  // Max 1 second
                dtMs = std::max(dtMs, 1u);     // Min 1ms
            }
        }

        // Run comprehensive anti-cheat checks
        auto cheatResult = antiCheat_->validateMovement(entity, oldPos, *newPos,
                                                        dtMs, input.input, registry);

        if (cheatResult.detected) {
            std::cerr << "[ANTICHEAT] " << cheatResult.description
                      << " [confidence: " << static_cast<int>(cheatResult.confidence * 100) << "%]"
                      << " for entity " << static_cast<uint32_t>(entity) << std::endl;

            // Apply position correction from anti-cheat
            *newPos = cheatResult.correctedPosition;

            // Send position correction to client (server authority)
            auto* entityToConnection = server_.getEntityToConnectionPtr();
            auto connIt = entityToConnection->find(entity);
            if (connIt != entityToConnection->end()) {
                Velocity vel{};
                if (Velocity* v = registry.try_get<Velocity>(entity)) {
                    vel = *v;
                }
                Security::ServerAuthority::sendPositionCorrection(
                    connIt->second, *newPos, vel, input.input.sequence);

                // Handle kick/ban based on severity
                if (cheatResult.severity == Security::ViolationSeverity::CRITICAL ||
                    cheatResult.severity == Security::ViolationSeverity::BAN) {
                    network_->disconnect(connIt->second, cheatResult.description);

                    // Log the enforcement action
                    std::cerr << "[ANTICHEAT] Player " << connIt->second
                              << (cheatResult.severity == Security::ViolationSeverity::BAN
                                  ? " banned" : " kicked")
                              << " for " << cheatResult.description << std::endl;
                    return;
                }
            }
        }

        // Update anti-cheat state tracking
        if (AntiCheatState* cheatState = registry.try_get<AntiCheatState>(entity)) {
            cheatState->lastValidPosition = *newPos;
            cheatState->lastValidationTime = currentTimeMs;

            // Track max recorded speed for behavior analysis
            Velocity vel{};
            if (Velocity* v = registry.try_get<Velocity>(entity)) {
                vel = *v;
                float speed = vel.speed();
                if (speed > cheatState->maxRecordedSpeed) {
                    cheatState->maxRecordedSpeed = speed;
                }
            }
        }
    }

    // Update network state
    if (NetworkState* netState = registry.try_get<NetworkState>(entity)) {
        netState->lastInputSequence = input.input.sequence;
        netState->lastInputTime = input.receiveTimeMs;
    }

    // [PHASE 3C] Process combat input — ability cast or melee attack
    if (input.input.abilitySlot > 0) {
        // Player is casting an ability from their loadout
        processAbilityInput(entity, input);
    } else if (input.input.attack) {
        // Standard melee attack
        if (combatEventHandler_) {
            combatEventHandler_->processAttackInput(entity, input);
        }
    }
}

void InputHandler::processAttackInput(EntityID entity, const ClientInputPacket& input) {
    auto& registry = server_.getRegistry();

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

    // Get RTT for lag compensation
    uint32_t rttMs = 100;  // Default fallback
    if (const NetworkState* netState = registry.try_get<NetworkState>(entity)) {
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
    lagAttack.serverTimestamp = server_.getCurrentTimeMs();
    lagAttack.rttMs = rttMs;

    // Process attack with lag compensation
    // This rewinds all potential targets to their positions at attack time
    auto* combatSystem = server_.getCombatSystemPtr();
    auto* lagCompensator = server_.getLagCompensatorPtr();
    if (!combatSystem || !lagCompensator) return;

    LagCompensatedCombat lagCombat(*combatSystem, *lagCompensator);
    auto hits = lagCombat.processAttackWithRewind(registry, lagAttack);

    // Send hit results to relevant clients
    auto* entityToConnection = server_.getEntityToConnectionPtr();
    for (const auto& hit : hits) {
        if (hit.hit) {
            // [SECURITY_AGENT] Validate combat action for anti-cheat
            Position attackerPos{0, 0, 0, 0};
            if (const Position* pos = registry.try_get<Position>(entity)) {
                attackerPos = *pos;
            }

            auto combatValidation = antiCheat_->validateCombat(entity, hit.target,
                                                               hit.damageDealt, hit.hitLocation,
                                                               attackerPos, registry);

            if (combatValidation.detected) {
                std::cerr << "[ANTICHEAT] Combat validation failed: "
                          << combatValidation.description << std::endl;

                // Skip this hit - don't apply damage
                if (combatValidation.severity == Security::ViolationSeverity::CRITICAL ||
                    combatValidation.severity == Security::ViolationSeverity::BAN) {
                    // Kick/ban the player
                    auto connIt = entityToConnection->find(entity);
                    if (connIt != entityToConnection->end()) {
                        network_->disconnect(connIt->second, combatValidation.description);
                    }
                    return;
                }
                continue;  // Skip applying this hit
            }

            // [NETWORK_AGENT] Send damage event to target
            auto targetConnIt = entityToConnection->find(hit.target);
            if (targetConnIt != entityToConnection->end()) {
                auto damageEvent = Netcode::ProtobufProtocol::createDamageEvent(
                    static_cast<uint32_t>(entity),
                    static_cast<uint32_t>(hit.target),
                    static_cast<int32_t>(hit.damageDealt)
                );
                damageEvent.set_timestamp(server_.getCurrentTimeMs());
                auto eventData = Netcode::ProtobufProtocol::serializeEvent(damageEvent);
                network_->sendEvent(targetConnIt->second, eventData);
                std::cerr << "[NETWORK] Sent damage event: " << hit.damageDealt
                          << " to entity " << static_cast<uint32_t>(hit.target) << std::endl;
            }

            // [NETWORK_AGENT] Send hit confirmation to attacker
            auto attackerConnIt = entityToConnection->find(entity);
            if (attackerConnIt != entityToConnection->end()) {
                auto hitConfirm = Netcode::ProtobufProtocol::createDamageEvent(
                    static_cast<uint32_t>(entity),
                    static_cast<uint32_t>(hit.target),
                    static_cast<int32_t>(hit.damageDealt)
                );
                hitConfirm.set_timestamp(server_.getCurrentTimeMs());
                auto eventData = Netcode::ProtobufProtocol::serializeEvent(hitConfirm);
                network_->sendEvent(attackerConnIt->second, eventData);
                std::cerr << "[NETWORK] Sent hit confirmation: " << hit.damageDealt
                          << " to attacker entity " << static_cast<uint32_t>(entity) << std::endl;
            }

            // [DATABASE_AGENT] Log combat event for analytics
        }
    }
}

void InputHandler::processAbilityInput(EntityID entity, const ClientInputPacket& input) {
    auto& registry = server_.getRegistry();
    uint32_t currentTimeMs = server_.getCurrentTimeMs();

    // Get the ability from the player's loadout
    const AbilityLoadout* loadout = registry.try_get<AbilityLoadout>(entity);
    if (!loadout) {
        return; // Player has no abilities
    }

    uint8_t slot = input.input.abilitySlot;
    if (slot == 0 || slot > MAX_ABILITY_SLOTS) {
        return; // Invalid slot
    }

    uint32_t abilityId = loadout->getAbilityInSlot(slot);
    if (abilityId == 0) {
        return; // No ability in this slot
    }

    if (!abilitySystem_) {
        return; // No ability system
    }

    // Get the ability definition
    const AbilityDefinition* ability = abilitySystem_->getAbility(abilityId);
    if (!ability) {
        return;
    }

    // Determine target — use targetEntity from input, or self-target for heals
    EntityID target = entt::null;
    if (input.input.targetEntity > 0) {
        // Look up target entity (it's a raw entity ID from the client)
        EntityID targetEntity = static_cast<EntityID>(input.input.targetEntity);
        if (registry.valid(targetEntity)) {
            target = targetEntity;
        }
    }

    // Self-target for heal abilities if no target specified
    if (target == entt::null && ability->effectType == AbilityEffectType::Heal) {
        target = entity;
    }

    if (target == entt::null) {
        return; // Need a target
    }

    // Cast the ability
    bool success = abilitySystem_->castAbility(registry, entity, target, *ability, currentTimeMs);

    if (success) {
        std::cout << "[ZONE " << server_.getConfig().zoneId << "] Entity "
                  << static_cast<uint32_t>(entity) << " cast ability '"
                  << ability->name << "' on entity "
                  << static_cast<uint32_t>(target) << std::endl;

        // Send ability cast event to relevant clients
        if (network_) {
            auto* entityToConnection = server_.getEntityToConnectionPtr();
            auto connIt = entityToConnection->find(entity);
            if (connIt != entityToConnection->end()) {
                // Notify attacker of successful cast
                auto castEvent = Netcode::ProtobufProtocol::createDamageEvent(
                    static_cast<uint32_t>(entity),
                    static_cast<uint32_t>(target),
                    0  // Ability cast, not direct damage
                );
                castEvent.set_timestamp(currentTimeMs);
                auto eventData = Netcode::ProtobufProtocol::serializeEvent(castEvent);
                network_->sendEvent(connIt->second, eventData);
            }
        }
    }
}

} // namespace DarkAges
