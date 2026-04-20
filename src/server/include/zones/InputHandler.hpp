#pragma once

#include "ecs/CoreTypes.hpp"
#include "netcode/NetworkManager.hpp"
#include "security/AntiCheat.hpp"
#include <unordered_map>
#include <cstdint>

namespace DarkAges {

class ZoneServer;
class PlayerManager;
class MovementSystem;
class CombatEventHandler;
class AbilitySystem;
class ItemSystem;
class ChatSystem;

// Handles client input validation, anti-cheat enforcement, and attack processing.
// Extracted from ZoneServer to reduce monolithic file size.
class InputHandler {
public:
    explicit InputHandler(ZoneServer& server);
    ~InputHandler() = default;

    // Set subsystem references (called during ZoneServer::initialize)
    void setPlayerManager(PlayerManager* pm) { playerManager_ = pm; }
    void setAntiCheat(Security::AntiCheatSystem* ac) { antiCheat_ = ac; }
    void setMovementSystem(MovementSystem* ms) { movementSystem_ = ms; }
    void setNetwork(NetworkManager* network) { network_ = network; }
    void setCombatEventHandler(CombatEventHandler* handler) { combatEventHandler_ = handler; }
    void setAbilitySystem(AbilitySystem* as) { abilitySystem_ = as; }
    void setItemSystem(ItemSystem* is) { itemSystem_ = is; }
    void setChatSystem(ChatSystem* cs) { chatSystem_ = cs; }

    // Process incoming client input (routing entry point)
    void onClientInput(const ClientInputPacket& input);

    // Validate and apply client input with anti-cheat enforcement
    void validateAndApplyInput(EntityID entity, const ClientInputPacket& input);

    // Process attack input with lag compensation
    void processAttackInput(EntityID entity, const ClientInputPacket& input);

    // Process ability cast from player loadout
    void processAbilityInput(EntityID entity, const ClientInputPacket& input);

    // Process item use from inventory slot
    void processItemUseInput(EntityID entity, const ClientInputPacket& input);

    // Process chat message from player
    // chatContent is the message text (passed separately from InputState)
    void processChatInput(EntityID entity, ChatChannel channel, const char* chatContent,
                          uint32_t currentTimeMs, uint32_t targetId = 0);

private:
    ZoneServer& server_;
    PlayerManager* playerManager_{nullptr};
    Security::AntiCheatSystem* antiCheat_{nullptr};
    MovementSystem* movementSystem_{nullptr};
    NetworkManager* network_{nullptr};
    CombatEventHandler* combatEventHandler_{nullptr};
    AbilitySystem* abilitySystem_{nullptr};
    ItemSystem* itemSystem_{nullptr};
    ChatSystem* chatSystem_{nullptr};
};

} // namespace DarkAges
