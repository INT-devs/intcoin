// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_DEFI_LENDING_H
#define INTCOIN_DEFI_LENDING_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace intcoin {
namespace defi {

/**
 * @brief Decentralized Lending Protocol
 *
 * Implements over-collateralized lending similar to Aave/Compound.
 * Users can deposit assets to earn interest or borrow against collateral.
 */

// Supported asset configuration
struct LendingAsset {
    std::string asset_id;
    uint64_t total_supplied{0};       // Total supplied to protocol
    uint64_t total_borrowed{0};       // Total borrowed from protocol
    double supply_apy{0.0};           // Supply interest rate
    double borrow_apy{0.0};           // Borrow interest rate
    uint32_t collateral_factor{0};   // Max borrow ratio (basis points, 7500 = 75%)
    uint32_t liquidation_threshold{0}; // Liquidation trigger (basis points, 8000 = 80%)
    uint32_t liquidation_penalty{0};  // Liquidation penalty (basis points, 500 = 5%)
};

// User position in lending protocol
struct LendingPosition {
    std::string user_address;
    uint64_t supplied_amount{0};
    uint64_t borrowed_amount{0};
    uint64_t collateral_value{0};     // USD value of collateral
    double health_factor{0.0};        // Health factor (>1.0 is safe, <1.0 liquidatable)
    uint64_t accrued_interest{0};     // Interest earned or owed
};

// Liquidation opportunity
struct LiquidationOpportunity {
    std::string user_address;
    std::string asset_id;
    uint64_t debt_amount{0};
    uint64_t collateral_amount{0};
    double health_factor{0.0};
    uint64_t profit_estimate{0};      // Estimated profit from liquidation
};

/**
 * @class LendingProtocol
 * @brief Implements decentralized over-collateralized lending
 */
class LendingProtocol {
public:
    LendingProtocol();
    ~LendingProtocol();

    // Asset management
    bool AddSupportedAsset(
        const std::string& asset_id,
        uint32_t collateral_factor,
        uint32_t liquidation_threshold,
        uint32_t liquidation_penalty
    );

    LendingAsset GetAsset(const std::string& asset_id) const;
    std::vector<LendingAsset> GetAllAssets() const;

    // Supply (deposit and earn interest)
    bool Supply(const std::string& user_address, const std::string& asset_id, uint64_t amount);
    bool Withdraw(const std::string& user_address, const std::string& asset_id, uint64_t amount);

    // Borrow (must have sufficient collateral)
    bool Borrow(const std::string& user_address, const std::string& asset_id, uint64_t amount);
    bool Repay(const std::string& user_address, const std::string& asset_id, uint64_t amount);

    // Collateral management
    bool EnableAssetAsCollateral(const std::string& user_address, const std::string& asset_id);
    bool DisableAssetAsCollateral(const std::string& user_address, const std::string& asset_id);

    // Position queries
    LendingPosition GetUserPosition(const std::string& user_address) const;
    double CalculateHealthFactor(const std::string& user_address) const;
    uint64_t GetMaxBorrowAmount(const std::string& user_address, const std::string& asset_id) const;

    // Liquidations
    std::vector<LiquidationOpportunity> GetLiquidatablePositions() const;
    bool Liquidate(
        const std::string& liquidator_address,
        const std::string& borrower_address,
        const std::string& debt_asset,
        uint64_t debt_amount,
        const std::string& collateral_asset
    );

    // Interest rate model (utilization-based)
    double CalculateSupplyAPY(const std::string& asset_id) const;
    double CalculateBorrowAPY(const std::string& asset_id) const;
    double GetUtilizationRate(const std::string& asset_id) const;

    // Protocol statistics
    struct ProtocolStats {
        uint64_t total_value_locked{0};  // Total USD value locked
        uint64_t total_borrowed{0};      // Total USD borrowed
        uint64_t total_interest{0};      // Total interest paid
        uint32_t num_active_users{0};
    };

    ProtocolStats GetProtocolStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace defi
} // namespace intcoin

#endif // INTCOIN_DEFI_LENDING_H
