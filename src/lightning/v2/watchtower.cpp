// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/watchtower.h>
#include <sstream>
#include <algorithm>
#include <map>

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

    Impl() : config_() {}
    explicit Impl(const Config& config) : config_(config) {}
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
    (void)block_height;
    pimpl_->stats_.blocks_monitored++;
    return 0;  // No breaches detected (stub)
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
