/**
 * @file PlayerProfileStore.cpp
 * @brief Player profile persistence implementation
 */

#include "memory/PlayerProfileStore.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace DarkAges {

// ============================================================================
// Starter Loadout Configuration
// ============================================================================

static const uint32_t STARTER_ITEMS[] = {
    1001,  // Iron Sword
    1010,  // Steel Sword (backup)
    2001,  // Iron Armor
    3001,  // Health Potion
    3002,  // Mana Potion
};

// ============================================================================
// PlayerProfileStore Implementation
// ============================================================================

PlayerProfileStore::PlayerProfileStore(const std::string& saveDirectory)
    : save_directory_(saveDirectory)
{
    // Create save directory if needed
    mkdir(save_directory_.c_str(), 0755);
    
    std::cout << "[ProfileStore] Initialized with save directory: " 
              << save_directory_ << std::endl;
}

PlayerProfileStore::~PlayerProfileStore() {
    Shutdown();
}

// ============================================================================
// Character CRUD
// ============================================================================

std::optional<uint64_t> PlayerProfileStore::CreateCharacter(
    uint64_t account_id,
    const std::string& name,
    uint8_t class_id
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate name
    if (!ValidateName(name)) {
        std::cout << "[ProfileStore] Invalid character name: " << name << std::endl;
        return std::nullopt;
    }
    
    // Check character limit (max 4 per account)
    auto it = account_characters_.find(account_id);
    if (it != account_characters_.end() && it->second.size() >= 4) {
        std::cout << "[ProfileStore] Max characters reached for account: " 
                  << account_id << std::endl;
        return std::nullopt;
    }
    
    // Create profile
    uint64_t character_id = next_character_id_.fetch_add(1);
    
    PlayerProfile profile;
    profile.account_id = account_id;
    profile.character_id = character_id;
    profile.name = name;
    profile.class_id = class_id;
    profile.level = 1;
    profile.experience = 0;
    profile.currency = 100;
    profile.home_zone_id = 98;  // Tutorial
    profile.created_at = std::chrono::system_clock::now();
    profile.last_login = std::chrono::system_clock::now();
    profile.last_save = std::chrono::system_clock::now();
    profile.dirty = true;
    
    // Apply starter loadout
    ApplyStarterLoadout(profile);
    
    // Add to account index
    account_characters_[account_id].push_back(character_id);
    
    // Cache and save
    cache_[character_id] = profile;
    
    // Immediate save for new character
    SaveCharacterImmediate(profile);
    
    std::cout << "[ProfileStore] Created character: " << name 
              << " (id: " << character_id << ")" << std::endl;
    
    return character_id;
}

std::optional<PlayerProfile> PlayerProfileStore::LoadCharacter(uint64_t character_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check cache first
    auto it = cache_.find(character_id);
    if (it != cache_.end()) {
        it->second.last_login = std::chrono::system_clock::now();
        return it->second;
    }
    
    // Load from disk
    std::string path = GetProfilePath(character_id);
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "[ProfileStore] Character not found: " << character_id << std::endl;
        return std::nullopt;
    }
    
    // Parse JSON (simplified - just load as text for now)
    PlayerProfile profile;
    profile.character_id = character_id;
    // Full JSON parsing would go here
    
    profile.last_login = std::chrono::system_clock::now();
    cache_[character_id] = profile;
    
    std::cout << "[ProfileStore] Loaded character: " << character_id << std::endl;
    
    return profile;
}

void PlayerProfileStore::SaveCharacter(const PlayerProfile& profile) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update cache
    auto it = cache_.find(profile.character_id);
    if (it != cache_.end()) {
        it->second = profile;
        it->second.dirty = true;
    } else {
        cache_[profile.character_id] = profile;
    }
    
    // Queue for auto-save
    auto& pending = pending_saves_[profile.character_id];
    if (pending == 0) {
        pending = AUTO_SAVE_DELAY_MS;
    }
}

void PlayerProfileStore::SaveCharacterImmediate(const PlayerProfile& profile) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string path = GetProfilePath(profile.character_id);
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cout << "[ProfileStore] Failed to save: " << profile.character_id << std::endl;
        return;
    }
    
    // JSON serialization would go here
    // For now, just write a marker
    file << "{\n";
    file << "  \"character_id\": " << profile.character_id << ",\n";
    file << "  \"name\": \"" << profile.name << "\",\n";
    file << "  \"level\": " << (int)profile.level << ",\n";
    file << "  \"experience\": " << profile.experience << ",\n";
    file << "  \"currency\": " << profile.currency << "\n";
    file << "}\n";
    
    // Update cache
    auto it = cache_.find(profile.character_id);
    if (it != cache_.end()) {
        it->second.dirty = false;
        it->second.last_save = std::chrono::system_clock::now();
    }
    
    std::cout << "[ProfileStore] Saved character: " << profile.character_id << std::endl;
}

std::vector<CharacterInfo> PlayerProfileStore::GetCharacters(uint64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CharacterInfo> result;
    
    auto it = account_characters_.find(account_id);
    if (it == account_characters_.end()) {
        return result;
    }
    
    for (uint64_t character_id : it->second) {
        auto profile_it = cache_.find(character_id);
        if (profile_it != cache_.end()) {
            const PlayerProfile& p = profile_it->second;
            result.push_back({
                p.character_id,
                p.name,
                p.class_id,
                p.level,
                p.last_login
            });
        }
    }
    
    return result;
}

bool PlayerProfileStore::DeleteCharacter(uint64_t character_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find and remove from cache
    auto it = cache_.find(character_id);
    if (it == cache_.end()) {
        return false;
    }
    
    uint64_t account_id = it->second.account_id;
    cache_.erase(it);
    
    // Remove from account index
    auto account_it = account_characters_.find(account_id);
    if (account_it != account_characters_.end()) {
        auto& list = account_it->second;
        list.erase(
            std::remove(list.begin(), list.end(), character_id),
            list.end()
        );
    }
    
    // Remove pending save
    pending_saves_.erase(character_id);
    
    std::cout << "[ProfileStore] Deleted character: " << character_id << std::endl;
    
    return true;
}

// ============================================================================
// Auto-save
// ============================================================================

void PlayerProfileStore::OnTick(uint32_t delta_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Process pending saves
    for (auto it = pending_saves_.begin(); it != pending_saves_.end(); ) {
        if (it->second <= delta_ms) {
            uint64_t character_id = it->first;
            
            // Save character
            auto cache_it = cache_.find(character_id);
            if (cache_it != cache_.end() && cache_it->second.dirty) {
                SaveCharacterImmediate(cache_it->second);
            }
            
            it = pending_saves_.erase(it);
        } else {
            it->second -= delta_ms;
            ++it;
        }
    }
}

void PlayerProfileStore::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Save all dirty profiles
    for (auto& [id, profile] : cache_) {
        if (profile.dirty) {
            SaveCharacterImmediate(profile);
        }
    }
    
    cache_.clear();
    pending_saves_.clear();
    
    std::cout << "[ProfileStore] Shutdown complete" << std::endl;
}

// ============================================================================
// Helpers
// ============================================================================

std::string PlayerProfileStore::GetProfilePath(uint64_t character_id) const {
    std::ostringstream ss;
    ss << save_directory_ << "/character_" << character_id << ".json";
    return ss.str();
}

bool PlayerProfileStore::ValidateName(const std::string& name) const {
    // 3-16 characters, alphanumeric only
    if (name.length() < 3 || name.length() > 16) {
        return false;
    }
    
    for (char c : name) {
        if (!std::isalnum(c)) {
            return false;
        }
    }
    
    return true;
}

void PlayerProfileStore::ApplyStarterLoadout(PlayerProfile& profile) const {
    // Give starting items based on class
    for (uint32_t item_id : STARTER_ITEMS) {
        profile.inventory.addItem(item_id, 1, 99);
    }
    
    // Starting equipment
    profile.equipment.slots[0].itemId = 1001;  // Iron Sword in hand
}

} // namespace DarkAges