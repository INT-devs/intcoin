// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_LIGHTNING_NODE_H
#define INTCOIN_LIGHTNING_NODE_H

#include "channel.h"
#include "routing.h"
#include "invoice.h"
#include "../primitives.h"
#include "../blockchain.h"
#include <memory>
#include <functional>

namespace intcoin {
namespace lightning {

/**
 * Lightning Network node
 *
 * Complete Lightning Network implementation for instant, low-cost payments
 */
class LightningNode {
public:
    LightningNode(const std::string& alias,
                 const PrivateKey& node_key,
                 Blockchain* blockchain);
    ~LightningNode();

    // Node lifecycle
    bool start(uint16_t port = 9735);
    void stop();
    bool is_running() const { return running_; }

    // Node info
    PublicKey get_node_id() const { return node_key_.to_public(); }
    std::string get_alias() const { return alias_; }
    std::vector<std::string> get_addresses() const;

    // Channel management
    Hash256 open_channel(const PublicKey& peer,
                        uint64_t local_amount,
                        uint64_t push_amount = 0);
    bool close_channel(const Hash256& channel_id, bool force = false);
    std::vector<std::shared_ptr<PaymentChannel>> list_channels() const;
    std::optional<std::shared_ptr<PaymentChannel>> get_channel(
        const Hash256& channel_id) const;

    // Payment operations
    struct PaymentResult {
        bool success;
        Hash256 payment_hash;
        Hash256 payment_preimage;
        PaymentRoute route;
        uint64_t amount_sent;
        uint64_t fees_paid;
        std::string error_message;

        PaymentResult() : success(false), amount_sent(0), fees_paid(0) {}
    };

    PaymentResult send_payment(const Invoice& invoice);
    PaymentResult send_payment(const PublicKey& destination,
                              uint64_t amount_msat,
                              const Hash256& payment_hash);

    // Multi-path payments
    PaymentResult send_multi_path_payment(const Invoice& invoice,
                                         size_t max_paths = 5);

    // Invoice operations
    Invoice create_invoice(uint64_t amount_msat,
                          const std::string& description,
                          uint32_t expiry = 3600);
    bool decode_invoice(const std::string& invoice_str);
    std::vector<Invoice> list_invoices() const;

    // Routing
    std::vector<PaymentRoute> find_route(const PublicKey& destination,
                                        uint64_t amount_msat);
    bool add_channel_to_graph(const ChannelAnnouncement& announcement);
    bool update_channel_in_graph(const ChannelUpdate& update);

    // Network gossip
    void sync_graph();
    void broadcast_channel(const Hash256& channel_id);
    void broadcast_node_announcement();

    // Peer management
    bool connect_peer(const std::string& address, uint16_t port);
    void disconnect_peer(const PublicKey& peer);
    std::vector<PublicKey> list_peers() const;
    bool is_connected(const PublicKey& peer) const;

    // Statistics
    struct NodeStats {
        size_t num_channels;
        size_t num_active_channels;
        size_t num_peers;
        uint64_t total_capacity;
        uint64_t total_local_balance;
        uint64_t total_remote_balance;
        uint64_t num_payments_sent;
        uint64_t num_payments_received;
        uint64_t total_sent;
        uint64_t total_received;
        uint64_t total_fees_earned;

        NodeStats() : num_channels(0), num_active_channels(0), num_peers(0),
                     total_capacity(0), total_local_balance(0),
                     total_remote_balance(0), num_payments_sent(0),
                     num_payments_received(0), total_sent(0),
                     total_received(0), total_fees_earned(0) {}
    };

    NodeStats get_stats() const;

    // Configuration
    struct Config {
        uint32_t cltv_expiry_delta = 40;
        uint64_t htlc_minimum_msat = 1000;
        uint64_t htlc_maximum_msat = 10000000000;  // 0.1 INT
        uint32_t fee_base_msat = 1000;
        uint32_t fee_proportional_millionths = 100;
        size_t max_htlc_in_flight = 483;
        uint64_t channel_reserve_satoshis = 10000;
        size_t max_accepted_htlcs = 483;
        bool accept_inbound_channels = true;
        bool auto_pilot = false;
    };

    void set_config(const Config& config) { config_ = config; }
    Config get_config() const { return config_; }

    // Callbacks
    void set_payment_received_callback(
        std::function<void(const Hash256&, uint64_t)> callback);
    void set_payment_sent_callback(
        std::function<void(const Hash256&, uint64_t)> callback);
    void set_channel_opened_callback(
        std::function<void(const Hash256&)> callback);
    void set_channel_closed_callback(
        std::function<void(const Hash256&)> callback);

    // Backup and recovery
    bool backup_channels(const std::string& filepath);
    bool restore_channels(const std::string& filepath);

private:
    // Core components
    PrivateKey node_key_;
    std::string alias_;
    Blockchain* blockchain_;
    bool running_;
    uint16_t port_;

    // Managers
    std::unique_ptr<ChannelManager> channel_manager_;
    std::unique_ptr<NetworkGraph> network_graph_;
    std::unique_ptr<RouteFinder> route_finder_;
    std::unique_ptr<InvoiceManager> invoice_manager_;
    std::unique_ptr<PreimageGenerator> preimage_generator_;

    // Configuration
    Config config_;

    // Statistics
    mutable std::mutex stats_mutex_;
    NodeStats stats_;

    // Callbacks
    std::function<void(const Hash256&, uint64_t)> payment_received_callback_;
    std::function<void(const Hash256&, uint64_t)> payment_sent_callback_;
    std::function<void(const Hash256&)> channel_opened_callback_;
    std::function<void(const Hash256&)> channel_closed_callback_;

    // Payment processing
    bool process_incoming_payment(const Hash256& payment_hash,
                                  uint64_t amount_msat);
    bool forward_payment(const HTLC& htlc, const PublicKey& next_hop);

    // HTLC handling
    bool add_htlc_to_channel(const Hash256& channel_id, const HTLC& htlc);
    bool settle_htlc_in_channel(const Hash256& channel_id,
                               const Hash256& payment_hash,
                               const Hash256& preimage);
    bool fail_htlc_in_channel(const Hash256& channel_id,
                             const Hash256& payment_hash);

    // Network communication
    void handle_peer_message(const PublicKey& peer,
                           const std::vector<uint8_t>& message);
    void send_to_peer(const PublicKey& peer,
                     const std::vector<uint8_t>& message);

    // Monitoring
    void monitor_blockchain();
    void monitor_channels();
    void process_pending_htlcs();

    // Helper methods
    std::vector<PaymentRoute> find_best_routes(const PublicKey& destination,
                                              uint64_t amount_msat,
                                              size_t count = 5);
    bool execute_payment_route(const PaymentRoute& route,
                              const Hash256& payment_hash);
};

/**
 * Lightning Network utilities
 */
class LightningUtils {
public:
    // Amount conversions
    static uint64_t satoshi_to_millisatoshi(uint64_t satoshi) {
        return satoshi * 1000;
    }
    static uint64_t millisatoshi_to_satoshi(uint64_t msat) {
        return (msat + 999) / 1000;  // Round up
    }

    // Short channel ID encoding
    static uint64_t encode_short_channel_id(uint32_t block_height,
                                            uint32_t tx_index,
                                            uint32_t output_index);
    static void decode_short_channel_id(uint64_t short_id,
                                       uint32_t& block_height,
                                       uint32_t& tx_index,
                                       uint32_t& output_index);

    // Feature bits
    static constexpr uint32_t FEATURE_OPTION_DATA_LOSS_PROTECT = 0;
    static constexpr uint32_t FEATURE_INITIAL_ROUTING_SYNC = 3;
    static constexpr uint32_t FEATURE_GOSSIP_QUERIES = 7;
    static constexpr uint32_t FEATURE_VAR_ONION_OPTIN = 9;
    static constexpr uint32_t FEATURE_PAYMENT_SECRET = 14;
    static constexpr uint32_t FEATURE_BASIC_MPP = 16;

    // CLTV calculations
    static uint32_t calculate_cltv_expiry(uint32_t current_height,
                                          uint32_t delta);
    static bool is_cltv_expired(uint32_t expiry, uint32_t current_height);

    // Fee calculations
    static uint64_t calculate_routing_fee(uint64_t amount_msat,
                                          uint32_t fee_base_msat,
                                          uint32_t fee_proportional);
};

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_NODE_H
