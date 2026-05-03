#include <catch2/catch_test_macros.hpp>
#include "accounts/AccountSystem.hpp"
#include <cstring>

using namespace DarkAges::Accounts;

// ============================================================================
// Account System Tests - PRD-039
// ============================================================================

TEST_CASE("AccountSystem createAccount valid", "[accounts][auth]") {
    AccountSystem sys;
    
    uint32_t id = sys.createAccount("testuser", "password123", "test@example.com");
    CHECK(id > 0);
    
    Account* acc = sys.getAccount(id);
    REQUIRE(acc != nullptr);
    CHECK(strcmp(acc->username, "testuser") == 0);
}

TEST_CASE("AccountSystem createAccount short password fails", "[accounts][auth]") {
    AccountSystem sys;
    
    uint32_t id = sys.createAccount("testuser", "short", "test@example.com");
    CHECK(id == 0);
}

TEST_CASE("AccountSystem createAccount duplicate fails", "[accounts][auth]") {
    AccountSystem sys;
    
    sys.createAccount("testuser", "password123", "test1@example.com");
    uint32_t id2 = sys.createAccount("testuser", "password456", "test2@example.com");
    
    CHECK(id2 == 0);
}

TEST_CASE("AccountSystem authenticate success", "[accounts][auth]") {
    AccountSystem sys;
    
    sys.createAccount("testuser", "password123", "test@example.com");
    uint32_t id = sys.authenticate("testuser", "password123");
    
    CHECK(id > 0);
}

TEST_CASE("AccountSystem authenticate wrong password fails", "[accounts][auth]") {
    AccountSystem sys;
    
    sys.createAccount("testuser", "password123", "test@example.com");
    uint32_t id = sys.authenticate("testuser", "wrongpassword");
    
    CHECK(id == 0);
}

TEST_CASE("AccountSystem createSession", "[accounts][session]") {
    AccountSystem sys;
    
    uint32_t accountId = sys.createAccount("testuser", "password123", "test@example.com");
    std::string token = sys.createSession(accountId);
    
    CHECK(token.length() > 0);
    CHECK(sys.validateSession(token.c_str()) == accountId);
}

TEST_CASE("AccountSystem session expires", "[accounts][session]") {
    AccountSystem sys;
    
    uint32_t accountId = sys.createAccount("testuser", "password123", "test@example.com");
    std::string token = sys.createSession(accountId);
    
    // Logout should invalidate
    sys.logout(token.c_str());
    CHECK(sys.validateSession(token.c_str()) == 0);
}

TEST_CASE("AccountSystem banAccount", "[accounts][ban]") {
    AccountSystem sys;
    
    uint32_t accountId = sys.createAccount("testuser", "password123", "test@example.com");
    
    // Ban the account
    sys.banAccount(accountId, 0, "Cheating");
    
    // Auth should fail
    uint32_t id = sys.authenticate("testuser", "password123");
    CHECK(id == 0);
}

TEST_CASE("AccountSystem unbanAccount", "[accounts][ban]") {
    AccountSystem sys;
    
    uint32_t accountId = sys.createAccount("testuser", "password123", "test@example.com");
    
    // Ban then unban
    sys.banAccount(accountId, 0, "Cheating");
    sys.unbanAccount(accountId);
    
    // Auth should work
    uint32_t id = sys.authenticate("testuser", "password123");
    CHECK(id == accountId);
}

TEST_CASE("AccountSystem failed login lockout", "[accounts][auth]") {
    AccountSystem sys;
    
    sys.createAccount("testuser", "password123", "test@example.com");
    
    // Fail 5 times
    for (int i = 0; i < 5; i++) {
        sys.authenticate("testuser", "wrongpassword");
    }
    
    // Should be locked out
    CHECK(sys.isLockedOut(1) == true);
}

TEST_CASE("AccountSystem getStats", "[accounts][stats]") {
    AccountSystem sys;
    
    sys.createAccount("user1", "password123", "user1@example.com");
    sys.createAccount("user2", "password456", "user2@example.com");
    
    AccountStats stats = sys.getStats();
    CHECK(stats.totalAccounts == 2);
}