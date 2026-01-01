// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/blockchain_monitor.h>

namespace intcoin {
namespace blockchain_monitor {

std::unique_ptr<BlockchainMonitor> CreateBlockchainMonitor(
    BlockchainType type,
    const std::string& rpc_url,
    const std::string& rpc_user,
    const std::string& rpc_password) {

    switch (type) {
        case BlockchainType::BITCOIN:
            return std::make_unique<BitcoinMonitor>(rpc_url, rpc_user, rpc_password, false);

        case BlockchainType::TESTNET_BTC:
            return std::make_unique<BitcoinMonitor>(rpc_url, rpc_user, rpc_password, true);

        case BlockchainType::LITECOIN:
            return std::make_unique<LitecoinMonitor>(rpc_url, rpc_user, rpc_password, false);

        case BlockchainType::TESTNET_LTC:
            return std::make_unique<LitecoinMonitor>(rpc_url, rpc_user, rpc_password, true);

        case BlockchainType::INTCOIN:
        case BlockchainType::TESTNET_INT:
            // INTcoin monitoring would use local blockchain state
            // Not implemented via RPC client
            return nullptr;

        default:
            return nullptr;
    }
}

}  // namespace blockchain_monitor
}  // namespace intcoin
