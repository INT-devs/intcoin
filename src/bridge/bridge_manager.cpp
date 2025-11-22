// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/bridge/bridge.h"
#include "../../include/intcoin/crypto/hash.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace bridge {

//==============================================================================
// BridgeManager Implementation
//==============================================================================

BridgeManager::BridgeManager(Blockchain* intcoin_chain)
    : intcoin_chain_(intcoin_chain) {
    if (!intcoin_chain_) {
        throw std::runtime_error("BridgeManager: intcoin_chain cannot be null");
    }
}

BridgeManager::~BridgeManager() {
    stop_all();
}

bool BridgeManager::add_bridge(ChainType chain, std::shared_ptr<Bridge> bridge) {
    if (!bridge) {
        return false;
    }

    std::lock_guard<std::mutex> lock(bridges_mutex_);

    // Check if bridge already exists
    if (bridges_.find(chain) != bridges_.end()) {
        return false;
    }

    // Add bridge
    bridges_[chain] = bridge;
    return true;
}

std::shared_ptr<Bridge> BridgeManager::get_bridge(ChainType chain) {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    auto it = bridges_.find(chain);
    if (it == bridges_.end()) {
        return nullptr;
    }

    return it->second;
}

void BridgeManager::remove_bridge(ChainType chain) {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    auto it = bridges_.find(chain);
    if (it != bridges_.end()) {
        // Stop bridge if running
        if (it->second->is_running()) {
            it->second->stop();
        }
        bridges_.erase(it);
    }
}

bool BridgeManager::start_all() {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    bool all_started = true;
    for (auto& [chain, bridge] : bridges_) {
        if (!bridge->is_running()) {
            if (!bridge->start()) {
                all_started = false;
            }
        }
    }

    return all_started;
}

void BridgeManager::stop_all() {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    for (auto& [chain, bridge] : bridges_) {
        if (bridge->is_running()) {
            bridge->stop();
        }
    }
}

Hash256 BridgeManager::create_cross_chain_swap(ChainType target_chain,
                                                const PublicKey& recipient,
                                                uint64_t amount) {
    auto bridge = get_bridge(target_chain);
    if (!bridge) {
        throw std::runtime_error("No bridge available for target chain");
    }

    if (!bridge->is_running()) {
        throw std::runtime_error("Bridge is not running");
    }

    if (bridge->get_status() != BridgeStatus::ONLINE) {
        throw std::runtime_error("Bridge is not online");
    }

    // Initiate swap on target chain bridge
    return bridge->initiate_swap(recipient, amount);
}

bool BridgeManager::complete_cross_chain_swap(ChainType source_chain,
                                               const Hash256& swap_id,
                                               const Hash256& secret) {
    auto bridge = get_bridge(source_chain);
    if (!bridge) {
        return false;
    }

    if (!bridge->is_running()) {
        return false;
    }

    // Complete swap on source chain bridge
    return bridge->complete_swap(swap_id, secret);
}

std::vector<ChainType> BridgeManager::get_available_chains() const {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    std::vector<ChainType> chains;
    chains.reserve(bridges_.size());

    for (const auto& [chain, bridge] : bridges_) {
        chains.push_back(chain);
    }

    return chains;
}

std::vector<std::shared_ptr<Bridge>> BridgeManager::get_all_bridges() const {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    std::vector<std::shared_ptr<Bridge>> all_bridges;
    all_bridges.reserve(bridges_.size());

    for (const auto& [chain, bridge] : bridges_) {
        all_bridges.push_back(bridge);
    }

    return all_bridges;
}

std::vector<std::shared_ptr<Bridge>> BridgeManager::get_online_bridges() const {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    std::vector<std::shared_ptr<Bridge>> online_bridges;

    for (const auto& [chain, bridge] : bridges_) {
        if (bridge->is_running() && bridge->get_status() == BridgeStatus::ONLINE) {
            online_bridges.push_back(bridge);
        }
    }

    return online_bridges;
}

BridgeManager::AllBridgeStats BridgeManager::get_all_stats() const {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    AllBridgeStats all_stats;
    all_stats.total_bridges = bridges_.size();

    for (const auto& [chain, bridge] : bridges_) {
        // Get stats for this bridge
        BridgeStats stats = bridge->get_stats();
        all_stats.per_chain_stats[chain] = stats;

        // Aggregate totals
        all_stats.total_swaps += stats.total_swaps;
        all_stats.total_volume += stats.total_volume_sent + stats.total_volume_received;

        // Count online bridges
        if (bridge->is_running() && bridge->get_status() == BridgeStatus::ONLINE) {
            all_stats.online_bridges++;
        }
    }

    return all_stats;
}

void BridgeManager::monitor_all_bridges() {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    for (auto& [chain, bridge] : bridges_) {
        if (bridge->is_running()) {
            // Trigger chain synchronization
            bridge->sync_chain();
        }
    }
}

std::vector<Hash256> BridgeManager::get_pending_swaps() const {
    std::lock_guard<std::mutex> lock(bridges_mutex_);

    std::vector<Hash256> pending_swaps;

    // This would typically query each bridge for pending swaps
    // For now, return empty vector - implementation depends on
    // how swaps are tracked in each bridge

    return pending_swaps;
}

//==============================================================================
// BridgeUtils Implementation
//==============================================================================

std::string BridgeUtils::chain_type_to_string(ChainType chain) {
    switch (chain) {
        case ChainType::BITCOIN:
            return "Bitcoin";
        case ChainType::ETHEREUM:
            return "Ethereum";
        case ChainType::LITECOIN:
            return "Litecoin";
        case ChainType::MONERO:
            return "Monero";
        case ChainType::INTCOIN:
            return "INTcoin";
        default:
            return "Unknown";
    }
}

std::optional<ChainType> BridgeUtils::string_to_chain_type(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);

    if (lower_str == "bitcoin" || lower_str == "btc") {
        return ChainType::BITCOIN;
    } else if (lower_str == "ethereum" || lower_str == "eth") {
        return ChainType::ETHEREUM;
    } else if (lower_str == "litecoin" || lower_str == "ltc") {
        return ChainType::LITECOIN;
    } else if (lower_str == "monero" || lower_str == "xmr") {
        return ChainType::MONERO;
    } else if (lower_str == "intcoin" || lower_str == "int") {
        return ChainType::INTCOIN;
    }

    return std::nullopt;
}

std::string BridgeUtils::intcoin_to_bitcoin_address(const PublicKey& key) {
    // Convert INTcoin public key to Bitcoin P2PKH address
    // This is a simplified conversion - full implementation would include:
    // 1. SHA-256 of public key
    // 2. RIPEMD-160 of hash
    // 3. Add version byte (0x00 for mainnet)
    // 4. Double SHA-256 checksum
    // 5. Base58 encode

    crypto::SHA256 hasher;
    hasher.update(key.data(), key.size());

    Hash256 pubkey_hash;
    hasher.finalize(pubkey_hash.data());

    // Placeholder: In production, implement full Base58Check encoding
    std::ostringstream oss;
    oss << "1";  // Bitcoin address prefix
    for (size_t i = 0; i < 20 && i < pubkey_hash.size(); ++i) {
        oss << std::hex << static_cast<int>(pubkey_hash[i]);
    }

    return oss.str();
}

std::string BridgeUtils::intcoin_to_ethereum_address(const PublicKey& key) {
    // Convert INTcoin public key to Ethereum address
    // Ethereum uses Keccak-256 of public key, take last 20 bytes
    // Prefix with 0x

    crypto::SHA256 hasher;  // In production, use Keccak-256
    hasher.update(key.data(), key.size());

    Hash256 hash;
    hasher.finalize(hash.data());

    // Take last 20 bytes and format as hex with 0x prefix
    std::ostringstream oss;
    oss << "0x";
    for (size_t i = hash.size() - 20; i < hash.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }

    return oss.str();
}

uint64_t BridgeUtils::intcoin_to_satoshi(uint64_t intcoin_amount) {
    // INTcoin uses 8 decimals, Bitcoin uses 8 decimals (satoshi)
    // 1 INT = 100000000 base units
    // 1 BTC = 100000000 satoshis
    // Direct 1:1 conversion for base units
    return intcoin_amount;
}

uint64_t BridgeUtils::satoshi_to_intcoin(uint64_t satoshi_amount) {
    // 1:1 conversion for base units
    return satoshi_amount;
}

uint64_t BridgeUtils::intcoin_to_wei(uint64_t intcoin_amount) {
    // INTcoin uses 8 decimals
    // Ethereum uses 18 decimals (wei)
    // 1 INT = 100000000 base units
    // 1 ETH = 1000000000000000000 wei
    // Conversion: multiply by 10^10
    return intcoin_amount * 10000000000ULL;
}

uint64_t BridgeUtils::wei_to_intcoin(uint64_t wei_amount) {
    // Convert wei to intcoin base units
    // Divide by 10^10
    return wei_amount / 10000000000ULL;
}

uint64_t BridgeUtils::estimate_swap_fee(ChainType chain, uint64_t amount) {
    // Estimate swap fee based on chain and amount
    // This is a simplified estimation - production would query real network fees

    switch (chain) {
        case ChainType::BITCOIN:
            // Bitcoin: typically 0.0001 BTC per transaction
            return 10000;  // 0.0001 BTC in satoshis

        case ChainType::ETHEREUM:
            // Ethereum: gas fees vary, estimate ~0.005 ETH
            return 5000000000000000ULL;  // 0.005 ETH in wei

        case ChainType::LITECOIN:
            // Litecoin: typically 0.001 LTC
            return 100000;  // 0.001 LTC in litoshis

        case ChainType::MONERO:
            // Monero: typically 0.0001 XMR
            return 100000000;  // 0.0001 XMR in atomic units

        default:
            // Default: 0.1% of amount
            return amount / 1000;
    }
}

uint32_t BridgeUtils::get_recommended_confirmations(ChainType chain) {
    // Recommended confirmations before considering transaction final

    switch (chain) {
        case ChainType::BITCOIN:
            return 6;  // Bitcoin: 6 confirmations (~1 hour)

        case ChainType::ETHEREUM:
            return 12;  // Ethereum: 12 confirmations (~3 minutes)

        case ChainType::LITECOIN:
            return 12;  // Litecoin: 12 confirmations (~30 minutes)

        case ChainType::MONERO:
            return 10;  // Monero: 10 confirmations (~20 minutes)

        case ChainType::INTCOIN:
            return 6;  // INTcoin: 6 confirmations

        default:
            return 6;
    }
}

uint32_t BridgeUtils::calculate_safe_timelock(ChainType chain) {
    // Calculate safe timelock duration in seconds
    // Should be enough time for confirmations + some buffer

    uint32_t confirmations = get_recommended_confirmations(chain);

    switch (chain) {
        case ChainType::BITCOIN:
            // Bitcoin: ~10 min per block
            return confirmations * 600 + 3600;  // +1 hour buffer

        case ChainType::ETHEREUM:
            // Ethereum: ~15 sec per block
            return confirmations * 15 + 300;  // +5 min buffer

        case ChainType::LITECOIN:
            // Litecoin: ~2.5 min per block
            return confirmations * 150 + 1800;  // +30 min buffer

        case ChainType::MONERO:
            // Monero: ~2 min per block
            return confirmations * 120 + 1200;  // +20 min buffer

        case ChainType::INTCOIN:
            // INTcoin: ~1 min per block
            return confirmations * 60 + 600;  // +10 min buffer

        default:
            return 3600;  // Default: 1 hour
    }
}

} // namespace bridge
} // namespace intcoin
