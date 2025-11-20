// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Privacy Protection - IP privacy, transaction unlinkability, Tor/I2P support, SPV privacy

#ifndef INTCOIN_PRIVACY_H
#define INTCOIN_PRIVACY_H

#include "primitives.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <optional>
#include <memory>
#include <chrono>

namespace intcoin {
namespace privacy {

/**
 * Network types for privacy routing
 */
enum class NetworkType {
    CLEARNET,   // Regular internet
    TOR,        // Tor network (.onion)
    I2P,        // I2P network (.b32.i2p)
    UNKNOWN
};

/**
 * IP address privacy protection
 * Prevents IP address correlation and tracking
 */
class IPAddressPrivacy {
public:
    IPAddressPrivacy();

    // IP address handling
    struct PrivateAddress {
        std::string address;      // IP or onion/i2p address
        NetworkType network_type;
        bool is_local;
        bool is_private_range;
        bool should_relay;        // Whether to relay to other peers
    };

    // Validate and classify address
    PrivateAddress classify_address(const std::string& address) const;
    bool is_routable(const std::string& address) const;
    bool is_local_address(const std::string& address) const;
    bool is_private_range(const std::string& address) const;

    // Privacy-preserving address handling
    std::vector<std::string> filter_addresses_for_relay(
        const std::vector<std::string>& addresses) const;

    bool should_announce_address(const std::string& address) const;

    // Prevent IP correlation
    std::string sanitize_address_for_logging(const std::string& address) const;

    // Tor/I2P detection
    bool is_tor_address(const std::string& address) const;
    bool is_i2p_address(const std::string& address) const;
    NetworkType detect_network_type(const std::string& address) const;

    // Local network detection (RFC 1918, RFC 4193)
    bool is_rfc1918(const std::string& address) const;  // 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16
    bool is_rfc4193(const std::string& address) const;  // fc00::/7 (IPv6 ULA)
    bool is_link_local(const std::string& address) const;  // 169.254.0.0/16, fe80::/10

    // Privacy configuration
    void set_relay_private_addresses(bool relay) { relay_private_addresses_ = relay; }
    bool get_relay_private_addresses() const { return relay_private_addresses_; }

private:
    bool relay_private_addresses_;

    bool is_ipv4(const std::string& address) const;
    bool is_ipv6(const std::string& address) const;
};

/**
 * Transaction unlinkability protection
 * Prevents transaction graph analysis and address clustering
 */
class TransactionUnlinkability {
public:
    TransactionUnlinkability();

    // Transaction privacy analysis
    struct PrivacyScore {
        double linkability_score;      // 0.0 = unlinkable, 1.0 = highly linkable
        double address_reuse_score;    // 0.0 = no reuse, 1.0 = heavy reuse
        double timing_correlation;     // 0.0 = no correlation, 1.0 = correlated
        double amount_correlation;     // 0.0 = no patterns, 1.0 = obvious patterns
        std::vector<std::string> privacy_warnings;
        std::vector<std::string> privacy_suggestions;
    };

    // Analyze transaction privacy
    PrivacyScore analyze_transaction_privacy(const Transaction& tx) const;

    // Check for common privacy issues
    bool has_address_reuse(const Transaction& tx) const;
    bool has_round_amount(const Transaction& tx) const;  // e.g., exactly 1.0 BTC
    bool has_change_address_leak(const Transaction& tx) const;
    bool has_timing_correlation(const Transaction& tx,
                                  const std::vector<Transaction>& recent_txs) const;

    // Privacy-enhancing recommendations
    struct PrivacyRecommendation {
        std::string issue;
        std::string recommendation;
        std::string severity;  // "high", "medium", "low"
    };
    std::vector<PrivacyRecommendation> get_privacy_recommendations(
        const Transaction& tx) const;

    // CoinJoin detection (privacy enhancement indicator)
    bool appears_to_be_coinjoin(const Transaction& tx) const;

    // UTXO selection privacy
    struct UTXOSelectionStrategy {
        enum Type {
            PRIVACY_FOCUSED,      // Maximize privacy (randomize selection)
            EFFICIENCY_FOCUSED,   // Minimize fees (use largest UTXOs)
            BALANCED              // Balance privacy and efficiency
        };
        Type strategy_type;
        bool avoid_address_reuse;
        bool randomize_order;
        double target_anonymity_set;  // Target number of similar transactions
    };

    std::vector<UTXO> select_utxos_for_privacy(
        const std::vector<UTXO>& available_utxos,
        uint64_t target_amount,
        const UTXOSelectionStrategy& strategy) const;

    // Transaction construction privacy
    bool should_use_multiple_outputs(uint64_t amount) const;
    std::vector<uint64_t> create_decoy_amounts(uint64_t real_amount) const;

private:
    // Privacy heuristics
    bool is_round_amount(uint64_t amount) const;
    double calculate_amount_pattern_score(const std::vector<uint64_t>& amounts) const;
    bool outputs_have_obvious_change(const Transaction& tx) const;
};

/**
 * Tor/I2P compatibility layer
 * Ensures privacy networks work correctly
 */
class PrivacyNetworkCompatibility {
public:
    PrivacyNetworkCompatibility();

    // Network configuration
    struct NetworkConfig {
        bool tor_enabled;
        bool i2p_enabled;
        bool clearnet_enabled;

        // Tor settings
        std::string tor_proxy;        // e.g., "127.0.0.1:9050"
        std::string tor_control_port; // e.g., "127.0.0.1:9051"
        bool tor_stream_isolation;    // Use different circuits per connection

        // I2P settings
        std::string i2p_sam_host;     // e.g., "127.0.0.1"
        uint16_t i2p_sam_port;        // e.g., 9336
        bool i2p_transient_keys;      // Generate new keys per session

        // Privacy preferences
        bool only_privacy_networks;   // Disable clearnet entirely
        bool prefer_privacy_networks; // Try Tor/I2P first
    };

    // Connectivity validation
    bool is_tor_available() const;
    bool is_i2p_available() const;
    bool can_connect_to_network(NetworkType network) const;

    // Connection routing
    std::string get_proxy_for_network(NetworkType network) const;
    bool should_use_proxy(const std::string& address) const;

    // Privacy-preserving connection
    struct ConnectionInfo {
        std::string target_address;
        NetworkType network_type;
        std::string proxy_address;
        bool use_stream_isolation;
        std::string isolation_key;  // For Tor stream isolation
    };
    ConnectionInfo prepare_connection(const std::string& target_address) const;

    // DNS privacy (prevent DNS leaks)
    bool should_resolve_dns(const std::string& hostname) const;
    std::optional<std::string> resolve_via_privacy_network(
        const std::string& hostname, NetworkType network) const;

    // Network mixing (connect via multiple networks)
    std::vector<NetworkType> get_available_networks() const;
    NetworkType select_network_for_connection(const std::string& target) const;

    // Configuration
    void set_config(const NetworkConfig& config) { config_ = config; }
    NetworkConfig get_config() const { return config_; }

    // Statistics
    struct NetworkStats {
        uint64_t tor_connections;
        uint64_t i2p_connections;
        uint64_t clearnet_connections;
        double tor_success_rate;
        double i2p_success_rate;
        double clearnet_success_rate;
    };
    NetworkStats get_network_stats() const;

private:
    NetworkConfig config_;
    NetworkStats stats_;

    bool test_socks_proxy(const std::string& proxy) const;
    bool test_i2p_sam(const std::string& host, uint16_t port) const;
};

/**
 * SPV (Simplified Payment Verification) client privacy
 * Prevents address leakage in light clients
 */
class SPVPrivacy {
public:
    SPVPrivacy();

    // Bloom filter privacy (BIP 37)
    struct BloomFilterConfig {
        uint32_t elements;          // Number of elements
        double false_positive_rate; // FPR (higher = more privacy)
        uint32_t tweak;            // Random tweak for filter
        bool auto_update;          // Update filter periodically
    };

    // Create privacy-preserving bloom filter
    std::vector<uint8_t> create_bloom_filter(
        const std::vector<std::string>& addresses,
        const BloomFilterConfig& config) const;

    // Privacy analysis
    struct FilterPrivacy {
        double effective_anonymity_set;  // How many addresses could match
        double information_leakage;      // 0.0 = no leak, 1.0 = full leak
        std::vector<std::string> warnings;
    };
    FilterPrivacy analyze_filter_privacy(
        const std::vector<uint8_t>& filter,
        uint32_t num_addresses) const;

    // Address leakage prevention
    bool should_include_address_in_filter(const std::string& address) const;
    std::vector<std::string> add_decoy_addresses(
        const std::vector<std::string>& real_addresses,
        uint32_t num_decoys) const;

    // Filter rotation (prevent tracking)
    bool should_rotate_filter(uint64_t current_height,
                               uint64_t filter_created_height) const;

    // BIP 158/159 compact block filters (better privacy than bloom)
    bool prefer_compact_block_filters() const;

    // Privacy-preserving transaction fetching
    struct FetchStrategy {
        bool fetch_full_blocks;        // Fetch entire blocks (max privacy)
        bool use_multiple_peers;       // Request from different peers
        bool add_timing_jitter;        // Randomize request timing
        bool fetch_extra_transactions; // Fetch unrelated txs as cover
    };

    std::vector<Hash256> select_blocks_to_fetch(
        const std::vector<Hash256>& blocks_with_my_txs,
        const FetchStrategy& strategy) const;

    // Neutrino protocol privacy (BIP 157/158)
    bool supports_neutrino() const { return true; }

private:
    static constexpr double MIN_FALSE_POSITIVE_RATE = 0.0001;  // 0.01%
    static constexpr double RECOMMENDED_FPR = 0.001;           // 0.1%
    static constexpr uint32_t FILTER_ROTATION_BLOCKS = 1000;   // Rotate every 1000 blocks

    uint32_t calculate_optimal_filter_size(uint32_t elements, double fpr) const;
    uint32_t calculate_optimal_hash_functions(double fpr) const;
};

/**
 * Wallet privacy manager
 * Coordinates all privacy features for wallet operations
 */
class WalletPrivacy {
public:
    WalletPrivacy();

    // Privacy mode
    enum class PrivacyMode {
        STANDARD,      // Basic privacy (address generation, no reuse)
        ENHANCED,      // Enhanced privacy (random UTXO selection, timing jitter)
        MAXIMUM        // Maximum privacy (Tor/I2P only, decoy transactions)
    };

    // Address management
    std::string generate_new_address(PrivacyMode mode);
    bool should_reuse_address(const std::string& address) const;

    // Transaction creation privacy
    struct PrivateTransaction {
        Transaction tx;
        double privacy_score;
        std::vector<std::string> privacy_notes;
    };

    PrivateTransaction create_private_transaction(
        const std::string& to_address,
        uint64_t amount,
        PrivacyMode mode);

    // Network privacy
    void set_network_mode(PrivacyMode mode);
    bool should_use_privacy_network() const;
    NetworkType select_network() const;

    // Balance queries (prevent leakage)
    struct PrivateBalanceQuery {
        bool use_bloom_filter;
        double bloom_fpr;
        bool fetch_full_blocks;
        bool use_multiple_peers;
    };
    PrivateBalanceQuery get_balance_query_strategy(PrivacyMode mode) const;

    // Transaction broadcast privacy
    struct BroadcastStrategy {
        bool broadcast_to_multiple_peers;
        bool use_different_networks;  // Tor, I2P, clearnet
        bool add_timing_delay;
        uint32_t delay_seconds;
    };
    BroadcastStrategy get_broadcast_strategy(PrivacyMode mode) const;

    // Privacy statistics
    struct PrivacyStats {
        uint32_t addresses_generated;
        uint32_t addresses_reused;
        double average_tx_privacy_score;
        uint32_t tor_connections_used;
        uint32_t i2p_connections_used;
        uint32_t clearnet_connections_used;
    };
    PrivacyStats get_privacy_stats() const;

    // Configuration
    void set_privacy_mode(PrivacyMode mode) { privacy_mode_ = mode; }
    PrivacyMode get_privacy_mode() const { return privacy_mode_; }

private:
    PrivacyMode privacy_mode_;
    PrivacyStats stats_;

    uint64_t get_timing_jitter(PrivacyMode mode) const;
    bool should_add_decoy_outputs(PrivacyMode mode) const;
};

/**
 * Privacy settings and configuration
 */
struct PrivacySettings {
    // Network privacy
    bool enable_tor;
    bool enable_i2p;
    bool disable_clearnet;
    bool prefer_privacy_networks;

    // IP privacy
    bool relay_private_addresses;
    bool sanitize_logs;

    // Transaction privacy
    bool avoid_address_reuse;
    bool randomize_utxo_selection;
    bool use_privacy_focused_utxo_selection;
    double target_false_positive_rate;  // For bloom filters

    // SPV privacy
    bool use_bloom_filters;
    bool use_compact_block_filters;  // BIP 158
    bool fetch_full_blocks;
    bool rotate_filters_regularly;

    // Wallet privacy
    WalletPrivacy::PrivacyMode default_privacy_mode;

    // Defaults
    static PrivacySettings standard() {
        PrivacySettings s;
        s.enable_tor = false;
        s.enable_i2p = false;
        s.disable_clearnet = false;
        s.prefer_privacy_networks = false;
        s.relay_private_addresses = false;
        s.sanitize_logs = true;
        s.avoid_address_reuse = true;
        s.randomize_utxo_selection = false;
        s.use_privacy_focused_utxo_selection = false;
        s.target_false_positive_rate = 0.001;
        s.use_bloom_filters = true;
        s.use_compact_block_filters = false;
        s.fetch_full_blocks = false;
        s.rotate_filters_regularly = true;
        s.default_privacy_mode = WalletPrivacy::PrivacyMode::STANDARD;
        return s;
    }

    static PrivacySettings enhanced() {
        PrivacySettings s = standard();
        s.enable_tor = true;
        s.prefer_privacy_networks = true;
        s.randomize_utxo_selection = true;
        s.use_compact_block_filters = true;
        s.target_false_positive_rate = 0.01;  // Higher FPR = more privacy
        s.default_privacy_mode = WalletPrivacy::PrivacyMode::ENHANCED;
        return s;
    }

    static PrivacySettings maximum() {
        PrivacySettings s = enhanced();
        s.enable_i2p = true;
        s.disable_clearnet = true;
        s.use_privacy_focused_utxo_selection = true;
        s.use_compact_block_filters = true;
        s.fetch_full_blocks = true;
        s.target_false_positive_rate = 0.1;  // Very high FPR
        s.default_privacy_mode = WalletPrivacy::PrivacyMode::MAXIMUM;
        return s;
    }
};

/**
 * Privacy manager - Central coordination
 */
class PrivacyManager {
public:
    static PrivacyManager& instance();

    // Initialize privacy subsystems
    void initialize(const PrivacySettings& settings);
    void shutdown();

    // Access components
    IPAddressPrivacy& ip_privacy() { return *ip_privacy_; }
    TransactionUnlinkability& tx_unlinkability() { return *tx_unlinkability_; }
    PrivacyNetworkCompatibility& network_compat() { return *network_compat_; }
    SPVPrivacy& spv_privacy() { return *spv_privacy_; }
    WalletPrivacy& wallet_privacy() { return *wallet_privacy_; }

    // Settings
    void set_settings(const PrivacySettings& settings) { settings_ = settings; }
    PrivacySettings get_settings() const { return settings_; }

    // Privacy audit
    struct PrivacyAudit {
        bool ip_privacy_enabled;
        bool tor_available;
        bool i2p_available;
        bool clearnet_disabled;
        double average_tx_privacy_score;
        uint32_t privacy_warnings;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
    };
    PrivacyAudit audit_privacy() const;

private:
    PrivacyManager();
    ~PrivacyManager() = default;
    PrivacyManager(const PrivacyManager&) = delete;
    PrivacyManager& operator=(const PrivacyManager&) = delete;

    std::unique_ptr<IPAddressPrivacy> ip_privacy_;
    std::unique_ptr<TransactionUnlinkability> tx_unlinkability_;
    std::unique_ptr<PrivacyNetworkCompatibility> network_compat_;
    std::unique_ptr<SPVPrivacy> spv_privacy_;
    std::unique_ptr<WalletPrivacy> wallet_privacy_;

    PrivacySettings settings_;
    bool initialized_;
};

} // namespace privacy
} // namespace intcoin

#endif // INTCOIN_PRIVACY_H
