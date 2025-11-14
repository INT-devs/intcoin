// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network integration layer for P2P networking.
// Handles Lightning message routing and peer management.

#ifndef INTCOIN_LIGHTNING_NETWORK_H
#define INTCOIN_LIGHTNING_NETWORK_H

#include "lightning.h"
#include "p2p.h"
#include "crypto.h"
#include <memory>
#include <map>
#include <optional>
#include <functional>

namespace intcoin {
namespace lightning {

/**
 * Lightning P2P message types (separate from base layer)
 */
enum class LightningMessageType : uint32_t {
    LIGHTNING_BASE = 10000,  // Base offset to avoid conflicts

    // Lightning handshake
    INIT = 16,
    ERROR_MSG = 17,
    PING_LIGHTNING = 18,
    PONG_LIGHTNING = 19,

    // Channel establishment
    OPEN_CHANNEL = 32,
    ACCEPT_CHANNEL = 33,
    FUNDING_CREATED = 34,
    FUNDING_SIGNED = 35,
    FUNDING_LOCKED = 36,

    // Channel operation
    UPDATE_ADD_HTLC = 128,
    UPDATE_FULFILL_HTLC = 130,
    UPDATE_FAIL_HTLC = 131,
    COMMITMENT_SIGNED = 132,
    REVOKE_AND_ACK = 133,
    UPDATE_FEE = 134,

    // Channel closing
    SHUTDOWN = 38,
    CLOSING_SIGNED = 39,

    // Gossip (network discovery)
    CHANNEL_ANNOUNCEMENT = 256,
    NODE_ANNOUNCEMENT = 257,
    CHANNEL_UPDATE = 258,
    QUERY_SHORT_CHANNEL_IDS = 261,
    REPLY_SHORT_CHANNEL_IDS_END = 262
};

/**
 * Lightning peer information
 */
struct LightningPeer {
    DilithiumPubKey node_id;
    p2p::PeerAddress address;
    bool connected;
    bool features_announced;
    uint64_t last_seen;
    std::vector<Hash256> channels;  // Channels with this peer

    // Feature flags
    bool supports_data_loss_protect;
    bool supports_initial_routing_sync;
    bool supports_upfront_shutdown_script;
    bool supports_gossip_queries;
    bool supports_var_onion_optin;
    bool supports_static_remote_key;
    bool supports_payment_secret;
    bool supports_basic_mpp;

    LightningPeer() : connected(false), features_announced(false), last_seen(0),
                      supports_data_loss_protect(false), supports_initial_routing_sync(false),
                      supports_upfront_shutdown_script(false), supports_gossip_queries(false),
                      supports_var_onion_optin(false), supports_static_remote_key(false),
                      supports_payment_secret(false), supports_basic_mpp(false) {}
};

/**
 * Lightning network packet
 */
struct LightningPacket {
    LightningMessageType type;
    std::vector<uint8_t> payload;
    DilithiumPubKey sender;  // Who sent this message

    LightningPacket() : type(LightningMessageType::INIT) {}
    LightningPacket(LightningMessageType t, const std::vector<uint8_t>& p,
                    const DilithiumPubKey& s)
        : type(t), payload(p), sender(s) {}

    std::vector<uint8_t> serialize() const;
    static std::optional<LightningPacket> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Lightning Network Manager
 * Integrates Lightning protocol with P2P network layer
 */
class LightningNetworkManager {
public:
    LightningNetworkManager(std::shared_ptr<LightningNode> ln_node,
                           std::shared_ptr<p2p::Network> p2p_network);
    ~LightningNetworkManager();

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const { return running_; }

    // Peer management
    bool connect_to_peer(const DilithiumPubKey& node_id, const p2p::PeerAddress& addr);
    void disconnect_peer(const DilithiumPubKey& node_id);
    std::vector<LightningPeer> get_connected_peers() const;
    std::optional<LightningPeer> get_peer(const DilithiumPubKey& node_id) const;
    size_t peer_count() const;

    // Message handling
    void send_message(const DilithiumPubKey& node_id, const messages::Message& msg);
    void broadcast_channel_announcement(const Hash256& channel_id);
    void broadcast_node_announcement();

    // Channel operations (with P2P integration)
    bool open_channel_with_peer(const DilithiumPubKey& remote_node,
                                uint64_t capacity_sat,
                                uint64_t push_amount_sat = 0);
    bool close_channel_with_peer(const Hash256& channel_id, bool force = false);

    // Payment operations
    bool send_payment_through_network(const DilithiumPubKey& destination,
                                     uint64_t amount_sat,
                                     const Hash256& payment_hash);

    // Network graph updates
    void sync_network_graph();
    void request_channel_announcements();

    // Statistics
    struct NetworkStats {
        size_t connected_peers;
        size_t announced_channels;
        size_t announced_nodes;
        size_t pending_htlcs;
        uint64_t total_network_capacity;
        size_t messages_sent;
        size_t messages_received;
    };

    NetworkStats get_stats() const;

    // Callbacks for events
    using ChannelOpenCallback = std::function<void(const Hash256&, const DilithiumPubKey&)>;
    using PaymentReceivedCallback = std::function<void(const Hash256&, uint64_t)>;
    using MessageReceivedCallback = std::function<void(const DilithiumPubKey&, const messages::Message&)>;

    void set_channel_open_callback(ChannelOpenCallback cb) { channel_open_callback_ = cb; }
    void set_payment_received_callback(PaymentReceivedCallback cb) { payment_received_callback_ = cb; }
    void set_message_received_callback(MessageReceivedCallback cb) { message_received_callback_ = cb; }

private:
    std::shared_ptr<LightningNode> lightning_node_;
    std::shared_ptr<p2p::Network> p2p_network_;
    bool running_;

    // Peer tracking
    std::map<DilithiumPubKey, LightningPeer> peers_;
    std::map<Hash256, DilithiumPubKey> channel_to_peer_;  // Channel ID -> Peer node ID

    // Message tracking
    std::map<DilithiumPubKey, std::vector<LightningPacket>> pending_messages_;
    size_t messages_sent_;
    size_t messages_received_;

    // Network graph cache
    struct NodeInfo {
        DilithiumPubKey node_id;
        p2p::PeerAddress last_known_address;
        uint32_t timestamp;
        std::string alias;
        std::vector<uint8_t> rgb_color;
    };
    std::map<DilithiumPubKey, NodeInfo> node_directory_;

    // Callbacks
    ChannelOpenCallback channel_open_callback_;
    PaymentReceivedCallback payment_received_callback_;
    MessageReceivedCallback message_received_callback_;

    // Internal message handlers
    void handle_init(const LightningPacket& packet);
    void handle_error(const LightningPacket& packet);
    void handle_ping(const LightningPacket& packet);
    void handle_pong(const LightningPacket& packet);
    void handle_open_channel(const LightningPacket& packet);
    void handle_accept_channel(const LightningPacket& packet);
    void handle_funding_created(const LightningPacket& packet);
    void handle_funding_signed(const LightningPacket& packet);
    void handle_update_add_htlc(const LightningPacket& packet);
    void handle_update_fulfill_htlc(const LightningPacket& packet);
    void handle_update_fail_htlc(const LightningPacket& packet);
    void handle_commitment_signed(const LightningPacket& packet);
    void handle_revoke_and_ack(const LightningPacket& packet);
    void handle_shutdown(const LightningPacket& packet);
    void handle_closing_signed(const LightningPacket& packet);
    void handle_channel_announcement(const LightningPacket& packet);
    void handle_node_announcement(const LightningPacket& packet);
    void handle_channel_update(const LightningPacket& packet);

    // P2P integration
    void on_p2p_message_received(const p2p::Message& msg, const p2p::PeerAddress& from);
    void send_to_p2p(const DilithiumPubKey& node_id, const LightningPacket& packet);

    // Helper functions
    LightningPeer* find_peer(const DilithiumPubKey& node_id);
    void process_message_queue(const DilithiumPubKey& node_id);
    void update_peer_last_seen(const DilithiumPubKey& node_id);
    std::optional<p2p::PeerAddress> resolve_node_address(const DilithiumPubKey& node_id);
};

/**
 * Lightning network protocol constants
 */
namespace ln_protocol {
    static constexpr uint32_t LN_PROTOCOL_VERSION = 1;
    static constexpr size_t MAX_LN_MESSAGE_SIZE = 65536;  // 64 KB (larger due to Dilithium)
    static constexpr uint64_t PING_INTERVAL_SECONDS = 60;
    static constexpr uint16_t DEFAULT_LN_PORT = 9735;  // Standard Lightning port
    static constexpr uint16_t DEFAULT_LN_PORT_TESTNET = 19735;

    // Feature bits
    static constexpr uint8_t FEATURE_DATA_LOSS_PROTECT = 0;
    static constexpr uint8_t FEATURE_INITIAL_ROUTING_SYNC = 3;
    static constexpr uint8_t FEATURE_UPFRONT_SHUTDOWN_SCRIPT = 4;
    static constexpr uint8_t FEATURE_GOSSIP_QUERIES = 7;
    static constexpr uint8_t FEATURE_VAR_ONION_OPTIN = 8;
    static constexpr uint8_t FEATURE_STATIC_REMOTE_KEY = 12;
    static constexpr uint8_t FEATURE_PAYMENT_SECRET = 14;
    static constexpr uint8_t FEATURE_BASIC_MPP = 16;
}

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_NETWORK_H
