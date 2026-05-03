// Zone Event System implementation
// Manages world events, world bosses, and zone-wide activities

#include "combat/ZoneEventSystem.hpp"
#include "combat/ChatSystem.hpp"
#include "combat/ExperienceSystem.hpp"
#include "combat/ItemSystem.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace DarkAges {

// ============================================================================
// EVENT REGISTRY
// ============================================================================

void ZoneEventSystem::registerEvent(const ZoneEventDefinition& definition) {
    for (auto& ev : events_) {
        if (ev.eventId == definition.eventId) {
            ev = definition;
            return;
        }
    }
    events_.push_back(definition);
}

const ZoneEventDefinition* ZoneEventSystem::getEvent(uint32_t eventId) const {
    for (const auto& ev : events_) {
        if (ev.eventId == eventId) return &ev;
    }
    return nullptr;
}

std::vector<uint32_t> ZoneEventSystem::getRegisteredEventIds() const {
    std::vector<uint32_t> ids;
    ids.reserve(events_.size());
    for (const auto& ev : events_) {
        ids.push_back(ev.eventId);
    }
    return ids;
}

// ============================================================================
// EVENT LIFECYCLE
// ============================================================================

bool ZoneEventSystem::startEvent(Registry& registry, uint32_t eventId,
                                  uint32_t currentTimeMs) {
    if (hasActiveEvent()) return false;

    const ZoneEventDefinition* def = getEvent(eventId);
    if (!def) return false;

    // Check cooldown
    for (size_t i = 0; i < events_.size(); ++i) {
        if (events_[i].eventId == eventId) {
            uint32_t lastCompleted = lastCompletionTime_[i];
            if (lastCompleted > 0 &&
                (currentTimeMs - lastCompleted) < def->cooldownMs) {
                return false;
            }
            break;
        }
    }

    activeEvent_.reset();
    activeEvent_.definition = def;
    activeEvent_.eventStartTimeMs = currentTimeMs;
    activeEvent_.lastStateChangeMs = currentTimeMs;

    transitionState(registry, ZoneEventState::Announcing, currentTimeMs);
    return true;
}

bool ZoneEventSystem::endEvent(Registry& registry, uint32_t currentTimeMs) {
    if (!hasActiveEvent()) return false;

    // Only distribute rewards if event was still active (not already distributed in completing transition)
    if (activeEvent_.state == ZoneEventState::Active ||
        activeEvent_.state == ZoneEventState::Announcing) {
        distributeRewards(registry, currentTimeMs);
    }

    // Clear all player event components
    auto view = registry.view<ZoneEventComponent>();
    for (auto entity : view) {
        auto& zec = view.get<ZoneEventComponent>(entity);
        zec.activeEventId = 0;
        zec.inEvent = false;
        zec.joinTimeMs = 0;
    }

    // Record cooldown start
    if (activeEvent_.definition) {
        for (size_t i = 0; i < events_.size(); ++i) {
            if (events_[i].eventId == activeEvent_.definition->eventId) {
                lastCompletionTime_[i] = currentTimeMs;
                break;
            }
        }
    }

    activeEvent_.reset();
    return true;
}

// ============================================================================
// TICK UPDATE
// ============================================================================

void ZoneEventSystem::update(Registry& registry, uint32_t currentTimeMs) {
    if (activeEvent_.state == ZoneEventState::Inactive) return;
    if (!activeEvent_.definition) return;

    switch (activeEvent_.state) {
        case ZoneEventState::Announcing: {
            if (currentTimeMs >= activeEvent_.announcementEndTimeMs) {
                transitionState(registry, ZoneEventState::Active, currentTimeMs);
            }
            break;
        }

        case ZoneEventState::Active: {
            // Priority: health threshold triggers immediate phase advance
            if (activeEvent_.bossEntity != entt::null) {
                const CombatState* cs = registry.try_get<CombatState>(activeEvent_.bossEntity);
                const BossProfile* bp = registry.try_get<BossProfile>(activeEvent_.bossEntity);
                if (cs && bp && !cs->isDead && bp->phaseCount > activeEvent_.currentPhase + 1) {
                    float healthPct = static_cast<float>(cs->health) / static_cast<float>(cs->maxHealth);
                    uint32_t nextPhase = activeEvent_.currentPhase + 1;
                    if (healthPct <= bp->phaseHealthThresholds[nextPhase]) {
                        advancePhase(registry, currentTimeMs);
                        break;
                    }
                }
            }

            // Check if current phase has timed out
            const ZoneEventPhaseDefinition* phase = getCurrentPhaseDef();
            if (phase && phase->durationMs > 0) {
                uint32_t elapsed = currentTimeMs - activeEvent_.phaseStartTimeMs;
                if (elapsed >= phase->durationMs) {
                    advancePhase(registry, currentTimeMs);
                    break;
                }
            }

            // Check if current phase objectives are complete
            if (arePhaseObjectivesComplete()) {
                advancePhase(registry, currentTimeMs);
            }
            break;
        }

        case ZoneEventState::Completing: {
            uint32_t completingDuration = 3000;
            uint32_t elapsed = currentTimeMs - activeEvent_.lastStateChangeMs;
            if (elapsed >= completingDuration) {
                endEvent(registry, currentTimeMs);
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// PARTICIPATION TRACKING
// ============================================================================

void ZoneEventSystem::onDamageDealt(Registry& registry, EntityID attacker,
                                     EntityID target, uint32_t damage,
                                     uint32_t currentTimeMs) {
    if (activeEvent_.state != ZoneEventState::Active) return;

    const PlayerInfo* info = registry.try_get<PlayerInfo>(attacker);
    if (!info) return;

    EventParticipation* p = activeEvent_.findParticipant(info->playerId);
    if (!p) {
        p = activeEvent_.addParticipant(info->playerId);
    }
    if (p) {
        p->totalDamage += damage;
        p->contributed = true;
    }

    // Track damage objective
    const ZoneEventPhaseDefinition* phase = getCurrentPhaseDef();
    if (phase) {
        for (uint32_t i = 0; i < phase->objectiveCount && i < MAX_EVENT_OBJECTIVES; ++i) {
            if (phase->objectives[i].type == EventObjectiveType::TotalDamage) {
                activeEvent_.objectiveProgress[i] += damage;
            }
        }
    }
}

void ZoneEventSystem::onNPCKilled(Registry& registry, EntityID killer,
                                   EntityID victim, uint32_t npcArchetypeId,
                                   uint32_t currentTimeMs) {
    if (activeEvent_.state != ZoneEventState::Active) return;

    const PlayerInfo* info = registry.try_get<PlayerInfo>(killer);
    if (!info) return;

    EventParticipation* p = activeEvent_.findParticipant(info->playerId);
    if (!p) {
        p = activeEvent_.addParticipant(info->playerId);
    }
    if (p) {
        p->kills++;
        p->contributed = true;
    }

    // Track kill objectives
    const ZoneEventPhaseDefinition* phase = getCurrentPhaseDef();
    if (phase) {
        for (uint32_t i = 0; i < phase->objectiveCount && i < MAX_EVENT_OBJECTIVES; ++i) {
            const ZoneEventObjective& obj = phase->objectives[i];
            if (obj.type == EventObjectiveType::KillNPC &&
                (obj.targetId == 0 || obj.targetId == npcArchetypeId)) {
                activeEvent_.objectiveProgress[i]++;
            }
        }
    }

    // Check if victim was the event boss (by entity reference OR by archetype)
    bool isBoss = false;
    if (activeEvent_.bossEntity != entt::null && activeEvent_.bossEntity == victim) {
        isBoss = true;
    } else if (phase) {
        for (uint32_t i = 0; i < phase->objectiveCount && i < MAX_EVENT_OBJECTIVES; ++i) {
            if (phase->objectives[i].type == EventObjectiveType::KillBoss &&
                phase->bossNpcArchetypeId > 0 && npcArchetypeId == phase->bossNpcArchetypeId) {
                isBoss = true;
                break;
            }
        }
    }

    if (isBoss && phase) {
        for (uint32_t i = 0; i < phase->objectiveCount && i < MAX_EVENT_OBJECTIVES; ++i) {
            if (phase->objectives[i].type == EventObjectiveType::KillBoss) {
                activeEvent_.objectiveProgress[i]++;
            }
        }
    }
}

void ZoneEventSystem::onPlayerDied(Registry& registry, EntityID player,
                                    uint32_t currentTimeMs) {
    if (activeEvent_.state != ZoneEventState::Active) return;

    const PlayerInfo* info = registry.try_get<PlayerInfo>(player);
    if (!info) return;

    EventParticipation* p = activeEvent_.findParticipant(info->playerId);
    if (p) {
        p->deaths++;
    }
}

bool ZoneEventSystem::joinEvent(Registry& registry, EntityID player,
                                 uint32_t currentTimeMs) {
    if (activeEvent_.state != ZoneEventState::Active &&
        activeEvent_.state != ZoneEventState::Announcing) {
        return false;
    }

    const PlayerInfo* info = registry.try_get<PlayerInfo>(player);
    if (!info) return false;

    if (activeEvent_.definition &&
        activeEvent_.definition->maxParticipants > 0 &&
        activeEvent_.participantCount >= activeEvent_.definition->maxParticipants) {
        return false;
    }

    EventParticipation* p = activeEvent_.addParticipant(info->playerId);
    if (!p) return false;

    if (!registry.all_of<ZoneEventComponent>(player)) {
        registry.emplace<ZoneEventComponent>(player);
    }
    ZoneEventComponent& zec = registry.get<ZoneEventComponent>(player);
    zec.activeEventId = activeEvent_.definition ? activeEvent_.definition->eventId : 0;
    zec.inEvent = true;
    zec.joinTimeMs = currentTimeMs;

    return true;
}

bool ZoneEventSystem::leaveEvent(Registry& registry, EntityID player) {
    if (!hasActiveEvent()) return false;

    const PlayerInfo* info = registry.try_get<PlayerInfo>(player);
    if (!info) return false;

    // Check if player is actually participating
    if (!activeEvent_.findParticipant(info->playerId)) return false;

    // Clear the player's event component
    if (registry.all_of<ZoneEventComponent>(player)) {
        ZoneEventComponent& zec = registry.get<ZoneEventComponent>(player);
        zec.activeEventId = 0;
        zec.inEvent = false;
        zec.joinTimeMs = 0;
    }

    return true;
}

// ============================================================================
// QUERIES
// ============================================================================

const ZoneEventPhaseDefinition* ZoneEventSystem::getCurrentPhaseDef() const {
    if (!activeEvent_.definition) return nullptr;
    if (activeEvent_.currentPhase >= activeEvent_.definition->phaseCount) return nullptr;
    return &activeEvent_.definition->phases[activeEvent_.currentPhase];
}

// ============================================================================
// INTERNAL LOGIC
// ============================================================================

void ZoneEventSystem::transitionState(Registry& registry, ZoneEventState newState,
                                       uint32_t currentTimeMs) {
    ZoneEventState oldState = activeEvent_.state;
    activeEvent_.state = newState;
    activeEvent_.lastStateChangeMs = currentTimeMs;

    switch (newState) {
        case ZoneEventState::Announcing: {
            activeEvent_.announcementEndTimeMs =
                currentTimeMs + config_.announcementDurationMs;

            if (activeEvent_.definition) {
                std::string msg = "[EVENT] ";
                msg += activeEvent_.definition->name;
                msg += " is about to begin! Prepare yourselves!";
                sendEventMessage(registry, msg.c_str());
            }
            break;
        }

        case ZoneEventState::Active: {
            activeEvent_.currentPhase = 0;
            activeEvent_.phaseStartTimeMs = currentTimeMs;

            spawnPhaseNPCs(registry, currentTimeMs);

            if (activeEvent_.definition) {
                std::string msg = "[EVENT] ";
                msg += activeEvent_.definition->name;
                msg += " has begun!";
                sendEventMessage(registry, msg.c_str());
            }
            break;
        }

        case ZoneEventState::Completing: {
            distributeRewards(registry, currentTimeMs);

            if (activeEvent_.definition) {
                std::string msg = "[EVENT] ";
                msg += activeEvent_.definition->name;
                msg += " completed! Rewards distributed!";
                sendEventMessage(registry, msg.c_str());
            }

            // Clear player event components
            auto view = registry.view<ZoneEventComponent>();
            for (auto entity : view) {
                auto& zec = view.get<ZoneEventComponent>(entity);
                zec.activeEventId = 0;
                zec.inEvent = false;
                zec.joinTimeMs = 0;
            }
            break;
        }

        default:
            break;
    }

    if (eventStateCallback_ && activeEvent_.definition) {
        eventStateCallback_(activeEvent_.definition->eventId, newState);
    }
}

void ZoneEventSystem::advancePhase(Registry& registry, uint32_t currentTimeMs) {
    if (!activeEvent_.definition) return;

    uint32_t nextPhase = activeEvent_.currentPhase + 1;

    if (nextPhase >= activeEvent_.definition->phaseCount) {
        transitionState(registry, ZoneEventState::Completing, currentTimeMs);
        return;
    }

    activeEvent_.currentPhase = nextPhase;
    activeEvent_.phaseStartTimeMs = currentTimeMs;

    // Sync BossProfile currentPhase to match event phase (so AI uses correct abilities)
    if (activeEvent_.bossEntity != entt::null) {
        BossProfile* bp = registry.try_get<BossProfile>(activeEvent_.bossEntity);
        if (bp) {
            bp->currentPhase = nextPhase;
        }
    }

    // Reset objective progress for new phase
    for (uint32_t i = 0; i < MAX_EVENT_OBJECTIVES; ++i) {
        activeEvent_.objectiveProgress[i] = 0;
    }

    spawnPhaseNPCs(registry, currentTimeMs);

    const ZoneEventPhaseDefinition* phase = getCurrentPhaseDef();
    if (phase) {
        std::string msg = "[EVENT] Phase: ";
        msg += phase->name;
        if (phase->description[0]) {
            msg += " - ";
            msg += phase->description;
        }
        sendEventMessage(registry, msg.c_str());
    }

    if (phaseChangeCallback_ && activeEvent_.definition) {
        phaseChangeCallback_(activeEvent_.definition->eventId, nextPhase);
    }
}

void ZoneEventSystem::spawnPhaseNPCs(Registry& registry, uint32_t currentTimeMs) {
    const ZoneEventPhaseDefinition* phase = getCurrentPhaseDef();
    if (!phase || !activeEvent_.definition) return;

    float spawnX = activeEvent_.definition->spawnX;
    float spawnZ = activeEvent_.definition->spawnZ;

    if (phase->npcCount > 0 && phase->npcArchetypeId > 0 && spawnCallback_) {
        spawnCallback_(spawnX, spawnZ, phase->npcArchetypeId,
                       phase->npcLevel, phase->npcCount);
    }

    if (phase->bossNpcArchetypeId > 0 && bossSpawnCallback_) {
        // Only spawn the boss once — if already present, skip (phase transition without respawning)
        if (activeEvent_.bossEntity == entt::null) {
            EntityID boss = bossSpawnCallback_(spawnX, spawnZ, phase->bossLevel);
            activeEvent_.bossEntity = boss;
        }
    }
}

bool ZoneEventSystem::arePhaseObjectivesComplete() const {
    const ZoneEventPhaseDefinition* phase = getCurrentPhaseDef();
    if (!phase) return false;

    for (uint32_t i = 0; i < phase->objectiveCount && i < MAX_EVENT_OBJECTIVES; ++i) {
        if (activeEvent_.objectiveProgress[i] < phase->objectives[i].requiredCount) {
            return false;
        }
    }
    return true;
}

void ZoneEventSystem::distributeRewards(Registry& registry, uint32_t currentTimeMs) {
    if (!activeEvent_.definition) return;

    const ZoneEventReward& reward = activeEvent_.definition->reward;
    const EventParticipation* topContributor = findTopContributor();

    for (uint32_t i = 0; i < activeEvent_.participantCount; ++i) {
        const EventParticipation& p = activeEvent_.participants[i];

        if (!p.contributed) continue;

        auto view = registry.view<PlayerInfo>();
        EntityID playerEntity = entt::null;
        for (auto entity : view) {
            const PlayerInfo& info = view.get<PlayerInfo>(entity);
            if (info.playerId == p.playerId) {
                playerEntity = entity;
                break;
            }
        }
        if (playerEntity == entt::null) continue;

        float multiplier = 1.0f;
        if (topContributor && topContributor->playerId == p.playerId) {
            multiplier = config_.topContributorBonusMultiplier;
        }

        if (reward.xpReward > 0 && experienceSystem_) {
            uint64_t xpAmount = static_cast<uint64_t>(reward.xpReward * multiplier);
            experienceSystem_->awardXP(registry, playerEntity, xpAmount);
        }

        if (reward.goldReward > 0) {
            if (!registry.all_of<Inventory>(playerEntity)) {
                registry.emplace<Inventory>(playerEntity);
            }
            Inventory& inv = registry.get<Inventory>(playerEntity);
            inv.gold += reward.goldReward * multiplier;
        }

        if (topContributor && topContributor->playerId == p.playerId &&
            reward.bonusItemId > 0 && itemSystem_) {
            itemSystem_->addToInventory(registry, playerEntity,
                                        reward.bonusItemId, reward.bonusItemQuantity);
        }
    }
}

const EventParticipation* ZoneEventSystem::findTopContributor() const {
    const EventParticipation* top = nullptr;
    uint32_t maxDamage = 0;

    for (uint32_t i = 0; i < activeEvent_.participantCount; ++i) {
        if (activeEvent_.participants[i].totalDamage > maxDamage) {
            maxDamage = activeEvent_.participants[i].totalDamage;
            top = &activeEvent_.participants[i];
        }
    }
    return top;
}

void ZoneEventSystem::sendEventMessage(Registry& registry, const char* message) {
    if (chatSystem_) {
        chatSystem_->broadcastSystemMessage(registry, message, 0);
    }
}

} // namespace DarkAges
