#include "accounts/AccountSystem.hpp"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <random>

namespace DarkAges {
namespace Accounts {

uint32_t AccountSystem::createAccount(const char* username, const char* password, 
                                    const char* email) {
    // Validate inputs
    if (!username || !password || !email) return 0;
    if (strlen(username) < 3 || strlen(username) > 16) return 0;
    if (strlen(password) < 8) return 0;
    
    // Check duplicate username
    for (const auto& acc : accounts_) {
        if (strcmp(acc.username, username) == 0) return 0;
    }
    
    // Create account
    Account acc;
    acc.id = nextAccountId_++;
    strncpy(acc.username, username, sizeof(acc.username) - 1);
    strncpy(acc.passwordHash, hashPassword(password).c_str(), sizeof(acc.passwordHash) - 1);
    strncpy(acc.email, email, sizeof(acc.email) - 1);
    acc.createdAt = static_cast<uint32_t>(time(nullptr));
    
    accounts_.push_back(acc);
    return acc.id;
}

uint32_t AccountSystem::authenticate(const char* username, const char* password) {
    if (!username || !password) return 0;
    
    for (auto& acc : accounts_) {
        if (strcmp(acc.username, username) == 0) {
            // Check if locked out
            if (isLockedOut(acc.id)) return 0;
            
            // Check if banned
            if (acc.banned) {
                uint32_t now = static_cast<uint32_t>(time(nullptr));
                if (acc.banExpires == 0 || acc.banExpires > now) {
                    return 0; // Banned
                }
                // Ban expired, unban
                acc.banned = false;
                acc.failedAttempts = 0;
            }
            
            // Verify password
            if (verifyPassword(password, acc.passwordHash)) {
                acc.failedAttempts = 0;
                acc.lastLogin = static_cast<uint32_t>(time(nullptr));
                return acc.id;
            }
            
            // Wrong password
            acc.failedAttempts++;
            if (acc.failedAttempts >= 5) {
                lockAccount(acc.id);
            }
            return 0;
        }
    }
    return 0;
}

std::string AccountSystem::createSession(uint32_t accountId) {
    if (accountId == 0) return "";
    
    // Find account
    Account* acc = getAccount(accountId);
    if (!acc || acc->banned) return "";
    
    // Create session
    Session sess;
    sess.accountId = accountId;
    strncpy(sess.token, generateToken().c_str(), sizeof(sess.token) - 1);
    sess.createdAt = static_cast<uint32_t>(time(nullptr));
    sess.expiresAt = sess.createdAt + (7 * 24 * 60 * 60); // 7 days
    sess.valid = true;
    
    sessions_.push_back(sess);
    return sess.token;
}

uint32_t AccountSystem::validateSession(const char* token) {
    if (!token) return 0;
    
    uint32_t now = static_cast<uint32_t>(time(nullptr));
    
    for (auto& sess : sessions_) {
        if (sess.valid && strcmp(sess.token, token) == 0) {
            if (sess.expiresAt > now) {
                return sess.accountId;
            }
            // Expired
            sess.valid = false;
        }
    }
    return 0;
}

void AccountSystem::logout(const char* token) {
    if (!token) return;
    
    for (auto& sess : sessions_) {
        if (sess.valid && strcmp(sess.token, token) == 0) {
            sess.valid = false;
            return;
        }
    }
}

void AccountSystem::banAccount(uint32_t accountId, uint32_t durationDays, const char* reason) {
    Account* acc = getAccount(accountId);
    if (!acc) return;
    
    acc->banned = true;
    uint32_t now = static_cast<uint32_t>(time(nullptr));
    
    if (durationDays == 0) {
        acc->banExpires = 0; // Permanent
    } else {
        acc->banExpires = now + (durationDays * 24 * 60 * 60);
    }
    
    if (reason) {
        strncpy(acc.banReason, reason, sizeof(acc.banReason) - 1);
    }
    
    // Invalidate all sessions
    for (auto& sess : sessions_) {
        if (sess.accountId == accountId) {
            sess.valid = false;
        }
    }
}

void AccountSystem::unbanAccount(uint32_t accountId) {
    Account* acc = getAccount(accountId);
    if (!acc) return;
    
    acc->banned = false;
    acc->banExpires = 0;
    acc->banReason[0] = '\0';
}

Account* AccountSystem::getAccount(uint32_t accountId) {
    for (auto& acc : accounts_) {
        if (acc.id == accountId) return &acc;
    }
    return nullptr;
}

bool AccountSystem::isLockedOut(uint32_t accountId) {
    Account* acc = getAccount(accountId);
    if (!acc) return false;
    
    uint32_t now = static_cast<uint32_t>(time(nullptr));
    if (acc->lockoutUntil > now) {
        return true;
    }
    return false;
}

AccountStats AccountSystem::getStats() const {
    AccountStats stats;
    stats.totalAccounts = static_cast<uint32_t>(accounts_.size());
    
    uint32_t now = static_cast<uint32_t>(time(nullptr));
    for (const auto& sess : sessions_) {
        if (sess.valid && sess.expiresAt > now) {
            stats.activeSessions++;
        }
    }
    
    for (const auto& acc : accounts_) {
        if (acc.banned) stats.bannedAccounts++;
    }
    
    return stats;
}

void AccountSystem::lockAccount(uint32_t accountId) {
    Account* acc = getAccount(accountId);
    if (!acc) return;
    
    uint32_t now = static_cast<uint32_t>(time(nullptr));
    acc->lockoutUntil = now + (15 * 60); // 15 minutes
}

void AccountSystem::cleanupSessions() {
    uint32_t now = static_cast<uint32_t>(time(nullptr));
    
    sessions_.erase(
        std::remove_if(sessions_.begin(), sessions_.end(),
            [now](const Session& sess) {
                return !sess.valid || sess.expiresAt <= now;
            }),
        sessions_.end());
}

std::string AccountSystem::generateToken() {
    static const char charset[] = 
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    
    std::string token;
    for (int i = 0; i < 32; i++) {
        token += charset[dist(gen)];
    }
    return token;
}

// Simple hash for demo - in production use bcrypt
std::string AccountSystem::hashPassword(const char* password) {
    // Simple hash for demo - NOT for production
    std::string hash;
    int val = 12345;
    while (*password) {
        val = val * 31 + *password++;
    }
    hash = "DEMO:" + std::to_string(val);
    return hash;
}

bool AccountSystem::verifyPassword(const char* password, const char* hash) {
    std::string computed = hashPassword(password);
    return computed == hash;
}

} // namespace Accounts
} // namespace DarkAges