// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_BRIDGE_H
#define INTCOIN_BRIDGE_H

#include "atomic_swap.h"
#include "spv_proof.h"
#include "../primitives.h"
#include "../blockchain.h"
#include <memory>
#include <string>
#include <functional>

namespace intcoin {
namespace bridge {

/**
 * Bridge status
 */
enum class BridgeStatus {
    OFFLINE,
    SYNCING,
    ONLINE,
    ERROR
};

/**
 * Bridge statistics
 */
struct BridgeStats {
    size_t total_swaps;
    size_t completed_swaps;
    size_t failed_swaps;
    uint64_t total_volume_sent;
    uint64_t total_volume_received;
    uint32_t avg_swap_time;
    double success_rate;

    BridgeStats() : total_swaps(0), completed_swaps(0), failed_swaps(0),
                   total_volume_sent(0), total_volume_received(0),
                   avg_swap_time(0), success_rate(0.0) {}
};

/**
 * Cross-chain bridge interface
 *
 * Base class for all blockchain bridges
 */
class Bridge {
public:
    virtual ~Bridge() = default;

    // Bridge lifecycle
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;

    // Bridge info
    virtual ChainType get_chain_type() const = 0;
    virtual std::string get_chain_name() const = 0;
    virtual BridgeStatus get_status() const = 0;

    // Swap operations
    virtual Hash256 initiate_swap(const DilithiumPubKey& recipient,
                                   uint64_t amount) = 0;
    virtual bool complete_swap(const Hash256& swap_id,
                              const Hash256& secret) = 0;
    virtual bool refund_swap(const Hash256& swap_id) = 0;

    // Proof verification
    virtual bool verify_lock_proof(const Hash256& swap_id,
                                   const CrossChainProof& proof) = 0;

    // Chain synchronization
    virtual bool sync_chain() = 0;
    virtual uint32_t get_chain_height() const = 0;
    virtual uint32_t get_sync_height() const = 0;

    // Statistics
    virtual BridgeStats get_stats() const = 0;
};

/**
 * Bitcoin bridge
 *
 * Enables atomic swaps between INTcoin and Bitcoin
 */
class BitcoinBridge : public Bridge {
public:
    BitcoinBridge(Blockchain* intcoin_chain,
                 const std::string& bitcoin_rpc_url = "http://localhost:8332");
    ~BitcoinBridge() override;

    // Bridge lifecycle
    bool start() override;
    void stop() override;
    bool is_running() const override { return running_; }

    // Bridge info
    ChainType get_chain_type() const override { return ChainType::BITCOIN; }
    std::string get_chain_name() const override { return "Bitcoin"; }
    BridgeStatus get_status() const override { return status_; }

    // Swap operations
    Hash256 initiate_swap(const DilithiumPubKey& recipient,
                         uint64_t amount) override;
    bool complete_swap(const Hash256& swap_id,
                      const Hash256& secret) override;
    bool refund_swap(const Hash256& swap_id) override;

    // Proof verification
    bool verify_lock_proof(const Hash256& swap_id,
                          const CrossChainProof& proof) override;

    // Chain synchronization
    bool sync_chain() override;
    uint32_t get_chain_height() const override;
    uint32_t get_sync_height() const override;

    // Statistics
    BridgeStats get_stats() const override;

    // Bitcoin-specific
    std::string get_bitcoin_address() const;
    bool verify_bitcoin_transaction(const std::string& txid);

private:
    Blockchain* intcoin_chain_;
    std::string bitcoin_rpc_url_;
    bool running_;
    BridgeStatus status_;

    std::unique_ptr<AtomicSwapManager> swap_manager_;
    std::unique_ptr<BridgeRelay> relay_;
    std::unique_ptr<SPVChainVerifier> btc_verifier_;

    BridgeStats stats_;
    mutable std::mutex stats_mutex_;

    // Bitcoin RPC
    bool query_bitcoin_rpc(const std::string& method,
                          const std::string& params,
                          std::string& result);
    bool get_bitcoin_block_header(uint32_t height, SPVBlockHeader& header);

    // Monitoring
    void monitor_swaps();
    void monitor_bitcoin_chain();
};

/**
 * Ethereum bridge
 *
 * Enables atomic swaps between INTcoin and Ethereum
 */
class EthereumBridge : public Bridge {
public:
    EthereumBridge(Blockchain* intcoin_chain,
                  const std::string& ethereum_rpc_url = "http://localhost:8545");
    ~EthereumBridge() override;

    // Bridge lifecycle
    bool start() override;
    void stop() override;
    bool is_running() const override { return running_; }

    // Bridge info
    ChainType get_chain_type() const override { return ChainType::ETHEREUM; }
    std::string get_chain_name() const override { return "Ethereum"; }
    BridgeStatus get_status() const override { return status_; }

    // Swap operations
    Hash256 initiate_swap(const DilithiumPubKey& recipient,
                         uint64_t amount) override;
    bool complete_swap(const Hash256& swap_id,
                      const Hash256& secret) override;
    bool refund_swap(const Hash256& swap_id) override;

    // Proof verification
    bool verify_lock_proof(const Hash256& swap_id,
                          const CrossChainProof& proof) override;

    // Chain synchronization
    bool sync_chain() override;
    uint32_t get_chain_height() const override;
    uint32_t get_sync_height() const override;

    // Statistics
    BridgeStats get_stats() const override;

    // Ethereum-specific
    std::string get_contract_address() const;
    bool deploy_swap_contract();
    bool verify_eth_transaction(const std::string& txhash);

private:
    Blockchain* intcoin_chain_;
    std::string ethereum_rpc_url_;
    std::string contract_address_;
    bool running_;
    BridgeStatus status_;

    std::unique_ptr<AtomicSwapManager> swap_manager_;
    std::unique_ptr<BridgeRelay> relay_;

    BridgeStats stats_;
    mutable std::mutex stats_mutex_;

    // Ethereum JSON-RPC
    bool query_ethereum_rpc(const std::string& method,
                           const std::string& params,
                           std::string& result);

    // Smart contract interaction
    std::string encode_swap_data(const Hash256& hash_lock,
                                const DilithiumPubKey& recipient,
                                uint32_t timelock);
    bool watch_contract_events();

    // Monitoring
    void monitor_swaps();
    void monitor_ethereum_chain();
};

/**
 * Bridge manager
 *
 * Manages multiple bridges to different blockchains
 */
class BridgeManager {
public:
    BridgeManager(Blockchain* intcoin_chain);
    ~BridgeManager();

    // Bridge management
    bool add_bridge(ChainType chain, std::shared_ptr<Bridge> bridge);
    std::shared_ptr<Bridge> get_bridge(ChainType chain);
    void remove_bridge(ChainType chain);

    // Start/stop all bridges
    bool start_all();
    void stop_all();

    // Swap operations across bridges
    Hash256 create_cross_chain_swap(ChainType target_chain,
                                     const DilithiumPubKey& recipient,
                                     uint64_t amount);

    bool complete_cross_chain_swap(ChainType source_chain,
                                    const Hash256& swap_id,
                                    const Hash256& secret);

    // Query operations
    std::vector<ChainType> get_available_chains() const;
    std::vector<std::shared_ptr<Bridge>> get_all_bridges() const;
    std::vector<std::shared_ptr<Bridge>> get_online_bridges() const;

    // Combined statistics
    struct AllBridgeStats {
        std::unordered_map<ChainType, BridgeStats> per_chain_stats;
        size_t total_bridges;
        size_t online_bridges;
        size_t total_swaps;
        uint64_t total_volume;

        AllBridgeStats() : total_bridges(0), online_bridges(0),
                          total_swaps(0), total_volume(0) {}
    };

    AllBridgeStats get_all_stats() const;

    // Monitoring
    void monitor_all_bridges();
    std::vector<Hash256> get_pending_swaps() const;

private:
    Blockchain* intcoin_chain_;
    std::unordered_map<ChainType, std::shared_ptr<Bridge>> bridges_;
    mutable std::mutex bridges_mutex_;
};

/**
 * Bridge utilities
 */
class BridgeUtils {
public:
    // Chain name conversion
    static std::string chain_type_to_string(ChainType chain);
    static std::optional<ChainType> string_to_chain_type(const std::string& str);

    // Address conversion
    static std::string intcoin_to_bitcoin_address(const DilithiumPubKey& key);
    static std::string intcoin_to_ethereum_address(const DilithiumPubKey& key);

    // Amount conversion (accounting for different decimals)
    static uint64_t intcoin_to_satoshi(uint64_t intcoin_amount);
    static uint64_t satoshi_to_intcoin(uint64_t satoshi_amount);
    static uint64_t intcoin_to_wei(uint64_t intcoin_amount);
    static uint64_t wei_to_intcoin(uint64_t wei_amount);

    // Fee estimation
    static uint64_t estimate_swap_fee(ChainType chain, uint64_t amount);
    static uint32_t get_recommended_confirmations(ChainType chain);

    // Timelock calculation
    static uint32_t calculate_safe_timelock(ChainType chain);
};

} // namespace bridge
} // namespace intcoin

#endif // INTCOIN_BRIDGE_H
