// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_LIGHTNING_V2_WATCHTOWER_H
#define INTCOIN_LIGHTNING_V2_WATCHTOWER_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace lightning {
namespace v2 {

/**
 * Watchtower mode
 */
enum class WatchtowerMode {
    ALTRUIST,                        // Free protection (limited storage)
    COMMERCIAL                       // Paid protection (guaranteed storage)
};

/**
 * Session type
 */
enum class SessionType {
    LEGACY,                          // BOLT 13 legacy sessions
    ANCHOR,                          // Anchor outputs support
    TAPROOT                          // Taproot channels support
};

/**
 * Breach status
 */
enum class BreachStatus {
    MONITORING,                      // Actively monitoring
    BREACH_DETECTED,                 // Breach detected
    PENALTY_BROADCAST,               // Penalty transaction broadcast
    PENALTY_CONFIRMED,               // Penalty confirmed on-chain
    EXPIRED                          // Session expired
};

/**
 * Encrypted justice transaction blob
 */
struct JusticeBlob {
    std::vector<uint8_t> encrypted_blob; // Encrypted penalty transaction
    std::vector<uint8_t> breach_hint;    // Hint for breach detection (16 bytes)
    uint32_t blob_version{1};
};

/**
 * Watchtower session
 */
struct WatchtowerSession {
    std::string session_id;
    std::string tower_pubkey;
    SessionType session_type{SessionType::ANCHOR};
    WatchtowerMode mode{WatchtowerMode::ALTRUIST};
    uint32_t max_updates{1000};
    uint64_t reward_base{0};             // Base reward (satoshis)
    uint32_t reward_rate{0};             // Reward rate (ppm)
    uint32_t sweep_fee_rate{10};         // Fee rate for sweep (sat/vbyte)
    uint64_t created_at{0};
    uint64_t expires_at{0};
    bool active{false};
};

/**
 * Channel backup entry
 */
struct ChannelBackup {
    std::string channel_id;
    std::string tower_id;
    uint32_t commitment_number{0};
    JusticeBlob justice_blob;
    uint64_t backed_up_at{0};
};

/**
 * Breach event
 */
struct BreachEvent {
    std::string channel_id;
    std::string breach_txid;
    uint32_t commitment_number{0};
    BreachStatus status{BreachStatus::BREACH_DETECTED};
    std::string penalty_txid;
    uint64_t penalty_amount{0};
    uint64_t detected_at{0};
    uint64_t resolved_at{0};
    uint32_t block_height{0};
    std::string error_message;           // Error message if breach handling failed
};

/**
 * Watchtower Client
 *
 * Manages connections to multiple watchtowers and backs up
 * channel state for breach protection.
 */
class WatchtowerClient {
public:
    WatchtowerClient();
    ~WatchtowerClient();

    /**
     * Add watchtower
     *
     * @param tower_address Watchtower address (host:port)
     * @param tower_pubkey Watchtower public key
     * @param mode Watchtower mode
     * @return Session ID
     */
    std::string AddWatchtower(
        const std::string& tower_address,
        const std::string& tower_pubkey,
        WatchtowerMode mode = WatchtowerMode::ALTRUIST
    );

    /**
     * Remove watchtower
     *
     * @param session_id Session ID
     * @return True if removed successfully
     */
    bool RemoveWatchtower(const std::string& session_id);

    /**
     * Create session with watchtower
     *
     * @param tower_id Watchtower ID
     * @param session_type Session type
     * @param max_updates Maximum updates allowed
     * @return Session ID
     */
    std::string CreateSession(
        const std::string& tower_id,
        SessionType session_type = SessionType::ANCHOR,
        uint32_t max_updates = 1000
    );

    /**
     * Backup channel state
     *
     * @param channel_id Channel ID
     * @param commitment_number Commitment transaction number
     * @param justice_tx Justice transaction (encrypted)
     * @return True if backup successful
     */
    bool BackupChannelState(
        const std::string& channel_id,
        uint32_t commitment_number,
        const JusticeBlob& justice_blob
    );

    /**
     * Get active sessions
     */
    std::vector<WatchtowerSession> GetActiveSessions() const;

    /**
     * Get session
     *
     * @param session_id Session ID
     * @return Watchtower session
     */
    WatchtowerSession GetSession(const std::string& session_id) const;

    /**
     * Get channel backups
     *
     * @param channel_id Channel ID (empty = all channels)
     * @return Vector of channel backups
     */
    std::vector<ChannelBackup> GetChannelBackups(
        const std::string& channel_id = ""
    ) const;

    /**
     * Get breach events
     */
    std::vector<BreachEvent> GetBreachEvents() const;

    /**
     * Get statistics
     */
    struct Statistics {
        uint32_t active_towers{0};
        uint32_t active_sessions{0};
        uint32_t backed_up_channels{0};
        uint32_t total_backups{0};
        uint32_t breaches_detected{0};
        uint32_t penalties_broadcast{0};
        uint64_t total_penalty_amount{0};
    };

    Statistics GetStatistics() const;

    /**
     * Enable/disable watchtower client
     *
     * @param enabled Enable flag
     */
    void SetEnabled(bool enabled);

    /**
     * Check if watchtower client is enabled
     */
    bool IsEnabled() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Watchtower Server
 *
 * Monitors blockchain for channel breaches and broadcasts
 * justice transactions on behalf of clients.
 */
class WatchtowerServer {
public:
    struct Config {
        uint16_t listen_port{9911};      // Default watchtower port
        WatchtowerMode mode{WatchtowerMode::ALTRUIST};
        uint32_t max_sessions{1000};
        uint32_t max_updates_per_session{1000};
        uint64_t reward_base{0};         // Base reward for commercial mode
        uint32_t reward_rate{100};       // Reward rate (ppm)
        uint32_t sweep_fee_rate{10};     // Default sweep fee rate
    };

    WatchtowerServer();
    explicit WatchtowerServer(const Config& config);
    ~WatchtowerServer();

    /**
     * Start watchtower server
     *
     * @return True if started successfully
     */
    bool Start();

    /**
     * Stop watchtower server
     */
    void Stop();

    /**
     * Check if server is running
     */
    bool IsRunning() const;

    /**
     * Process new block
     *
     * Check for channel breaches in new block
     *
     * @param block_height Block height
     * @return Number of breaches detected
     */
    uint32_t ProcessBlock(uint32_t block_height);

    /**
     * Create client session
     *
     * @param client_pubkey Client public key
     * @param session_type Session type
     * @param max_updates Maximum updates
     * @return Session ID
     */
    std::string CreateClientSession(
        const std::string& client_pubkey,
        SessionType session_type,
        uint32_t max_updates
    );

    /**
     * Store encrypted justice blob
     *
     * @param session_id Session ID
     * @param blob Justice blob
     * @return True if stored successfully
     */
    bool StoreJusticeBlob(
        const std::string& session_id,
        const JusticeBlob& blob
    );

    /**
     * Get active sessions
     */
    std::vector<WatchtowerSession> GetActiveSessions() const;

    /**
     * Get breach events
     */
    std::vector<BreachEvent> GetBreachEvents() const;

    /**
     * Get configuration
     */
    Config GetConfig() const;

    /**
     * Set configuration
     *
     * @param config Server configuration
     */
    void SetConfig(const Config& config);

    /**
     * Get statistics
     */
    struct Statistics {
        uint32_t active_sessions{0};
        uint32_t total_blobs_stored{0};
        uint32_t breaches_detected{0};
        uint32_t penalties_broadcast{0};
        uint64_t total_rewards_earned{0};
        uint64_t blocks_monitored{0};
    };

    Statistics GetStatistics() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get watchtower mode name
 */
std::string GetWatchtowerModeName(WatchtowerMode mode);

/**
 * Parse watchtower mode from string
 */
WatchtowerMode ParseWatchtowerMode(const std::string& name);

/**
 * Get session type name
 */
std::string GetSessionTypeName(SessionType type);

/**
 * Parse session type from string
 */
SessionType ParseSessionType(const std::string& name);

/**
 * Get breach status name
 */
std::string GetBreachStatusName(BreachStatus status);

/**
 * Parse breach status from string
 */
BreachStatus ParseBreachStatus(const std::string& name);

} // namespace v2
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_V2_WATCHTOWER_H
