#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <vector>

// ============================================================================
// Account System — PRD-039 Integration
// Manages player accounts, authentication, and security
// ============================================================================

namespace DarkAges {
namespace Accounts {

// Account database record
struct Account {
    uint32_t id{0};
    char username[32]{0};
    char passwordHash[64]{0};      // bcrypt hash (60 chars + null)
    char email[64]{0};
    uint32_t createdAt{0};
    uint32_t lastLogin{0};
    bool banned{false};
    uint32_t banExpires{0};         // 0 = permanent, >0 = temp ban
    char banReason[128]{0};
    uint8_t failedAttempts{0};      // Login failure counter
    uint32_t lockoutUntil{0};        // Account locked until
};

// Session token
struct Session {
    uint32_t accountId{0};
    char token[64]{0};              // Random token
    uint32_t createdAt{0};
    uint32_t expiresAt{0};
    bool valid{true};
};

// Account statistics
struct AccountStats {
    uint32_t totalAccounts{0};
    uint32_t activeSessions{0};
    uint32_t bannedAccounts{0};
};

class AccountSystem {
public:
    AccountSystem() = default;
    
    // PRD-039: Create new account
    // Returns account ID on success, 0 on failure
    uint32_t createAccount(const char* username, const char* password, 
                          const char* email);
    
    // PRD-039: Authenticate account  
    // Returns account ID on success, 0 on failure
    uint32_t authenticate(const char* username, const char* password);
    
    // PRD-039: Generate session token
    // Returns session token, empty on failure
    std::string createSession(uint32_t accountId);
    
    // PRD-039: Validate session
    // Returns account ID if valid, 0 if invalid/expired
    uint32_t validateSession(const char* token);
    
    // PRD-039: Invalidate session (logout)
    void logout(const char* token);
    
    // PRD-039: Ban account
    void banAccount(uint32_t accountId, uint32_t durationDays, const char* reason);
    
    // PRD-039: Unban account
    void unbanAccount(uint32_t accountId);
    
    // PRD-039: Get account info
    Account* getAccount(uint32_t accountId);
    
    // PRD-039: Check if locked out
    bool isLockedOut(uint32_t accountId);
    
    // PRD-039: Get statistics
    AccountStats getStats() const;
    
private:
    void lockAccount(uint32_t accountId);
    void cleanupSessions();
    std::string hashPassword(const char* password);
    bool verifyPassword(const char* password, const char* hash);
    std::string generateToken();
    
    std::vector<Account> accounts_;
    std::vector<Session> sessions_;
    uint32_t nextAccountId_{1};
};

} // namespace Accounts
} // namespace DarkAges