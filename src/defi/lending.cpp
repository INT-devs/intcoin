// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/defi/lending.h>
#include <algorithm>
#include <map>
#include <mutex>
#include <cmath>

namespace intcoin {
namespace defi {

class LendingProtocol::Impl {
public:
    std::map<std::string, LendingAsset> assets_;
    std::map<std::string, std::map<std::string, uint64_t>> user_supplied_;   // user -> asset -> amount
    std::map<std::string, std::map<std::string, uint64_t>> user_borrowed_;   // user -> asset -> amount
    std::map<std::string, std::vector<std::string>> user_collateral_;       // user -> collateral assets
    std::mutex mutex_;

    // Simple interest rate model
    double CalculateInterestRate(double utilization, bool is_borrow) const {
        // Base rate + utilization-based rate
        double base_rate = is_borrow ? 0.02 : 0.01;  // 2% borrow, 1% supply
        double slope = is_borrow ? 0.20 : 0.10;      // 20% max borrow, 10% max supply

        return base_rate + (utilization * slope);
    }
};

LendingProtocol::LendingProtocol()
    : pimpl_(std::make_unique<Impl>()) {}

LendingProtocol::~LendingProtocol() = default;

bool LendingProtocol::AddSupportedAsset(
    const std::string& asset_id,
    uint32_t collateral_factor,
    uint32_t liquidation_threshold,
    uint32_t liquidation_penalty
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->assets_.count(asset_id) > 0) {
        return false; // Already exists
    }

    LendingAsset asset;
    asset.asset_id = asset_id;
    asset.collateral_factor = collateral_factor;
    asset.liquidation_threshold = liquidation_threshold;
    asset.liquidation_penalty = liquidation_penalty;

    pimpl_->assets_[asset_id] = asset;
    return true;
}

LendingAsset LendingProtocol::GetAsset(const std::string& asset_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->assets_.count(asset_id) == 0) {
        return LendingAsset{};
    }

    return pimpl_->assets_.at(asset_id);
}

std::vector<LendingAsset> LendingProtocol::GetAllAssets() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::vector<LendingAsset> assets;
    for (const auto& [id, asset] : pimpl_->assets_) {
        assets.push_back(asset);
    }
    return assets;
}

bool LendingProtocol::Supply(
    const std::string& user_address,
    const std::string& asset_id,
    uint64_t amount
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->assets_.count(asset_id) == 0) {
        return false;
    }

    pimpl_->user_supplied_[user_address][asset_id] += amount;
    pimpl_->assets_[asset_id].total_supplied += amount;

    return true;
}

bool LendingProtocol::Withdraw(
    const std::string& user_address,
    const std::string& asset_id,
    uint64_t amount
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->user_supplied_[user_address][asset_id] < amount) {
        return false; // Insufficient balance
    }

    pimpl_->user_supplied_[user_address][asset_id] -= amount;
    pimpl_->assets_[asset_id].total_supplied -= amount;

    return true;
}

bool LendingProtocol::Borrow(
    const std::string& user_address,
    const std::string& asset_id,
    uint64_t amount
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->assets_.count(asset_id) == 0) {
        return false;
    }

    // Check if user has sufficient collateral
    // TODO: Implement proper collateral check with price oracle

    uint64_t available = pimpl_->assets_[asset_id].total_supplied -
                        pimpl_->assets_[asset_id].total_borrowed;

    if (amount > available) {
        return false; // Insufficient liquidity
    }

    pimpl_->user_borrowed_[user_address][asset_id] += amount;
    pimpl_->assets_[asset_id].total_borrowed += amount;

    return true;
}

bool LendingProtocol::Repay(
    const std::string& user_address,
    const std::string& asset_id,
    uint64_t amount
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->user_borrowed_[user_address][asset_id] < amount) {
        return false; // Repaying more than borrowed
    }

    pimpl_->user_borrowed_[user_address][asset_id] -= amount;
    pimpl_->assets_[asset_id].total_borrowed -= amount;

    return true;
}

bool LendingProtocol::EnableAssetAsCollateral(
    const std::string& user_address,
    const std::string& asset_id
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    auto& collateral = pimpl_->user_collateral_[user_address];
    if (std::find(collateral.begin(), collateral.end(), asset_id) == collateral.end()) {
        collateral.push_back(asset_id);
    }

    return true;
}

bool LendingProtocol::DisableAssetAsCollateral(
    const std::string& user_address,
    const std::string& asset_id
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    auto& collateral = pimpl_->user_collateral_[user_address];
    collateral.erase(std::remove(collateral.begin(), collateral.end(), asset_id), collateral.end());

    return true;
}

LendingPosition LendingProtocol::GetUserPosition(const std::string& user_address) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    LendingPosition position;
    position.user_address = user_address;

    // Sum up all supplied and borrowed amounts
    // TODO: Convert to USD using price oracle

    for (const auto& [asset_id, amount] : pimpl_->user_supplied_.at(user_address)) {
        position.supplied_amount += amount;
    }

    for (const auto& [asset_id, amount] : pimpl_->user_borrowed_.at(user_address)) {
        position.borrowed_amount += amount;
    }

    position.health_factor = CalculateHealthFactor(user_address);

    return position;
}

double LendingProtocol::CalculateHealthFactor(const std::string& user_address) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    // Health Factor = (Collateral * Liquidation Threshold) / Total Borrowed
    // TODO: Implement with price oracle

    uint64_t total_collateral = 0;
    uint64_t total_borrowed = 0;

    for (const auto& [asset_id, amount] : pimpl_->user_supplied_.at(user_address)) {
        total_collateral += amount;
    }

    for (const auto& [asset_id, amount] : pimpl_->user_borrowed_.at(user_address)) {
        total_borrowed += amount;
    }

    if (total_borrowed == 0) {
        return 999.0; // No debt, health is infinite
    }

    return static_cast<double>(total_collateral) / total_borrowed;
}

uint64_t LendingProtocol::GetMaxBorrowAmount(
    const std::string& user_address,
    const std::string& asset_id
) const {
    // TODO: Calculate based on collateral and collateral factor
    return 0;
}

std::vector<LiquidationOpportunity> LendingProtocol::GetLiquidatablePositions() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::vector<LiquidationOpportunity> opportunities;

    // Find positions with health factor < 1.0
    for (const auto& [user, borrowed_map] : pimpl_->user_borrowed_) {
        double health = CalculateHealthFactor(user);
        if (health < 1.0) {
            // TODO: Create liquidation opportunity
        }
    }

    return opportunities;
}

bool LendingProtocol::Liquidate(
    const std::string& liquidator_address,
    const std::string& borrower_address,
    const std::string& debt_asset,
    uint64_t debt_amount,
    const std::string& collateral_asset
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    // Check if position is liquidatable
    double health = CalculateHealthFactor(borrower_address);
    if (health >= 1.0) {
        return false; // Position is healthy
    }

    // TODO: Execute liquidation with penalty

    return false;
}

double LendingProtocol::CalculateSupplyAPY(const std::string& asset_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    double utilization = GetUtilizationRate(asset_id);
    return pimpl_->CalculateInterestRate(utilization, false);
}

double LendingProtocol::CalculateBorrowAPY(const std::string& asset_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    double utilization = GetUtilizationRate(asset_id);
    return pimpl_->CalculateInterestRate(utilization, true);
}

double LendingProtocol::GetUtilizationRate(const std::string& asset_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->assets_.count(asset_id) == 0) {
        return 0.0;
    }

    const auto& asset = pimpl_->assets_.at(asset_id);

    if (asset.total_supplied == 0) {
        return 0.0;
    }

    return static_cast<double>(asset.total_borrowed) / asset.total_supplied;
}

LendingProtocol::ProtocolStats LendingProtocol::GetProtocolStats() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    ProtocolStats stats;

    for (const auto& [id, asset] : pimpl_->assets_) {
        stats.total_value_locked += asset.total_supplied;
        stats.total_borrowed += asset.total_borrowed;
    }

    stats.num_active_users = pimpl_->user_supplied_.size();

    return stats;
}

} // namespace defi
} // namespace intcoin
