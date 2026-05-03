/**
 * @file PlayerProfileStore.hpp
 * @brief Player profile persistence for DarkAges server
 * 
 * Handles character creation, loading, saving, and deletion.
 * Uses JSON for serialization.
 */

#pragma once

#include "memory/MemoryPool.hpp"
#include "ecs/CoreTypes.hpp"
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>

namespace DarkAges {

// Forward declarations
struct PlayerProfile;
struct CharacterInfo;

/**
 * @brief Persistent storage for player profiles
 * 
 * Handles character CRUD operations and auto-save.
 * Currently uses JSON file storage (expandable to Redis/Scylla).
 */
class PlayerProfileStore {
public:
    /**
     * @brief Initialize the profile store
     * @param saveDirectory Path to save directory
     */
    explicit PlayerProfileStore(const std::string& saveDirectory);
    
    ~PlayerProfileStore();

    // ============================================================================
    // Character CRUD
    // ============================================================================

    /**
     * @brief Create a new character for an account
     * @param account_id Owner account
     * @param name Character name
     * @param class_id Character class
     * @return Character ID or nullopt on failure
     */
    std::optional<uint64_t> CreateCharacter(
        uint64_t account_id,
        const std::string& name,
        uint8_t class_id
    );

    /**
     * @brief Load a character profile
     * @param character_id Character to load
     * @return Profile data or nullopt if not found
     */
    std::optional<PlayerProfile> LoadCharacter(uint64_t character_id);

    /**
     * @brief Save character profile (debounced internally)
     * @param profile Profile to save
     */
    void SaveCharacter(const PlayerProfile& profile);

    /**
     * @brief Force immediate save (bypass debounce)
     * @param profile Profile to save
     */
    void SaveCharacterImmediate(const PlayerProfile& profile);

    /**
     * @brief Get all characters for an account
     * @param account_id Account to query
     * @return List of character info (without sensitive data)
     */
    std::vector<CharacterInfo> GetCharacters(uint64_t account_id);

    /**
     * @brief Delete a character
     * @param character_id Character to delete
     * @return true if deleted
     */
    bool DeleteCharacter(uint64_t character_id);

    // ============================================================================
    // Auto-save
    // ============================================================================

    /**
     * @brief Called every tick to process auto-saves
     * @param delta_ms Time since last call
     */
    void OnTick(uint32_t delta_ms);

    /**
     * @brief Shutdown and save all pending profiles
     */
    void Shutdown();

private:
    std::string save_directory_;
    std::mutex mutex_;
    
    // In-memory cache
    std::unordered_map<uint64_t, PlayerProfile> cache_;
    std::unordered_map<uint64_t, std::vector<uint64_t>> account_characters_;
    
    // Auto-save tracking
    std::unordered_map<uint64_t, uint32_t> pending_saves_;
    static constexpr uint32_t AUTO_SAVE_DELAY_MS = 30000;
    static constexpr uint32_t MAX_PENDING_SAVES = 100;
    
    // Character ID allocation
    std::atomic<uint64_t> next_character_id_{1};
    
    // Helpers
    std::string GetProfilePath(uint64_t character_id) const;
    bool ValidateName(const std::string& name) const;
    void ApplyStarterLoadout(PlayerProfile& profile) const;
};

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Full player profile for persistence
 */
struct PlayerProfile {
    uint64_t account_id{0};
    uint64_t character_id{0};
    std::string name;
    uint8_t class_id{0};
    uint8_t level{1};
    uint64_t experience{0};
    uint64_t currency{100};
    
    Inventory inventory;
    Inventory equipment;
    
    uint32_t home_zone_id{98};
    float home_x{0};
    float home_y{0};
    float home_z{0};
    
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_login;
    std::chrono::system_clock::time_point last_save;
    
    bool dirty{false};
};

/**
 * @brief Character info for list display (no sensitive data)
 */
struct CharacterInfo {
    uint64_t character_id{0};
    std::string name;
    uint8_t class_id{0};
    uint8_t level{1};
    std::chrono::system_clock::time_point last_login;
};

} // namespace DarkAges