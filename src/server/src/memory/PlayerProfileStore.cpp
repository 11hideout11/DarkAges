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
    
    // Parse JSON
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    
    PlayerProfile profile = ParseProfileJson(character_id, json);
    profile.last_login = std::chrono::system_clock::now();
    
    // Add to account index
    account_characters_[profile.account_id].push_back(character_id);
    
    cache_[character_id] = profile;
    
    std::cout << "[ProfileStore] Loaded character: " << profile.name 
              << " (id: " << character_id << ")" << std::endl;
    
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
    std::string path = GetProfilePath(profile.character_id);
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cout << "[ProfileStore] Failed to save: " << profile.character_id << std::endl;
        return;
    }
    
    // JSON serialization
    file << "{\n";
    file << "  \"character_id\": " << profile.character_id << ",\n";
    file << "  \"account_id\": " << profile.account_id << ",\n";
    file << "  \"name\": \"" << profile.name << "\",\n";
    file << "  \"class_id\": " << (int)profile.class_id << ",\n";
    file << "  \"level\": " << (int)profile.level << ",\n";
    file << "  \"experience\": " << profile.experience << ",\n";
    file << "  \"currency\": " << profile.currency << ",\n";
    file << "  \"home_zone_id\": " << profile.home_zone_id << ",\n";
    file << "  \"position\": {\" << profile.position.x << ", " 
        << profile.position.y << ", " << profile.position.z << "},\n";
    file << "  \"health\": " << profile.health << ",\n";
    file << "  \"max_health\": " << profile.max_health << ",\n";
    file << "  \"mana\": " << profile.mana << ",\n";
    file << "  \"max_mana\": " << profile.max_mana << ",\n";
    file << "  \"inventory\": " << profile.inventory.getItemCount() << ",\n";
    file << "  \"equipment\": " << profile.equipment.getFilledSlots() << "\n";
    file << "}\n";
    
    // Update cache (caller holds mutex)
    auto it = cache_.find(profile.character_id);
    if (it != cache_.end()) {
        it->second.dirty = false;
        it->second.last_save = std::chrono::system_clock::now();
    }
    
    std::cout << "[ProfileStore] Saved character: " << profile.character_id << std::endl;
}

// Internal save without mutex lock (caller must hold mutex)
void PlayerProfileStore::SaveCharacterInternal(const PlayerProfile& profile) {
    std::string path = GetProfilePath(profile.character_id);
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cout << "[ProfileStore] Failed to save: " << profile.character_id << std::endl;
        return;
    }
    
    // JSON serialization
    file << "{\n";
    file << "  \"character_id\": " << profile.character_id << ",\n";
    file << "  \"account_id\": " << profile.account_id << ",\n";
    file << "  \"name\": \"" << profile.name << "\",\n";
    file << "  \"class_id\": " << (int)profile.class_id << ",\n";
    file << "  \"level\": " << (int)profile.level << ",\n";
    file << "  \"experience\": " << profile.experience << ",\n";
    file << "  \"currency\": " << profile.currency << ",\n";
    file << "  \"home_zone_id\": " << profile.home_zone_id << ",\n";
    file << "  \"position\": {\"" << profile.position.x << "\", \"" 
        << profile.position.y << "\", \"" << profile.position.z << "\"},\n";
    file << "  \"health\": " << profile.health << ",\n";
    file << "  \"max_health\": " << profile.max_health << ",\n";
    file << "  \"mana\": " << profile.mana << ",\n";
    file << "  \"max_mana\": " << profile.max_mana << ",\n";
    file << "  \"inventory\": " << profile.inventory.getItemCount() << ",\n";
    file << "  \"equipment\": " << profile.equipment.getFilledSlots() << "\n";
    file << "}\n";
    
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
            
            // Save character using internal (mutex already held)
            auto cache_it = cache_.find(character_id);
            if (cache_it != cache_.end() && cache_it->second.dirty) {
                cache_it->second.dirty = false;
                cache_it->second.last_save = std::chrono::system_clock::now();
                SaveCharacterInternal(cache_it->second);
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
    
    // Save all dirty profiles using internal (mutex already held)
    for (auto& [id, profile] : cache_) {
        if (profile.dirty) {
            profile.dirty = false;
            profile.last_save = std::chrono::system_clock::now();
            SaveCharacterInternal(profile);
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

PlayerProfile PlayerProfileStore::ParseProfileJson(uint64_t character_id, const std::string& json) const {
    PlayerProfile profile;
    profile.character_id = character_id;
    
    // Simple JSON parsing (key: value)
    std::istringstream iss(json);
    std::string line;
    while (std::getline(iss, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\n\r");
        line = line.substr(start, end - start + 1);
        
        // Skip brackets
        if (line == "{" || line == "}") continue;
        
        // Find colon
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        
        // Trim
        key.erase(key.find_last_not_of(" \t\"") + 1);
        key.erase(0, key.find_first_not_of(" \t\""));
        value.erase(0, value.find_first_not_of(" \t\""));
        value.erase(value.find_last_not_of(" \t\",}") + 1);
        
        // Parse fields
        if (key == "account_id") profile.account_id = std::stoull(value);
        else if (key == "name") profile.name = value;
        else if (key == "class_id") profile.class_id = std::stoi(value);
        else if (key == "level") profile.level = std::stoi(value);
        else if (key == "experience") profile.experience = std::stoul(value);
        else if (key == "currency") profile.currency = std::stoul(value);
        else if (key == "home_zone_id") profile.home_zone_id = std::stoul(value);
        else if (key == "health") profile.health = std::stof(value);
        else if (key == "max_health") profile.max_health = std::stof(value);
        else if (key == "mana") profile.mana = std::stof(value);
        else if (key == "max_mana") profile.max_mana = std::stof(value);
    }
    
    return profile;
}

} // namespace DarkAges