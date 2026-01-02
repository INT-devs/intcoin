// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/mobile_rpc.h>
#include <intcoin/util.h>

#include <algorithm>
#include <chrono>

namespace intcoin {
namespace mobile {

MobileRPC::MobileRPC(std::shared_ptr<SPVClient> spv_client,
                     std::shared_ptr<wallet::Wallet> wallet)
    : spv_client_(spv_client), wallet_(wallet) {

    LogF(LogLevel::INFO, "Mobile RPC: Initialized for INTcoin lightweight clients");
}

Result<SyncResponse> MobileRPC::Sync(const SyncRequest& request) {
    SyncResponse response;

    // Set bloom filter on SPV client
    spv_client_->SetBloomFilter(request.filter);

    // Get headers starting from last known block
    uint64_t start_height = 0;
    if (request.last_block_hash != uint256{}) {
        auto header_result = spv_client_->GetHeader(request.last_block_hash);
        if (header_result.IsOk()) {
            // Get height from header (need to track this)
            // For now, start from current height - max_headers
            start_height = (spv_client_->GetBestHeight() > request.max_headers) ?
                          (spv_client_->GetBestHeight() - request.max_headers) : 0;
        }
    }

    // Get headers
    uint64_t end_height = spv_client_->GetBestHeight();
    uint64_t num_headers = std::min(static_cast<uint64_t>(request.max_headers),
                                   end_height - start_height + 1);

    response.headers = spv_client_->GetHeadersInRange(start_height,
                                                      start_height + num_headers - 1);

    // Set network status
    response.best_height = spv_client_->GetBestHeight();
    response.best_hash = spv_client_->GetBestHash();

    // Estimate fee rate (default 1000 INTS per KB for now)
    // TODO: Integrate with mempool fee estimation
    response.fee_rate = 1000;

    // TODO: Get filtered transactions from mempool/blocks
    // This would require integration with full node for transaction data

    LogF(LogLevel::INFO, "Mobile RPC: Sync returned %zu headers (height %llu)",
         response.headers.size(), response.best_height);

    return Result<SyncResponse>::Ok(response);
}

Result<BalanceResponse> MobileRPC::GetBalance(const BalanceRequest& request) {
    BalanceResponse response;
    response.confirmed_balance = 0;
    response.unconfirmed_balance = 0;
    response.total_balance = 0;
    response.utxo_count = 0;

    // If wallet is available, use it
    if (wallet_) {
        uint64_t confirmed = 0;
        uint64_t unconfirmed = 0;

        // Get wallet balance
        // TODO: Integrate with wallet GetBalance method
        // For now, return placeholder

        response.confirmed_balance = confirmed;
        response.unconfirmed_balance = unconfirmed;
        response.total_balance = confirmed + unconfirmed;

        LogF(LogLevel::DEBUG, "Mobile RPC: Balance for %s: %llu INTS confirmed, %llu INTS unconfirmed",
             request.address.c_str(), confirmed, unconfirmed);
    } else {
        // No wallet available
        LogF(LogLevel::WARNING, "Mobile RPC: GetBalance called without wallet instance");
        return Result<BalanceResponse>::Error("Wallet not available");
    }

    return Result<BalanceResponse>::Ok(response);
}

Result<HistoryResponse> MobileRPC::GetHistory(const HistoryRequest& request) {
    HistoryResponse response;
    response.entries = {};
    response.total_count = 0;
    response.page = request.page;
    response.total_pages = 0;

    // TODO: Implement transaction history from indexed transactions
    // This requires integration with transaction indexing

    LogF(LogLevel::DEBUG, "Mobile RPC: GetHistory for %s (page %u)",
         request.address.c_str(), request.page);

    return Result<HistoryResponse>::Ok(response);
}

Result<SendTransactionResponse> MobileRPC::SendTransaction(const SendTransactionRequest& request) {
    SendTransactionResponse response;

    // Deserialize transaction
    auto tx_result = Transaction::Deserialize(request.raw_transaction);
    if (tx_result.IsError()) {
        response.accepted = false;
        response.error = "Failed to deserialize transaction: " + tx_result.error;
        return Result<SendTransactionResponse>::Ok(response);
    }

    Transaction tx = tx_result.GetValue();
    response.tx_hash = tx.GetHash();

    // TODO: Broadcast transaction to network
    // This requires integration with P2P network

    response.accepted = true;
    response.error = "";
    response.estimated_confirmation = 300;  // ~5 minutes (default block time)

    LogF(LogLevel::INFO, "Mobile RPC: Broadcasting transaction %s",
         BytesToHex(std::vector<uint8_t>(response.tx_hash.begin(),
                                         response.tx_hash.end())).substr(0, 16).c_str());

    return Result<SendTransactionResponse>::Ok(response);
}

Result<UTXOResponse> MobileRPC::GetUTXOs(const UTXORequest& request) {
    UTXOResponse response;
    response.utxos = {};
    response.total_amount = 0;

    // TODO: Implement UTXO retrieval from storage
    // This requires integration with UTXO set

    LogF(LogLevel::DEBUG, "Mobile RPC: GetUTXOs for %s (min conf: %u)",
         request.address.c_str(), request.min_confirmations);

    return Result<UTXOResponse>::Ok(response);
}

Result<FeeEstimateResponse> MobileRPC::EstimateFee(const FeeEstimateRequest& request) {
    FeeEstimateResponse response;

    // Fee estimation for INTcoin
    // Base fee rate depends on network congestion
    // For now, use simple estimation based on target blocks

    uint64_t base_fee_rate = 1000;  // 1000 INTS per KB (0.001 INT per KB)

    // Adjust based on confirmation target
    if (request.target_blocks <= 2) {
        // Fast confirmation: higher fee
        base_fee_rate = 5000;  // 0.005 INT per KB
        response.confidence = 0.95;
    } else if (request.target_blocks <= 6) {
        // Normal confirmation
        base_fee_rate = 2000;  // 0.002 INT per KB
        response.confidence = 0.90;
    } else {
        // Slow confirmation: lower fee
        base_fee_rate = 1000;  // 0.001 INT per KB
        response.confidence = 0.80;
    }

    response.fee_rate = base_fee_rate;

    // Calculate estimated fee for transaction
    // Fee = (tx_size / 1000) * fee_rate
    response.estimated_fee = (request.tx_size * base_fee_rate) / 1000;

    // Minimum fee: 1000 INTS (0.001 INT)
    if (response.estimated_fee < 1000) {
        response.estimated_fee = 1000;
    }

    LogF(LogLevel::DEBUG, "Mobile RPC: Fee estimate for %u blocks: %llu INTS/KB, %llu INTS for %u bytes",
         request.target_blocks, response.fee_rate, response.estimated_fee, request.tx_size);

    return Result<FeeEstimateResponse>::Ok(response);
}

Result<MobileRPC::NetworkStatus> MobileRPC::GetNetworkStatus() {
    NetworkStatus status;

    status.block_height = spv_client_->GetBestHeight();
    status.block_hash = spv_client_->GetBestHash();
    status.is_syncing = spv_client_->IsSyncing();
    status.sync_progress = spv_client_->GetSyncProgress();
    status.peer_count = 0;  // TODO: Get from network manager

    return Result<NetworkStatus>::Ok(status);
}

uint64_t MobileRPC::CalculateTransactionFee(const Transaction& tx) {
    // Calculate total input value
    uint64_t input_value = 0;
    // TODO: Get input values from UTXO set

    // Calculate total output value
    uint64_t output_value = 0;
    for (const auto& output : tx.outputs) {
        output_value += output.value;
    }

    // Fee = inputs - outputs
    if (input_value > output_value) {
        return input_value - output_value;
    }

    return 0;
}

uint32_t MobileRPC::GetConfirmations(uint64_t block_height) {
    uint64_t best_height = spv_client_->GetBestHeight();

    if (block_height == 0 || block_height > best_height) {
        return 0;  // Unconfirmed or invalid
    }

    return static_cast<uint32_t>(best_height - block_height + 1);
}

}  // namespace mobile
}  // namespace intcoin
