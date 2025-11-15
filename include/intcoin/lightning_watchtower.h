// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network Watchtower implementation
// Third-party monitoring service for channel security
//
// Watchtowers monitor the blockchain for outdated channel commitment
// transactions and broadcast penalty transactions to protect users.

#ifndef INTCOIN_LIGHTNING_WATCHTOWER_H
#define INTCOIN_LIGHTNING_WATCHTOWER_H

#include "lightning.h"
#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <map>
#include <optional>
#include <chrono>
#include <mutex>

namespace intcoin {
namespace lightning {

/**
 * Watchtower protocol version
 */
static constexpr uint32_t WATCHTOWER_VERSION = 1;

/**
 * Maximum breach remedies stored per client
 */
static constexpr size_t MAX_BREACH_REMEDIES_PER_CLIENT = 10000;

/**
 * Breach remedy retention period (in seconds)
 * Default: 180 days
 */
static constexpr uint64_t BREACH_REMEDY_RETENTION = 180 * 24 * 60 * 60;

/**
 * Watchtower message types
 */
enum class WatchtowerMessageType : uint8_t {
    REGISTER_CLIENT = 0x01,      // Client registration
    BREACH_REMEDY = 0x02,        // Encrypted breach remedy data
    BREACH_DETECTED = 0x03,      // Watchtower detected a breach
    REMEDY_RESPONSE = 0x04,      // Response to breach remedy submission
    PING = 0x05,                 // Keep-alive ping
    PONG = 0x06,                 // Keep-alive pong
    ERROR = 0xFF                 // Error message
};

/**
 * Watchtower error codes
 */
enum class WatchtowerError : uint8_t {
    NONE = 0x00,
    INVALID_SIGNATURE = 0x01,
    STORAGE_FULL = 0x02,
    INVALID_REMEDY = 0x03,
    DUPLICATE_REMEDY = 0x04,
    CLIENT_NOT_REGISTERED = 0x05,
    RATE_LIMIT_EXCEEDED = 0x06,
    INTERNAL_ERROR = 0xFF
};

/**
 * Encrypted breach remedy
 * Contains encrypted data needed to create a penalty transaction
 * if a breach is detected. The encryption key is derived from the
 * commitment transaction ID, so only a breach reveals the key.
 */
struct BreachRemedy {
    // Commitment transaction locator (blinded/encrypted)
    Hash256 commitment_txid_hint;  // First 32 bytes of SHA3(commitment_txid || secret)

    // Encrypted payload containing:
    // - Penalty transaction
    // - Revocation key
    // - Witness data
    std::vector<uint8_t> encrypted_payload;

    // Encryption key derivation salt
    std::vector<uint8_t> salt;

    // Client signature (proves client owns this remedy)
    DilithiumSignature client_sig;

    // Expiry timestamp
    uint64_t expiry_timestamp;

    // Channel ID (for organization)
    Hash256 channel_id;

    std::vector<uint8_t> serialize() const;
    static BreachRemedy deserialize(const std::vector<uint8_t>& data);

    // Verify the remedy signature
    bool verify_signature(const DilithiumPubKey& client_pubkey) const;
};

/**
 * Decrypted breach remedy payload
 * Only accessible after detecting a breach commitment transaction
 */
struct BreachRemedyPayload {
    Transaction penalty_tx;              // Pre-signed penalty transaction
    DilithiumPrivKey revocation_privkey; // Revocation private key
    std::vector<uint8_t> witness_data;   // Witness/script data
    uint64_t to_local_amount;            // Amount being stolen
    uint64_t to_remote_amount;           // Amount belonging to remote party

    std::vector<uint8_t> serialize() const;
    static BreachRemedyPayload deserialize(const std::vector<uint8_t>& data);
};

/**
 * Watchtower client registration
 */
struct WatchtowerClientRegistration {
    DilithiumPubKey client_pubkey;  // Client's public key
    uint64_t timestamp;              // Registration timestamp
    DilithiumSignature signature;    // Signature over (pubkey || timestamp)
    std::string client_id;           // Optional client identifier

    std::vector<uint8_t> serialize() const;
    static WatchtowerClientRegistration deserialize(const std::vector<uint8_t>& data);
    bool verify() const;
};

/**
 * Watchtower client (user-side)
 *
 * The client is responsible for:
 * 1. Connecting to watchtower servers
 * 2. Encrypting and sending breach remedies when channel state updates
 * 3. Managing watchtower credentials
 */
class WatchtowerClient {
public:
    WatchtowerClient(const DilithiumPrivKey& client_privkey);
    ~WatchtowerClient() = default;

    // Register with a watchtower server
    bool register_with_watchtower(const std::string& watchtower_address,
                                   uint16_t watchtower_port);

    // Create and upload breach remedy for a commitment transaction
    // Called whenever a new commitment transaction is created
    bool upload_breach_remedy(const std::string& watchtower_address,
                              uint16_t watchtower_port,
                              const Hash256& channel_id,
                              const CommitmentTransaction& commitment,
                              const DilithiumPrivKey& revocation_privkey,
                              const Transaction& penalty_tx);

    // List connected watchtowers
    std::vector<std::pair<std::string, uint16_t>> get_watchtowers() const;

    // Remove a watchtower
    bool remove_watchtower(const std::string& watchtower_address,
                           uint16_t watchtower_port);

    // Get client public key
    DilithiumPubKey get_pubkey() const { return client_pubkey_; }

    // Get statistics
    size_t get_remedy_count() const { return remedy_count_; }
    size_t get_watchtower_count() const { return watchtowers_.size(); }

private:
    DilithiumPrivKey client_privkey_;
    DilithiumPubKey client_pubkey_;

    // Connected watchtowers
    std::map<std::pair<std::string, uint16_t>, uint64_t> watchtowers_;  // address:port -> last_contact

    // Remedy tracking
    size_t remedy_count_;

    // Mutex for thread safety
    mutable std::mutex mutex_;

    // Encrypt breach remedy payload
    std::vector<uint8_t> encrypt_remedy_payload(const BreachRemedyPayload& payload,
                                                 const Hash256& commitment_txid,
                                                 const std::vector<uint8_t>& salt) const;

    // Create commitment TXID hint (blinded identifier)
    Hash256 create_txid_hint(const Hash256& commitment_txid,
                             const std::vector<uint8_t>& salt) const;

    // Send message to watchtower
    bool send_watchtower_message(const std::string& address,
                                  uint16_t port,
                                  WatchtowerMessageType msg_type,
                                  const std::vector<uint8_t>& payload);
};

/**
 * Watchtower server (monitoring service)
 *
 * The server is responsible for:
 * 1. Accepting client registrations
 * 2. Storing encrypted breach remedies
 * 3. Monitoring the blockchain for breaches
 * 4. Broadcasting penalty transactions when breaches are detected
 */
class WatchtowerServer {
public:
    WatchtowerServer(uint16_t listen_port);
    ~WatchtowerServer();

    // Start the watchtower server
    bool start();

    // Stop the watchtower server
    void stop();

    // Check if server is running
    bool is_running() const { return running_; }

    // Process a new block (scan for breaches)
    void process_block(const std::vector<Transaction>& transactions,
                       uint32_t block_height);

    // Get server statistics
    struct Stats {
        size_t registered_clients;
        size_t stored_remedies;
        size_t breaches_detected;
        size_t penalties_broadcast;
        uint64_t uptime_seconds;
    };
    Stats get_stats() const;

    // Configuration
    void set_max_clients(size_t max_clients) { max_clients_ = max_clients; }
    void set_max_remedies_per_client(size_t max_remedies) {
        max_remedies_per_client_ = max_remedies;
    }

    // Get listen port
    uint16_t get_port() const { return listen_port_; }

private:
    uint16_t listen_port_;
    bool running_;

    // Client registry
    std::map<std::string, WatchtowerClientRegistration> registered_clients_;  // client_id -> registration

    // Breach remedy storage
    // commitment_txid_hint -> breach_remedy
    std::map<Hash256, BreachRemedy> breach_remedies_;

    // Index: channel_id -> list of remedy hints
    std::map<Hash256, std::vector<Hash256>> channel_remedies_index_;

    // Statistics
    size_t breaches_detected_;
    size_t penalties_broadcast_;
    std::chrono::steady_clock::time_point start_time_;

    // Configuration
    size_t max_clients_;
    size_t max_remedies_per_client_;

    // Thread safety
    mutable std::mutex mutex_;

    // Message handlers
    bool handle_client_registration(const std::vector<uint8_t>& payload,
                                     std::string& client_id);

    bool handle_breach_remedy(const std::vector<uint8_t>& payload,
                              const std::string& client_id);

    // Breach detection
    bool is_breach_transaction(const Transaction& tx,
                               BreachRemedy& matched_remedy);

    // Decrypt and broadcast penalty
    bool broadcast_penalty(const BreachRemedy& remedy,
                          const Hash256& commitment_txid);

    // Decrypt remedy payload using commitment TXID
    std::optional<BreachRemedyPayload> decrypt_remedy_payload(
        const BreachRemedy& remedy,
        const Hash256& commitment_txid) const;

    // Cleanup expired remedies
    void cleanup_expired_remedies();

    // Network handling (simplified - would use actual networking)
    void handle_incoming_message(const std::vector<uint8_t>& message);
};

/**
 * Watchtower manager
 * Coordinates multiple watchtower clients for redundancy
 */
class WatchtowerManager {
public:
    WatchtowerManager(const DilithiumPrivKey& client_privkey);
    ~WatchtowerManager() = default;

    // Add a watchtower to the pool
    bool add_watchtower(const std::string& address, uint16_t port);

    // Remove a watchtower from the pool
    bool remove_watchtower(const std::string& address, uint16_t port);

    // Upload breach remedy to all watchtowers
    // Returns number of successful uploads
    size_t upload_to_all_watchtowers(const Hash256& channel_id,
                                       const CommitmentTransaction& commitment,
                                       const DilithiumPrivKey& revocation_privkey,
                                       const Transaction& penalty_tx);

    // Get status of all watchtowers
    struct WatchtowerStatus {
        std::string address;
        uint16_t port;
        bool online;
        uint64_t last_contact;
        size_t remedies_uploaded;
    };
    std::vector<WatchtowerStatus> get_watchtower_status() const;

    // Get overall statistics
    size_t get_total_watchtowers() const;
    size_t get_online_watchtowers() const;
    size_t get_total_remedies_uploaded() const;

private:
    std::unique_ptr<WatchtowerClient> client_;

    // Track upload counts per watchtower
    std::map<std::pair<std::string, uint16_t>, size_t> remedy_counts_;

    mutable std::mutex mutex_;
};

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_WATCHTOWER_H
