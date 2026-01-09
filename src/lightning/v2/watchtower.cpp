// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/watchtower.h>
#include <sstream>
#include <algorithm>
#include <map>
#include <chrono>
#include <cstring>
#include <vector>
#include <random>

namespace intcoin {
namespace lightning {
namespace v2 {

// Utility functions for enum conversion
std::string GetWatchtowerModeName(WatchtowerMode mode) {
    switch (mode) {
        case WatchtowerMode::ALTRUIST: return "ALTRUIST";
        case WatchtowerMode::COMMERCIAL: return "COMMERCIAL";
        default: return "UNKNOWN";
    }
}

WatchtowerMode ParseWatchtowerMode(const std::string& name) {
    if (name == "ALTRUIST") return WatchtowerMode::ALTRUIST;
    if (name == "COMMERCIAL") return WatchtowerMode::COMMERCIAL;
    return WatchtowerMode::ALTRUIST;
}

std::string GetSessionTypeName(SessionType type) {
    switch (type) {
        case SessionType::LEGACY: return "LEGACY";
        case SessionType::ANCHOR: return "ANCHOR";
        case SessionType::TAPROOT: return "TAPROOT";
        default: return "UNKNOWN";
    }
}

SessionType ParseSessionType(const std::string& name) {
    if (name == "LEGACY") return SessionType::LEGACY;
    if (name == "ANCHOR") return SessionType::ANCHOR;
    if (name == "TAPROOT") return SessionType::TAPROOT;
    return SessionType::ANCHOR;
}

std::string GetBreachStatusName(BreachStatus status) {
    switch (status) {
        case BreachStatus::MONITORING: return "MONITORING";
        case BreachStatus::BREACH_DETECTED: return "BREACH_DETECTED";
        case BreachStatus::PENALTY_BROADCAST: return "PENALTY_BROADCAST";
        case BreachStatus::PENALTY_CONFIRMED: return "PENALTY_CONFIRMED";
        case BreachStatus::EXPIRED: return "EXPIRED";
        default: return "UNKNOWN";
    }
}

BreachStatus ParseBreachStatus(const std::string& name) {
    if (name == "MONITORING") return BreachStatus::MONITORING;
    if (name == "BREACH_DETECTED") return BreachStatus::BREACH_DETECTED;
    if (name == "PENALTY_BROADCAST") return BreachStatus::PENALTY_BROADCAST;
    if (name == "PENALTY_CONFIRMED") return BreachStatus::PENALTY_CONFIRMED;
    if (name == "EXPIRED") return BreachStatus::EXPIRED;
    return BreachStatus::MONITORING;
}

//
// WatchtowerClient implementation
//

class WatchtowerClient::Impl {
public:
    std::map<std::string, WatchtowerSession> sessions_;
    std::vector<ChannelBackup> backups_;
    std::vector<BreachEvent> breaches_;
    Statistics stats_;
    bool enabled_{true};

    Impl() = default;
};

WatchtowerClient::WatchtowerClient()
    : pimpl_(std::make_unique<Impl>()) {}

WatchtowerClient::~WatchtowerClient() = default;

std::string WatchtowerClient::AddWatchtower(
    const std::string& tower_address,
    const std::string& tower_pubkey,
    WatchtowerMode mode
) {
    (void)tower_address;

    WatchtowerSession session;
    session.session_id = "session_" + std::to_string(pimpl_->sessions_.size());
    session.tower_pubkey = tower_pubkey;
    session.mode = mode;
    session.active = true;

    pimpl_->sessions_[session.session_id] = session;
    pimpl_->stats_.active_towers++;
    pimpl_->stats_.active_sessions++;

    return session.session_id;
}

bool WatchtowerClient::RemoveWatchtower(const std::string& session_id) {
    auto it = pimpl_->sessions_.find(session_id);
    if (it != pimpl_->sessions_.end()) {
        pimpl_->sessions_.erase(it);
        if (pimpl_->stats_.active_towers > 0) {
            pimpl_->stats_.active_towers--;
        }
        if (pimpl_->stats_.active_sessions > 0) {
            pimpl_->stats_.active_sessions--;
        }
        return true;
    }
    return false;
}

std::string WatchtowerClient::CreateSession(
    const std::string& tower_id,
    SessionType session_type,
    uint32_t max_updates
) {
    (void)tower_id;

    WatchtowerSession session;
    session.session_id = "session_" + std::to_string(pimpl_->sessions_.size());
    session.session_type = session_type;
    session.max_updates = max_updates;
    session.active = true;

    pimpl_->sessions_[session.session_id] = session;
    pimpl_->stats_.active_sessions++;

    return session.session_id;
}

bool WatchtowerClient::BackupChannelState(
    const std::string& channel_id,
    uint32_t commitment_number,
    const JusticeBlob& justice_blob
) {
    ChannelBackup backup;
    backup.channel_id = channel_id;
    backup.commitment_number = commitment_number;
    backup.justice_blob = justice_blob;

    pimpl_->backups_.push_back(backup);
    pimpl_->stats_.total_backups++;

    // Update backed_up_channels count (unique channels)
    bool is_new_channel = true;
    for (const auto& b : pimpl_->backups_) {
        if (b.channel_id == channel_id && &b != &backup) {
            is_new_channel = false;
            break;
        }
    }
    if (is_new_channel) {
        pimpl_->stats_.backed_up_channels++;
    }

    return true;
}

std::vector<WatchtowerSession> WatchtowerClient::GetActiveSessions() const {
    std::vector<WatchtowerSession> active;

    for (const auto& [id, session] : pimpl_->sessions_) {
        if (session.active) {
            active.push_back(session);
        }
    }

    return active;
}

WatchtowerSession WatchtowerClient::GetSession(const std::string& session_id) const {
    auto it = pimpl_->sessions_.find(session_id);
    if (it != pimpl_->sessions_.end()) {
        return it->second;
    }
    return WatchtowerSession{};
}

std::vector<ChannelBackup> WatchtowerClient::GetChannelBackups(
    const std::string& channel_id
) const {
    if (channel_id.empty()) {
        return pimpl_->backups_;
    }

    std::vector<ChannelBackup> filtered;
    for (const auto& backup : pimpl_->backups_) {
        if (backup.channel_id == channel_id) {
            filtered.push_back(backup);
        }
    }

    return filtered;
}

std::vector<BreachEvent> WatchtowerClient::GetBreachEvents() const {
    return pimpl_->breaches_;
}

WatchtowerClient::Statistics WatchtowerClient::GetStatistics() const {
    return pimpl_->stats_;
}

void WatchtowerClient::SetEnabled(bool enabled) {
    pimpl_->enabled_ = enabled;
}

bool WatchtowerClient::IsEnabled() const {
    return pimpl_->enabled_;
}

//
// WatchtowerServer implementation
//

class WatchtowerServer::Impl {
public:
    Config config_;
    std::map<std::string, WatchtowerSession> sessions_;
    std::vector<BreachEvent> breaches_;
    std::map<std::string, std::vector<JusticeBlob>> justice_blobs_;
    Statistics stats_;
    bool running_{false};
    uint32_t last_block_height_{0};

    Impl() : config_() {}
    explicit Impl(const Config& config) : config_(config) {}

    // Helper: Check if transaction matches breach hint
    bool MatchesBreachHint(const std::vector<uint8_t>& tx_data, const std::vector<uint8_t>& breach_hint) {
        if (breach_hint.size() != 16 || tx_data.size() < 16) {
            return false;
        }

        // Compare first 16 bytes of transaction with breach hint
        // In production, this would use proper transaction parsing and commitment point extraction
        for (size_t i = 0; i < 16 && i < tx_data.size(); i++) {
            if (tx_data[i] != breach_hint[i]) {
                return false;
            }
        }

        return true;
    }

    // Helper: Extract commitment number from transaction
    uint32_t ExtractCommitmentNumber(const std::vector<uint8_t>& tx_data) {
        // In production, this would parse the transaction's locktime/sequence fields
        // For now, simulate extraction from transaction data
        if (tx_data.size() < 8) {
            return 0;
        }

        uint32_t commitment_number = 0;
        for (size_t i = 0; i < 4 && i < tx_data.size(); i++) {
            commitment_number |= (static_cast<uint32_t>(tx_data[i]) << (i * 8));
        }

        return commitment_number % 1000000;  // Reasonable commitment number range
    }

    // Helper: Decrypt justice blob
    std::vector<uint8_t> DecryptJusticeBlob(const JusticeBlob& blob) {
        // In production, this would use the breach transaction as decryption key
        // For now, return the encrypted blob as-is (simulating decryption)
        return blob.encrypted_blob;
    }

    // Helper: Parse penalty amount from justice transaction
    uint64_t ExtractPenaltyAmount(const std::vector<uint8_t>& justice_tx) {
        // In production, this would parse the transaction outputs
        // For now, simulate a penalty amount
        if (justice_tx.size() < 16) {
            return 0;
        }

        uint64_t amount = 0;
        for (size_t i = 0; i < 8 && i + 8 < justice_tx.size(); i++) {
            amount |= (static_cast<uint64_t>(justice_tx[i + 8]) << (i * 8));
        }

        return amount % 100000000;  // Max ~1 BTC penalty
    }

    // Helper: Broadcast penalty transaction
    std::string BroadcastPenaltyTransaction(const std::vector<uint8_t>& penalty_tx) {
        // In production, this would broadcast to the network via RPC
        // For now, generate a simulated txid
        std::string txid = "penalty_";
        for (size_t i = 0; i < 32 && i < penalty_tx.size(); i++) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", penalty_tx[i]);
            txid += hex;
        }

        return txid;
    }

    // Helper: Generate simulated transaction data for testing
    std::vector<uint8_t> GenerateSimulatedTxData(uint32_t tx_index) {
        std::vector<uint8_t> tx_data(64);  // Simulated transaction
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);

        for (auto& byte : tx_data) {
            byte = dis(gen);
        }

        // Encode tx_index in first 4 bytes for deterministic testing
        for (size_t i = 0; i < 4; i++) {
            tx_data[i] = (tx_index >> (i * 8)) & 0xFF;
        }

        return tx_data;
    }

    // Helper: Simulate getting transactions from a block
    std::vector<std::vector<uint8_t>> GetBlockTransactions(uint32_t block_height) {
        (void)block_height;

        // In production, this would query the blockchain via RPC
        // For now, return empty vector (no transactions to check)
        // This can be expanded later when integrated with actual blockchain data
        std::vector<std::vector<uint8_t>> transactions;

        return transactions;
    }

    // Helper: Create breach event record
    BreachEvent CreateBreachEvent(
        const std::string& channel_id,
        const std::string& breach_txid,
        uint32_t commitment_number,
        uint32_t block_height
    ) {
        BreachEvent event;
        event.channel_id = channel_id;
        event.breach_txid = breach_txid;
        event.commitment_number = commitment_number;
        event.status = BreachStatus::BREACH_DETECTED;
        event.block_height = block_height;
        event.detected_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

        return event;
    }

    // Process pending breach events - check confirmations
    uint32_t ProcessPendingBreaches(uint32_t current_block_height) {
        uint32_t confirmed_count = 0;

        for (auto& breach : breaches_) {
            if (breach.status == BreachStatus::PENALTY_BROADCAST) {
                uint32_t blocks_since_broadcast = current_block_height - breach.block_height;
                if (blocks_since_broadcast >= 6) {
                    breach.status = BreachStatus::PENALTY_CONFIRMED;
                    breach.resolved_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
                    confirmed_count++;
                }
            }
        }
        return confirmed_count;
    }

    // Clean up expired justice blobs
    uint32_t CleanupExpiredBlobs(uint64_t current_timestamp) {
        uint32_t removed_count = 0;
        std::vector<std::string> expired_sessions;

        for (const auto& [session_id, session] : sessions_) {
            if (session.expires_at > 0 && session.expires_at < current_timestamp) {
                expired_sessions.push_back(session_id);
            }
        }

        for (const auto& session_id : expired_sessions) {
            auto it = justice_blobs_.find(session_id);
            if (it != justice_blobs_.end()) {
                removed_count += static_cast<uint32_t>(it->second.size());
                justice_blobs_.erase(it);
            }

            // Mark breach events as expired
            for (auto& breach : breaches_) {
                if (breach.channel_id.find(session_id) != std::string::npos &&
                    breach.status == BreachStatus::MONITORING) {
                    breach.status = BreachStatus::EXPIRED;
                }
            }

            // Mark session as inactive
            auto session_it = sessions_.find(session_id);
            if (session_it != sessions_.end()) {
                session_it->second.active = false;
            }
        }
        return removed_count;
    }

    // Get breach events by status
    std::vector<BreachEvent> GetBreachEventsByStatus(BreachStatus status) const {
        std::vector<BreachEvent> filtered;
        for (const auto& breach : breaches_) {
            if (breach.status == status) {
                filtered.push_back(breach);
            }
        }
        return filtered;
    }

    // Get breach events by channel
    std::vector<BreachEvent> GetBreachEventsByChannel(const std::string& channel_id) const {
        std::vector<BreachEvent> filtered;
        for (const auto& breach : breaches_) {
            if (breach.channel_id == channel_id) {
                filtered.push_back(breach);
            }
        }
        return filtered;
    }
};

WatchtowerServer::WatchtowerServer()
    : pimpl_(std::make_unique<Impl>()) {}

WatchtowerServer::WatchtowerServer(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

WatchtowerServer::~WatchtowerServer() = default;

bool WatchtowerServer::Start() {
    pimpl_->running_ = true;
    return true;
}

void WatchtowerServer::Stop() {
    pimpl_->running_ = false;
}

bool WatchtowerServer::IsRunning() const {
    return pimpl_->running_;
}

uint32_t WatchtowerServer::ProcessBlock(uint32_t block_height) {
    if (!pimpl_->running_) {
        return 0;
    }

    pimpl_->stats_.blocks_monitored++;
    pimpl_->last_block_height_ = block_height;

    uint32_t breaches_detected = 0;

    // Get all transactions in the block
    auto block_transactions = pimpl_->GetBlockTransactions(block_height);

    // Check each transaction against all stored justice blobs
    for (const auto& tx_data : block_transactions) {
        // Generate a transaction ID for reference
        std::string tx_id = "tx_" + std::to_string(block_height) + "_" +
                           std::to_string(tx_data.size());

        // Check against all sessions' justice blobs
        for (const auto& [session_id, blobs] : pimpl_->justice_blobs_) {
            for (const auto& blob : blobs) {
                // Check if transaction matches breach hint
                if (pimpl_->MatchesBreachHint(tx_data, blob.breach_hint)) {
                    // BREACH DETECTED!
                    breaches_detected++;

                    // Extract commitment number from breach transaction
                    uint32_t commitment_number = pimpl_->ExtractCommitmentNumber(tx_data);

                    // Create breach event
                    std::string channel_id = "channel_" + session_id;
                    BreachEvent event = pimpl_->CreateBreachEvent(
                        channel_id,
                        tx_id,
                        commitment_number,
                        block_height
                    );

                    // Decrypt justice blob to get penalty transaction
                    std::vector<uint8_t> penalty_tx = pimpl_->DecryptJusticeBlob(blob);

                    if (!penalty_tx.empty()) {
                        // Extract penalty amount
                        uint64_t penalty_amount = pimpl_->ExtractPenaltyAmount(penalty_tx);
                        event.penalty_amount = penalty_amount;

                        // Broadcast penalty transaction
                        std::string penalty_txid = pimpl_->BroadcastPenaltyTransaction(penalty_tx);
                        event.penalty_txid = penalty_txid;
                        event.status = BreachStatus::PENALTY_BROADCAST;

                        // Update statistics
                        pimpl_->stats_.penalties_broadcast++;
                        pimpl_->stats_.total_rewards_earned += penalty_amount;

                        // Calculate tower reward (for commercial mode)
                        auto session_it = pimpl_->sessions_.find(session_id);
                        if (session_it != pimpl_->sessions_.end()) {
                            const auto& session = session_it->second;
                            if (session.mode == WatchtowerMode::COMMERCIAL) {
                                uint64_t reward = session.reward_base +
                                    (penalty_amount * session.reward_rate) / 1000000;
                                pimpl_->stats_.total_rewards_earned += reward;
                            }
                        }
                    } else {
                        // Failed to decrypt/process justice blob
                        event.status = BreachStatus::BREACH_DETECTED;
                        event.error_message = "Failed to decrypt justice blob";
                    }

                    // Store breach event
                    pimpl_->breaches_.push_back(event);
                    pimpl_->stats_.breaches_detected++;

                    // Note: In production, we might want to remove the used justice blob
                    // to save storage, but keeping it for audit trail
                }
            }
        }
    }

    return breaches_detected;
}

std::string WatchtowerServer::CreateClientSession(
    const std::string& client_pubkey,
    SessionType session_type,
    uint32_t max_updates
) {
    WatchtowerSession session;
    session.session_id = "server_session_" + std::to_string(pimpl_->sessions_.size());
    session.tower_pubkey = client_pubkey;
    session.session_type = session_type;
    session.max_updates = max_updates;
    session.mode = pimpl_->config_.mode;
    session.reward_base = pimpl_->config_.reward_base;
    session.reward_rate = pimpl_->config_.reward_rate;
    session.sweep_fee_rate = pimpl_->config_.sweep_fee_rate;
    session.active = true;

    pimpl_->sessions_[session.session_id] = session;
    pimpl_->stats_.active_sessions++;

    return session.session_id;
}

bool WatchtowerServer::StoreJusticeBlob(
    const std::string& session_id,
    const JusticeBlob& blob
) {
    auto it = pimpl_->sessions_.find(session_id);
    if (it != pimpl_->sessions_.end()) {
        pimpl_->justice_blobs_[session_id].push_back(blob);
        pimpl_->stats_.total_blobs_stored++;
        return true;
    }
    return false;
}

std::vector<WatchtowerSession> WatchtowerServer::GetActiveSessions() const {
    std::vector<WatchtowerSession> active;

    for (const auto& [id, session] : pimpl_->sessions_) {
        if (session.active) {
            active.push_back(session);
        }
    }

    return active;
}

std::vector<BreachEvent> WatchtowerServer::GetBreachEvents() const {
    return pimpl_->breaches_;
}

WatchtowerServer::Config WatchtowerServer::GetConfig() const {
    return pimpl_->config_;
}

void WatchtowerServer::SetConfig(const Config& config) {
    pimpl_->config_ = config;
}

WatchtowerServer::Statistics WatchtowerServer::GetStatistics() const {
    return pimpl_->stats_;
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
