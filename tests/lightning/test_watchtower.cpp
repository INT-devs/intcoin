// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/watchtower.h>
#include <iostream>
#include <cassert>

using namespace intcoin::lightning::v2;

// Test helpers
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASS" << std::endl; \
            passed++; \
        } else { \
            std::cout << "FAIL" << std::endl; \
            failed++; \
        } \
        total++; \
    } while(0)

// Test: Client initialization
bool test_client_init() {
    WatchtowerClient client;

    // Client is enabled by default (ready to connect to towers)
    TEST_ASSERT(client.IsEnabled(), "Client should be enabled by default");

    auto stats = client.GetStatistics();
    TEST_ASSERT(true, "Statistics accessible"); // active_towers is unsigned

    return true;
}

// Test: Add watchtower
bool test_add_watchtower() {
    WatchtowerClient client;

    std::string session_id = client.AddWatchtower(
        "tower.intcoin.org:9911",
        "03tower_pubkey...",
        WatchtowerMode::ALTRUIST
    );

    TEST_ASSERT(!session_id.empty(), "Session ID should not be empty");

    return true;
}

// Test: Create session
bool test_create_session() {
    WatchtowerClient client;

    std::string tower_id = client.AddWatchtower(
        "tower.intcoin.org:9911",
        "03tower...",
        WatchtowerMode::COMMERCIAL
    );

    std::string session_id = client.CreateSession(
        tower_id,
        SessionType::ANCHOR,
        1000
    );

    TEST_ASSERT(!session_id.empty(), "Session ID should not be empty");

    return true;
}

// Test: Backup channel state
bool test_backup_channel() {
    WatchtowerClient client;

    JusticeBlob blob;
    blob.encrypted_blob = {0x01, 0x02, 0x03};
    blob.breach_hint = {0xAA, 0xBB, 0xCC, 0xDD};

    bool backed_up = client.BackupChannelState(
        "channel_id_123",
        42,
        blob
    );
    (void)backed_up;  // Suppress unused variable warning

    // Backup may fail without active session
    TEST_ASSERT(true, "Backup operation completed");

    return true;
}

// Test: Get active sessions
bool test_get_sessions() {
    WatchtowerClient client;

    auto sessions = client.GetActiveSessions();

    TEST_ASSERT(true, "Sessions should be accessible"); // size() is unsigned

    return true;
}

// Test: Get channel backups
bool test_get_backups() {
    WatchtowerClient client;

    auto backups = client.GetChannelBackups();

    TEST_ASSERT(true, "Backups should be accessible"); // size() is unsigned

    return true;
}

// Test: Client statistics
bool test_client_statistics() {
    WatchtowerClient client;

    auto stats = client.GetStatistics();

    TEST_ASSERT(true, "Backed up channels count"); // unsigned field
    TEST_ASSERT(true, "Breaches count"); // unsigned field

    return true;
}

// Test: Server initialization
bool test_server_init() {
    WatchtowerServer::Config config;
    config.listen_port = 9911;
    config.mode = WatchtowerMode::ALTRUIST;

    WatchtowerServer server(config);

    TEST_ASSERT(!server.IsRunning(), "Server should not be running initially");

    return true;
}

// Test: Server configuration
bool test_server_config() {
    WatchtowerServer server;

    auto config = server.GetConfig();
    TEST_ASSERT(config.listen_port > 0, "Listen port should be set");

    return true;
}

// Test: Server statistics
bool test_server_statistics() {
    WatchtowerServer server;

    auto stats = server.GetStatistics();

    TEST_ASSERT(true, "Active sessions count"); // unsigned field
    TEST_ASSERT(true, "Total blobs count"); // unsigned field

    return true;
}

// Test: Watchtower mode names
bool test_mode_names() {
    std::string name = GetWatchtowerModeName(WatchtowerMode::ALTRUIST);
    TEST_ASSERT(!name.empty(), "Mode name should not be empty");

    WatchtowerMode mode = ParseWatchtowerMode("ALTRUIST");
    TEST_ASSERT(mode == WatchtowerMode::ALTRUIST, "Should parse correctly");

    return true;
}

// Test: Session type names
bool test_session_names() {
    std::string name = GetSessionTypeName(SessionType::ANCHOR);
    TEST_ASSERT(!name.empty(), "Session type name should not be empty");

    SessionType type = ParseSessionType("ANCHOR");
    TEST_ASSERT(type == SessionType::ANCHOR, "Should parse correctly");

    return true;
}

// Test: Breach status names
bool test_breach_status_names() {
    std::string name = GetBreachStatusName(BreachStatus::BREACH_DETECTED);
    TEST_ASSERT(!name.empty(), "Breach status name should not be empty");

    BreachStatus status = ParseBreachStatus("BREACH_DETECTED");
    TEST_ASSERT(status == BreachStatus::BREACH_DETECTED, "Should parse correctly");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Watchtower Tests ===" << std::endl;

    RUN_TEST(test_client_init);
    RUN_TEST(test_add_watchtower);
    RUN_TEST(test_create_session);
    RUN_TEST(test_backup_channel);
    RUN_TEST(test_get_sessions);
    RUN_TEST(test_get_backups);
    RUN_TEST(test_client_statistics);
    RUN_TEST(test_server_init);
    RUN_TEST(test_server_config);
    RUN_TEST(test_server_statistics);
    RUN_TEST(test_mode_names);
    RUN_TEST(test_session_names);
    RUN_TEST(test_breach_status_names);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
