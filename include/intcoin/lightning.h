// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#ifndef INTCOIN_LIGHTNING_H
#define INTCOIN_LIGHTNING_H

#include "types.h"
#include "crypto.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

namespace intcoin {

// Forward declarations
class Blockchain;
class P2PNode;

// ============================================================================
// Lightning Network Constants
// ============================================================================

namespace lightning {
    // BOLT specifications version
    constexpr uint32_t PROTOCOL_VERSION = 1;

    // Network ports (within INTcoin's 2210-2220 range)
    constexpr uint16_t DEFAULT_LIGHTNING_PORT = 2213;       // Lightning P2P port
    constexpr uint16_t DEFAULT_LIGHTNING_RPC_PORT = 2214;   // Lightning RPC port

    // Channel limits
    constexpr uint64_t MIN_CHANNEL_CAPACITY = 100000;      // 0.001 INT minimum
    constexpr uint64_t MAX_CHANNEL_CAPACITY = 1000000000;   // 10 INT maximum
    constexpr uint64_t DUST_LIMIT = 546;                    // Dust threshold
    constexpr uint32_t MAX_HTLC_COUNT = 483;                // Maximum HTLCs per commitment

    // Timelock limits
    constexpr uint32_t MIN_CLTV_EXPIRY = 144;               // ~1 day
    constexpr uint32_t MAX_CLTV_EXPIRY = 2016;              // ~2 weeks
    constexpr uint32_t CLTV_EXPIRY_DELTA = 40;              // Safety margin

    // Fee parameters
    constexpr uint64_t BASE_FEE = 1000;                     // Base fee (INTS)
    constexpr uint64_t FEE_RATE = 1;                        // Fee rate (millionths)

    // Message types (BOLT #1)
    constexpr uint16_t MSG_INIT = 16;
    constexpr uint16_t MSG_ERROR = 17;
    constexpr uint16_t MSG_PING = 18;
    constexpr uint16_t MSG_PONG = 19;
    constexpr uint16_t MSG_OPEN_CHANNEL = 32;
    constexpr uint16_t MSG_ACCEPT_CHANNEL = 33;
    constexpr uint16_t MSG_FUNDING_CREATED = 34;
    constexpr uint16_t MSG_FUNDING_SIGNED = 35;
    constexpr uint16_t MSG_FUNDING_LOCKED = 36;
    constexpr uint16_t MSG_SHUTDOWN = 38;
    constexpr uint16_t MSG_CLOSING_SIGNED = 39;
    constexpr uint16_t MSG_UPDATE_ADD_HTLC = 128;
    constexpr uint16_t MSG_UPDATE_FULFILL_HTLC = 130;
    constexpr uint16_t MSG_UPDATE_FAIL_HTLC = 131;
    constexpr uint16_t MSG_COMMITMENT_SIGNED = 132;
    constexpr uint16_t MSG_REVOKE_AND_ACK = 133;
    constexpr uint16_t MSG_UPDATE_FEE = 134;
    constexpr uint16_t MSG_CHANNEL_ANNOUNCEMENT = 256;
    constexpr uint16_t MSG_NODE_ANNOUNCEMENT = 257;
    constexpr uint16_t MSG_CHANNEL_UPDATE = 258;
}

// ============================================================================
// Channel States
// ============================================================================

enum class ChannelState {
    OPENING,            // Opening in progress
    OPEN,               // Channel is open and operational
    CLOSING_MUTUAL,     // Mutual close initiated
    CLOSING_FORCE,      // Force close initiated
    CLOSED,             // Channel is closed
    ERROR_STATE         // Error occurred
};

// ============================================================================
// BOLT #2 Channel Establishment Messages
// ============================================================================

// open_channel message (BOLT #2)
struct OpenChannelMsg {
    uint256 chain_hash;
    uint256 temporary_channel_id;
    uint64_t funding_satoshis;
    uint64_t push_msat;
    uint64_t dust_limit_satoshis;
    uint64_t max_htlc_value_in_flight_msat;
    uint64_t channel_reserve_satoshis;
    uint64_t htlc_minimum_msat;
    uint32_t feerate_per_kw;
    uint16_t to_self_delay;
    uint16_t max_accepted_htlcs;
    PublicKey funding_pubkey;
    PublicKey revocation_basepoint;
    PublicKey payment_basepoint;
    PublicKey delayed_payment_basepoint;
    PublicKey htlc_basepoint;
    PublicKey first_per_commitment_point;
    uint8_t channel_flags;

    OpenChannelMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<OpenChannelMsg> Deserialize(const std::vector<uint8_t>& data);
};

// accept_channel message (BOLT #2)
struct AcceptChannelMsg {
    uint256 temporary_channel_id;
    uint64_t dust_limit_satoshis;
    uint64_t max_htlc_value_in_flight_msat;
    uint64_t channel_reserve_satoshis;
    uint64_t htlc_minimum_msat;
    uint32_t minimum_depth;
    uint16_t to_self_delay;
    uint16_t max_accepted_htlcs;
    PublicKey funding_pubkey;
    PublicKey revocation_basepoint;
    PublicKey payment_basepoint;
    PublicKey delayed_payment_basepoint;
    PublicKey htlc_basepoint;
    PublicKey first_per_commitment_point;

    AcceptChannelMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<AcceptChannelMsg> Deserialize(const std::vector<uint8_t>& data);
};

// funding_created message (BOLT #2)
struct FundingCreatedMsg {
    uint256 temporary_channel_id;
    uint256 funding_txid;
    uint16_t funding_output_index;
    Signature signature;

    FundingCreatedMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<FundingCreatedMsg> Deserialize(const std::vector<uint8_t>& data);
};

// funding_signed message (BOLT #2)
struct FundingSignedMsg {
    uint256 channel_id;
    Signature signature;

    FundingSignedMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<FundingSignedMsg> Deserialize(const std::vector<uint8_t>& data);
};

// funding_locked message (BOLT #2)
struct FundingLockedMsg {
    uint256 channel_id;
    PublicKey next_per_commitment_point;

    FundingLockedMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<FundingLockedMsg> Deserialize(const std::vector<uint8_t>& data);
};

// shutdown message (BOLT #2) - Initiate channel close
struct ShutdownMsg {
    uint256 channel_id;
    Script scriptpubkey;  // Closing transaction output script

    ShutdownMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<ShutdownMsg> Deserialize(const std::vector<uint8_t>& data);
};

// closing_signed message (BOLT #2) - Negotiate closing transaction
struct ClosingSignedMsg {
    uint256 channel_id;
    uint64_t fee_satoshis;  // Proposed closing fee
    Signature signature;     // Signature for closing transaction

    ClosingSignedMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<ClosingSignedMsg> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// BOLT #2 HTLC Update Messages
// ============================================================================

// update_add_htlc message (BOLT #2) - Add HTLC to commitment
struct UpdateAddHTLCMsg {
    uint256 channel_id;
    uint64_t id;                        // HTLC ID
    uint64_t amount_msat;               // Amount in millisatoshis
    uint256 payment_hash;               // Hash of payment preimage
    uint32_t cltv_expiry;              // CLTV expiry block height
    std::vector<uint8_t> onion_routing_packet;  // Encrypted routing info (1366 bytes)

    UpdateAddHTLCMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<UpdateAddHTLCMsg> Deserialize(const std::vector<uint8_t>& data);
};

// update_fulfill_htlc message (BOLT #2) - Fulfill HTLC with preimage
struct UpdateFulfillHTLCMsg {
    uint256 channel_id;
    uint64_t id;                        // HTLC ID to fulfill
    uint256 payment_preimage;           // Preimage that hashes to payment_hash

    UpdateFulfillHTLCMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<UpdateFulfillHTLCMsg> Deserialize(const std::vector<uint8_t>& data);
};

// update_fail_htlc message (BOLT #2) - Fail/cancel HTLC
struct UpdateFailHTLCMsg {
    uint256 channel_id;
    uint64_t id;                        // HTLC ID to fail
    std::vector<uint8_t> reason;        // Encrypted failure reason

    UpdateFailHTLCMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<UpdateFailHTLCMsg> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// BOLT #2 Commitment Signature Exchange Messages
// ============================================================================

// commitment_signed message (BOLT #2) - Commit HTLC updates
struct CommitmentSignedMsg {
    uint256 channel_id;
    Signature signature;                        // Signature for commitment transaction
    std::vector<Signature> htlc_signatures;     // Signatures for HTLC outputs

    CommitmentSignedMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<CommitmentSignedMsg> Deserialize(const std::vector<uint8_t>& data);
};

// revoke_and_ack message (BOLT #2) - Revoke old commitment and ack new one
struct RevokeAndAckMsg {
    uint256 channel_id;
    uint256 per_commitment_secret;              // Secret for previous commitment
    PublicKey next_per_commitment_point;        // Public key for next commitment

    RevokeAndAckMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<RevokeAndAckMsg> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// BOLT #7 Routing Gossip Messages
// ============================================================================

// channel_announcement message (BOLT #7) - Announce a new public channel
struct ChannelAnnouncementMsg {
    Signature node_signature_1;         // Signature from node1
    Signature node_signature_2;         // Signature from node2
    Signature bitcoin_signature_1;      // Bitcoin key signature 1
    Signature bitcoin_signature_2;      // Bitcoin key signature 2
    std::vector<uint8_t> features;      // Channel feature flags
    uint256 chain_hash;                 // Blockchain identifier (genesis hash)
    uint64_t short_channel_id;          // Short channel ID (block:tx:output format)
    PublicKey node_id_1;                // First node public key
    PublicKey node_id_2;                // Second node public key
    PublicKey bitcoin_key_1;            // First Bitcoin key
    PublicKey bitcoin_key_2;            // Second Bitcoin key

    ChannelAnnouncementMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<ChannelAnnouncementMsg> Deserialize(const std::vector<uint8_t>& data);
};

// node_announcement message (BOLT #7) - Announce node information
struct NodeAnnouncementMsg {
    Signature signature;                // Node signature
    std::vector<uint8_t> features;      // Node feature flags
    uint32_t timestamp;                 // Announcement timestamp
    PublicKey node_id;                  // Node public key
    std::array<uint8_t, 3> rgb_color;   // Node RGB color
    std::string alias;                  // Node alias (32 bytes max)
    std::vector<uint8_t> addresses;     // Node network addresses

    NodeAnnouncementMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<NodeAnnouncementMsg> Deserialize(const std::vector<uint8_t>& data);
};

// channel_update message (BOLT #7) - Update channel parameters
struct ChannelUpdateMsg {
    Signature signature;                // Signature of the node
    uint256 chain_hash;                 // Blockchain identifier
    uint64_t short_channel_id;          // Short channel ID
    uint32_t timestamp;                 // Update timestamp
    uint8_t message_flags;              // Message flags
    uint8_t channel_flags;              // Channel flags (direction bit)
    uint16_t cltv_expiry_delta;        // CLTV expiry delta
    uint64_t htlc_minimum_msat;        // Minimum HTLC amount
    uint32_t fee_base_msat;            // Base fee in millisatoshi
    uint32_t fee_proportional_millionths;  // Proportional fee

    ChannelUpdateMsg();
    std::vector<uint8_t> Serialize() const;
    static Result<ChannelUpdateMsg> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// HTLC (Hash Time-Locked Contract)
// ============================================================================

struct HTLC {
    uint64_t id;                    // HTLC identifier
    uint64_t amount;                // Amount in INTS
    uint256 payment_hash;           // Hash of payment preimage
    uint32_t cltv_expiry;          // CLTV expiry height
    std::vector<uint8_t> onion_routing_packet;  // Encrypted routing info
    bool incoming;                  // true = incoming, false = outgoing
    bool fulfilled;                 // Payment fulfilled
    uint256 preimage;              // Payment preimage (if fulfilled)

    HTLC();
    HTLC(uint64_t id, uint64_t amt, const uint256& hash, uint32_t expiry, bool inc);

    // Serialization
    std::vector<uint8_t> Serialize() const;
    static Result<HTLC> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Channel Configuration
// ============================================================================

struct ChannelConfig {
    uint64_t dust_limit;            // Dust threshold
    uint64_t max_htlc_value;        // Maximum HTLC value
    uint64_t channel_reserve;       // Reserve amount
    uint32_t htlc_minimum;          // Minimum HTLC amount
    uint32_t to_self_delay;         // CSV delay for to_self outputs
    uint32_t max_accepted_htlcs;    // Maximum HTLCs accepted

    ChannelConfig();
    static ChannelConfig Default();
};

// ============================================================================
// Commitment Transaction
// ============================================================================

struct CommitmentTransaction {
    uint64_t commitment_number;     // Commitment sequence number
    Transaction tx;                 // The commitment transaction
    uint256 revocation_key;         // Revocation key
    uint256 local_delayed_key;      // Local delayed payment key
    uint256 remote_payment_key;     // Remote payment key
    std::vector<HTLC> htlcs;       // Active HTLCs
    uint64_t local_balance;         // Local balance
    uint64_t remote_balance;        // Remote balance
    uint64_t fee;                   // Transaction fee

    CommitmentTransaction();

    // Build commitment transaction
    static Result<CommitmentTransaction> Build(
        const uint256& funding_txid,
        uint32_t funding_vout,
        uint64_t funding_amount,
        uint64_t local_balance,
        uint64_t remote_balance,
        const std::vector<HTLC>& htlcs,
        uint64_t commitment_number,
        const ChannelConfig& config
    );

    // Verify commitment transaction
    bool Verify(const PublicKey& local_key, const PublicKey& remote_key) const;
};

// ============================================================================
// Payment Channel
// ============================================================================

class Channel {
public:
    // Channel identifiers
    uint256 channel_id;             // Unique channel identifier
    uint256 temporary_id;           // Temporary ID during opening

    // Channel participants
    PublicKey local_node_id;        // Local node public key
    PublicKey remote_node_id;       // Remote node public key

    // Channel state
    ChannelState state;             // Current state
    uint64_t capacity;              // Total channel capacity
    uint64_t local_balance;         // Local balance
    uint64_t remote_balance;        // Remote balance

    // Funding transaction
    uint256 funding_txid;           // Funding transaction ID
    uint32_t funding_vout;          // Funding output index
    uint32_t funding_confirmations; // Number of confirmations

    // Commitment transactions
    CommitmentTransaction local_commitment;   // Local commitment
    CommitmentTransaction remote_commitment;  // Remote commitment
    uint64_t commitment_number;              // Current commitment number

    // HTLCs
    std::vector<HTLC> pending_htlcs;         // Pending HTLCs
    uint64_t next_htlc_id;                   // Next HTLC ID

    // Configuration
    ChannelConfig local_config;     // Local channel configuration
    ChannelConfig remote_config;    // Remote channel configuration

    // Timing
    std::chrono::system_clock::time_point opened_at;
    std::chrono::system_clock::time_point last_update;

    Channel();
    Channel(const PublicKey& local, const PublicKey& remote, uint64_t cap);

    // Channel operations
    Result<void> Open(const Transaction& funding_tx, uint32_t vout);
    Result<void> Close(bool force = false);
    Result<uint64_t> AddHTLC(uint64_t amount, const uint256& payment_hash, uint32_t expiry);
    Result<void> FulfillHTLC(uint64_t htlc_id, const uint256& preimage);
    Result<void> FailHTLC(uint64_t htlc_id);
    Result<void> UpdateCommitment();

    // Balance management
    uint64_t GetLocalBalance() const;
    uint64_t GetRemoteBalance() const;
    uint64_t GetAvailableBalance() const;
    bool CanSend(uint64_t amount) const;
    bool CanReceive(uint64_t amount) const;

    // Serialization
    std::vector<uint8_t> Serialize() const;
    static Result<Channel> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Payment Route
// ============================================================================

struct RouteHop {
    PublicKey node_id;              // Node public key
    uint256 channel_id;             // Channel to use
    uint64_t amount;                // Amount to forward
    uint32_t cltv_expiry;          // CLTV expiry
    uint64_t fee;                   // Fee for this hop

    RouteHop();
    RouteHop(const PublicKey& node, const uint256& chan, uint64_t amt, uint32_t expiry);
};

struct PaymentRoute {
    std::vector<RouteHop> hops;     // Route hops
    uint64_t total_amount;          // Total amount (including fees)
    uint64_t total_fees;            // Total fees
    uint32_t total_cltv;           // Total CLTV delay

    PaymentRoute();

    // Route validation
    bool IsValid() const;
    uint64_t CalculateTotalFees() const;
};

// ============================================================================
// Lightning Invoice (BOLT #11)
// ============================================================================

class Invoice {
public:
    uint256 payment_hash;           // Payment hash
    uint64_t amount;                // Amount in INTS
    std::string description;        // Payment description
    uint32_t expiry;                // Expiry time (seconds)
    uint32_t min_final_cltv;       // Minimum final CLTV
    PublicKey payee;                // Payee node ID
    std::vector<RouteHop> route_hints;  // Routing hints
    std::chrono::system_clock::time_point created_at;
    Signature signature;            // Invoice signature

    Invoice();
    Invoice(uint64_t amt, const std::string& desc, const PublicKey& payee_key);

    // Generate payment hash from preimage
    static uint256 GeneratePaymentHash(const uint256& preimage);

    // Encode/decode (BOLT #11 format)
    std::string Encode() const;
    static Result<Invoice> Decode(const std::string& bolt11);

    // Sign and verify
    Result<void> Sign(const SecretKey& key);
    bool Verify() const;

    // Check if expired
    bool IsExpired() const;
};

// ============================================================================
// Network Graph (for routing)
// ============================================================================

struct ChannelInfo {
    uint256 channel_id;
    PublicKey node1;
    PublicKey node2;
    uint64_t capacity;
    uint64_t base_fee;
    uint64_t fee_rate;
    uint32_t cltv_expiry_delta;
    bool enabled;
    std::chrono::system_clock::time_point last_update;

    ChannelInfo();
};

struct NodeInfo {
    PublicKey node_id;
    std::string alias;
    std::vector<uint256> channels;
    std::chrono::system_clock::time_point last_update;

    NodeInfo();
};

class NetworkGraph {
public:
    NetworkGraph();

    // Graph management
    void AddChannel(const ChannelInfo& channel);
    void RemoveChannel(const uint256& channel_id);
    void UpdateChannel(const uint256& channel_id, const ChannelInfo& info);
    void AddNode(const NodeInfo& node);
    void RemoveNode(const PublicKey& node_id);

    // Queries
    Result<ChannelInfo> GetChannel(const uint256& channel_id) const;
    Result<NodeInfo> GetNode(const PublicKey& node_id) const;
    std::vector<ChannelInfo> GetNodeChannels(const PublicKey& node_id) const;

    // Pathfinding (Dijkstra's algorithm)
    Result<PaymentRoute> FindRoute(
        const PublicKey& source,
        const PublicKey& dest,
        uint64_t amount,
        uint32_t max_hops = 20
    ) const;

    // Serialization
    std::vector<uint8_t> Serialize() const;
    static Result<std::unique_ptr<NetworkGraph>> Deserialize(const std::vector<uint8_t>& data);

private:
    std::map<uint256, ChannelInfo> channels_;
    std::map<PublicKey, NodeInfo> nodes_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Onion Routing (BOLT #4)
// ============================================================================

struct OnionPacket {
    uint8_t version;                // Packet version
    std::vector<uint8_t> public_key; // Ephemeral public key
    std::vector<uint8_t> hops_data;  // Encrypted hop data
    std::vector<uint8_t> hmac;       // HMAC

    OnionPacket();

    // Create onion packet
    static Result<OnionPacket> Create(
        const std::vector<RouteHop>& route,
        const uint256& payment_hash,
        const std::vector<uint8_t>& session_key
    );

    // Peel one layer
    Result<std::pair<RouteHop, OnionPacket>> Peel(const SecretKey& node_key) const;

    // Serialization
    std::vector<uint8_t> Serialize() const;
    static Result<OnionPacket> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Watchtower (BOLT #13 - for monitoring force-close and breaches)
// ============================================================================

// Encrypted blob containing justice transaction data
struct EncryptedBlob {
    std::vector<uint8_t> encrypted_data;    // AES-256-GCM encrypted justice tx
    std::vector<uint8_t> hint;              // Hint for matching (first 16 bytes of commitment txid)
    std::vector<uint8_t> nonce;             // 12-byte nonce for AES-256-GCM
    std::vector<uint8_t> auth_tag;          // 16-byte authentication tag for AES-256-GCM
    uint32_t sequence_number;               // Commitment transaction sequence

    EncryptedBlob();

    // Encrypt justice transaction data
    static Result<EncryptedBlob> Encrypt(
        const std::vector<uint8_t>& justice_tx_data,
        const uint256& encryption_key,
        const uint256& commitment_txid,
        uint32_t sequence
    );

    // Decrypt to recover justice transaction
    Result<std::vector<uint8_t>> Decrypt(const uint256& encryption_key) const;
};

// Breach retribution data - what to do if peer broadcasts revoked commitment
struct BreachRetribution {
    uint256 channel_id;
    uint256 revoked_commitment_txid;        // Transaction ID of revoked commitment
    uint256 commitment_secret;              // Per-commitment secret for this state
    uint64_t revoked_local_balance;         // Local balance in revoked state
    uint64_t revoked_remote_balance;        // Remote balance in revoked state
    std::vector<HTLC> revoked_htlcs;        // HTLCs in revoked state
    Transaction justice_tx;                 // Pre-built penalty transaction
    uint32_t to_self_delay;                 // CSV delay for to_self output
    PublicKey revocation_pubkey;            // Public key for claiming revoked output

    BreachRetribution();
};

// Watchtower task - tracks a specific channel state to watch for
struct WatchtowerTask {
    uint256 channel_id;
    uint256 revoked_commitment_txid;
    EncryptedBlob encrypted_justice;        // Encrypted justice transaction
    uint32_t watch_until_height;
    std::chrono::system_clock::time_point created_at;
    bool is_active;

    WatchtowerTask();
};

class Watchtower {
public:
    Watchtower(Blockchain* blockchain);
    ~Watchtower();

    // Start/stop watchtower monitoring
    Result<void> Start();
    void Stop();
    bool IsRunning() const;

    // Client interface - upload encrypted justice transaction
    Result<void> UploadBlob(
        const uint256& channel_id,
        const EncryptedBlob& blob,
        uint32_t watch_until_height
    );

    // Add breach retribution data for a channel
    void WatchChannel(const uint256& channel_id, const BreachRetribution& retribution);

    // Remove channel from watch
    void UnwatchChannel(const uint256& channel_id);

    // Check all monitored channels for breaches
    void CheckForBreaches();

    // Detect if a specific transaction is a revoked commitment
    Result<BreachRetribution> DetectBreach(const Transaction& tx) const;

    // Build justice (penalty) transaction for a breach
    static Result<Transaction> BuildJusticeTransaction(
        const BreachRetribution& retribution,
        const Transaction& breach_tx,
        const PublicKey& destination
    );

    // Broadcast penalty transaction
    Result<void> BroadcastPenalty(const uint256& channel_id);

    // Get watchtower statistics
    struct Stats {
        uint64_t channels_watched;
        uint64_t breaches_detected;
        uint64_t penalties_broadcast;
        uint64_t blobs_stored;
    };
    Stats GetStatistics() const;

private:
    Blockchain* blockchain_;

    // Channel breach monitoring
    std::map<uint256, BreachRetribution> breach_data_;          // channel_id -> retribution data
    std::map<uint256, std::vector<WatchtowerTask>> tasks_;      // channel_id -> watch tasks
    std::map<uint256, EncryptedBlob> encrypted_blobs_;          // hint -> encrypted blob

    // Statistics
    Stats stats_;

    // Monitoring thread
    std::atomic<bool> running_;
    std::thread monitor_thread_;

    // Thread safety
    mutable std::mutex mutex_;

    // Internal helpers
    void MonitoringLoop();
    bool IsRevokedCommitment(const uint256& txid) const;
};

// ============================================================================
// Lightning Network Manager
// ============================================================================

class LightningNetwork {
public:
    LightningNetwork(Blockchain* blockchain, P2PNode* p2p);
    ~LightningNetwork();

    // Initialization
    Result<void> Start(const PublicKey& node_id, const SecretKey& node_key);
    void Stop();
    bool IsRunning() const;

    // Node management
    PublicKey GetNodeId() const;
    std::string GetNodeAlias() const;
    void SetNodeAlias(const std::string& alias);

    // Channel management
    Result<uint256> OpenChannel(
        const PublicKey& remote_node,
        uint64_t capacity,
        uint64_t push_amount = 0
    );
    Result<void> CloseChannel(const uint256& channel_id, bool force = false);
    std::vector<Channel> ListChannels() const;
    Result<Channel> GetChannel(const uint256& channel_id) const;

    // Payments
    Result<uint256> SendPayment(const std::string& bolt11_invoice);
    Result<uint256> SendPayment(
        const PublicKey& dest,
        uint64_t amount,
        const std::string& description
    );
    Result<Invoice> CreateInvoice(uint64_t amount, const std::string& description);

    // Routing
    Result<PaymentRoute> FindRoute(
        const PublicKey& dest,
        uint64_t amount
    ) const;

    // Network graph
    NetworkGraph& GetNetworkGraph();
    const NetworkGraph& GetNetworkGraph() const;

    // Statistics
    struct Stats {
        size_t num_channels;
        size_t num_active_channels;
        uint64_t total_capacity;
        uint64_t local_balance;
        uint64_t remote_balance;
        size_t num_pending_htlcs;
        size_t num_payments_sent;
        size_t num_payments_received;
        uint64_t total_fees_earned;
        uint64_t total_fees_paid;
    };
    Stats GetStats() const;

    // Message handling
    void HandleMessage(const PublicKey& peer, uint16_t type, const std::vector<uint8_t>& data);

private:
    // Internal state
    Blockchain* blockchain_;
    P2PNode* p2p_;
    PublicKey node_id_;
    SecretKey node_key_;
    std::string node_alias_;
    bool running_;

    // Channels
    std::map<uint256, std::shared_ptr<Channel>> channels_;

    // Network graph
    NetworkGraph network_graph_;

    // Watchtower
    std::unique_ptr<Watchtower> watchtower_;

    // Pending payments
    struct PendingPayment {
        uint256 payment_hash;
        uint256 preimage;
        PublicKey destination;
        uint64_t amount;
        uint64_t total_amount;          // Amount + fees
        uint64_t total_fees;
        PaymentRoute route;
        std::chrono::system_clock::time_point created_at;
        std::string status;             // "pending", "succeeded", "failed"
    };
    std::map<uint256, PendingPayment> pending_payments_;

    // Statistics
    mutable Stats stats_;

    // Thread safety
    mutable std::mutex mutex_;

    // Message handlers
    void HandleOpenChannel(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleAcceptChannel(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleFundingCreated(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleFundingSigned(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleFundingLocked(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleShutdown(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleClosingSigned(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleUpdateAddHTLC(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleUpdateFulfillHTLC(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleUpdateFailHTLC(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleCommitmentSigned(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleRevokeAndAck(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleChannelAnnouncement(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleNodeAnnouncement(const PublicKey& peer, const std::vector<uint8_t>& data);
    void HandleChannelUpdate(const PublicKey& peer, const std::vector<uint8_t>& data);

    // Internal helpers
    Result<std::shared_ptr<Channel>> FindChannelByPeer(const PublicKey& peer);
    Result<void> SendMessage(const PublicKey& peer, uint16_t type, const std::vector<uint8_t>& data);
    void UpdateStats();
};

} // namespace intcoin

#endif // INTCOIN_LIGHTNING_H
