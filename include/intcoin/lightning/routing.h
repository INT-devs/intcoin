// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_LIGHTNING_ROUTING_H
#define INTCOIN_LIGHTNING_ROUTING_H

#include "../primitives.h"
#include "channel.h"
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <optional>

namespace intcoin {
namespace lightning {

/**
 * Channel announcement
 *
 * Broadcast to advertise a channel to the network
 */
struct ChannelAnnouncement {
    Hash256 channel_id;
    PublicKey node1_key;
    PublicKey node2_key;
    uint64_t capacity;
    uint32_t features;

    ChannelAnnouncement() : capacity(0), features(0) {}
};

/**
 * Channel update
 *
 * Updates routing information for a channel
 */
struct ChannelUpdate {
    Hash256 channel_id;
    uint32_t timestamp;
    uint16_t cltv_expiry_delta;
    uint64_t htlc_minimum;
    uint64_t htlc_maximum;
    uint32_t fee_base;          // Base fee in millisatoshis
    uint32_t fee_proportional;  // Proportional fee in millionths
    bool disabled;

    ChannelUpdate() : timestamp(0), cltv_expiry_delta(0), htlc_minimum(0),
                     htlc_maximum(0), fee_base(0), fee_proportional(0),
                     disabled(false) {}

    // Calculate fee for amount
    uint64_t calculate_fee(uint64_t amount) const {
        return fee_base + (amount * fee_proportional / 1000000);
    }
};

/**
 * Node announcement
 *
 * Advertise a Lightning node to the network
 */
struct NodeAnnouncement {
    PublicKey node_key;
    std::string alias;
    std::vector<std::string> addresses;  // Network addresses
    uint32_t timestamp;
    uint32_t features;

    NodeAnnouncement() : timestamp(0), features(0) {}
};

/**
 * Payment route hop
 *
 * Represents one hop in a multi-hop payment route
 */
struct RouteHop {
    PublicKey node_key;
    Hash256 channel_id;
    uint64_t amount;           // Amount to forward
    uint64_t fee;              // Fee charged by this hop
    uint32_t cltv_expiry;      // CLTV expiry
    uint16_t cltv_delta;       // Blocks to subtract

    RouteHop() : amount(0), fee(0), cltv_expiry(0), cltv_delta(0) {}
};

/**
 * Complete payment route
 */
struct PaymentRoute {
    std::vector<RouteHop> hops;
    uint64_t total_amount;     // Total including fees
    uint64_t total_fees;       // Sum of all hop fees
    uint32_t total_cltv;       // Total CLTV
    double success_probability;

    PaymentRoute() : total_amount(0), total_fees(0), total_cltv(0),
                    success_probability(0.0) {}

    // Route validation
    bool is_valid() const { return !hops.empty(); }
    size_t hop_count() const { return hops.size(); }

    // Get source and destination
    PublicKey get_source() const {
        return hops.empty() ? PublicKey() : hops.front().node_key;
    }
    PublicKey get_destination() const {
        return hops.empty() ? PublicKey() : hops.back().node_key;
    }
};

/**
 * Network graph edge (channel)
 */
struct GraphEdge {
    Hash256 channel_id;
    PublicKey from_node;
    PublicKey to_node;
    uint64_t capacity;
    ChannelUpdate update;
    bool enabled;

    GraphEdge() : capacity(0), enabled(true) {}

    // Check if channel can route payment
    bool can_route(uint64_t amount) const {
        return enabled && !update.disabled &&
               amount >= update.htlc_minimum &&
               amount <= update.htlc_maximum;
    }
};

/**
 * Network graph node
 */
struct GraphNode {
    PublicKey node_key;
    NodeAnnouncement announcement;
    std::vector<Hash256> channels;  // Connected channels
    uint32_t last_update;

    GraphNode() : last_update(0) {}
};

/**
 * Lightning Network graph
 *
 * Maintains topology of the Lightning Network for routing
 */
class NetworkGraph {
public:
    NetworkGraph();
    ~NetworkGraph();

    // Graph updates
    bool add_node(const NodeAnnouncement& announcement);
    bool add_channel(const ChannelAnnouncement& announcement);
    bool update_channel(const ChannelUpdate& update);
    bool remove_channel(const Hash256& channel_id);
    bool remove_node(const PublicKey& node_key);

    // Graph queries
    std::optional<GraphNode> get_node(const PublicKey& node_key) const;
    std::optional<GraphEdge> get_channel(const Hash256& channel_id) const;
    std::vector<GraphEdge> get_node_channels(const PublicKey& node_key) const;
    std::vector<PublicKey> get_neighbors(const PublicKey& node_key) const;

    // Network statistics
    size_t get_node_count() const;
    size_t get_channel_count() const;
    uint64_t get_total_capacity() const;

    // Pathfinding
    std::vector<PaymentRoute> find_routes(const PublicKey& source,
                                          const PublicKey& destination,
                                          uint64_t amount,
                                          size_t max_routes = 5) const;

    // Graph maintenance
    void prune_old_entries(uint32_t max_age);
    void disable_channel(const Hash256& channel_id);
    void enable_channel(const Hash256& channel_id);

private:
    std::map<PublicKey, GraphNode> nodes_;
    std::map<Hash256, GraphEdge> channels_;
    mutable std::mutex graph_mutex_;

    // Pathfinding helpers
    std::vector<PaymentRoute> dijkstra(const PublicKey& source,
                                       const PublicKey& destination,
                                       uint64_t amount,
                                       size_t max_routes) const;
    double calculate_edge_weight(const GraphEdge& edge, uint64_t amount) const;
    double estimate_success_probability(const PaymentRoute& route) const;
};

/**
 * Route finder
 *
 * Advanced pathfinding with multiple algorithms
 */
class RouteFinder {
public:
    RouteFinder(const NetworkGraph& graph);

    // Routing algorithms
    std::vector<PaymentRoute> find_shortest_path(const PublicKey& source,
                                                  const PublicKey& dest,
                                                  uint64_t amount);

    std::vector<PaymentRoute> find_cheapest_path(const PublicKey& source,
                                                  const PublicKey& dest,
                                                  uint64_t amount);

    std::vector<PaymentRoute> find_most_reliable_path(const PublicKey& source,
                                                       const PublicKey& dest,
                                                       uint64_t amount);

    // Multi-path payments
    std::vector<PaymentRoute> find_multi_path(const PublicKey& source,
                                              const PublicKey& dest,
                                              uint64_t total_amount,
                                              size_t max_paths);

    // Route optimization
    PaymentRoute optimize_route(const PaymentRoute& route);
    std::vector<PaymentRoute> rank_routes(std::vector<PaymentRoute>& routes);

    // Configuration
    void set_max_hops(size_t max_hops) { max_hops_ = max_hops; }
    void set_max_fee_percent(double percent) { max_fee_percent_ = percent; }

private:
    const NetworkGraph& graph_;
    size_t max_hops_;
    double max_fee_percent_;

    // Pathfinding utilities
    bool is_route_valid(const PaymentRoute& route, uint64_t amount) const;
    double calculate_route_score(const PaymentRoute& route) const;
};

/**
 * Onion routing
 *
 * Creates onion-encrypted payment packets for privacy
 */
class OnionRouter {
public:
    OnionRouter();

    // Create onion packet
    std::vector<uint8_t> create_onion(const PaymentRoute& route,
                                      const Hash256& payment_hash,
                                      uint64_t amount);

    // Peel onion layer
    struct OnionPayload {
        PublicKey next_node;
        Hash256 next_channel;
        uint64_t amount;
        uint32_t cltv_expiry;
        std::vector<uint8_t> next_onion;  // Encrypted for next hop
        bool is_final;

        OnionPayload() : amount(0), cltv_expiry(0), is_final(false) {}
    };

    std::optional<OnionPayload> peel_onion(const std::vector<uint8_t>& onion,
                                           const PrivateKey& node_key);

    // Sphinx packet construction
    static constexpr size_t MAX_HOPS = 20;
    static constexpr size_t PACKET_SIZE = 1366;

private:
    // Onion encryption
    std::vector<uint8_t> encrypt_layer(const std::vector<uint8_t>& data,
                                       const PublicKey& node_key);
    std::vector<uint8_t> decrypt_layer(const std::vector<uint8_t>& data,
                                       const PrivateKey& node_key);

    // Shared secret derivation
    Hash256 derive_shared_secret(const PublicKey& pub, const PrivateKey& priv);
};

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_ROUTING_H
