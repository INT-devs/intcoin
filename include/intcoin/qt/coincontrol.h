// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_COINCONTROL_H
#define INTCOIN_QT_COINCONTROL_H

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <memory>

namespace intcoin {
namespace qt {

/**
 * Coin/UTXO representation
 */
struct CoinEntry {
    std::string txid;
    uint32_t vout{0};
    uint64_t amount{0};              // Amount in satoshis
    std::string address;
    uint32_t confirmations{0};
    uint32_t height{0};              // Block height
    bool is_coinbase{false};
    bool is_frozen{false};           // Manually frozen
    double privacy_score{0.0};       // 0.0 (low) to 1.0 (high)
    uint64_t timestamp{0};           // Creation timestamp
};

/**
 * Coin selection strategy
 */
enum class CoinSelectionStrategy {
    AUTOMATIC,                       // Automatic optimal selection
    MANUAL,                          // User manual selection
    MINIMIZE_FEE,                    // Minimize transaction fee
    MAXIMIZE_PRIVACY,                // Maximize privacy score
    OLDEST_FIRST,                    // Spend oldest coins first
    LARGEST_FIRST,                   // Spend largest coins first
    SMALLEST_FIRST                   // Spend smallest coins first (reduce UTXO set)
};

/**
 * Coin selection result
 */
struct CoinSelectionResult {
    std::vector<CoinEntry> selected_coins;
    uint64_t total_amount{0};
    uint64_t change_amount{0};
    uint64_t fee_amount{0};
    double avg_privacy_score{0.0};
    std::string error_message;
};

/**
 * Coin Control Settings
 */
struct CoinControlSettings {
    CoinSelectionStrategy strategy{CoinSelectionStrategy::AUTOMATIC};
    bool use_frozen_coins{false};    // Include frozen coins
    uint32_t min_confirmations{1};   // Minimum confirmations required
    double min_privacy_score{0.0};   // Minimum privacy score
    bool avoid_partial_spends{true}; // Avoid creating change
    uint64_t custom_fee_rate{0};     // Custom fee rate (0 = auto)
};

/**
 * Coin Control Manager
 *
 * Advanced UTXO management for the desktop wallet.
 * Supports manual coin selection, freezing, privacy scoring,
 * and multiple selection strategies.
 */
class CoinControl {
public:
    CoinControl();
    ~CoinControl();

    /**
     * Get all available coins
     *
     * @param include_frozen Include frozen coins
     * @return Vector of available coins
     */
    std::vector<CoinEntry> GetAvailableCoins(bool include_frozen = false) const;

    /**
     * Select coins for transaction
     *
     * @param target_amount Amount to send
     * @param settings Selection settings
     * @return Selection result
     */
    CoinSelectionResult SelectCoins(
        uint64_t target_amount,
        const CoinControlSettings& settings
    ) const;

    /**
     * Manually select specific coins
     *
     * @param coins Vector of coin entries to use
     * @return True if selection is valid
     */
    bool SetManualSelection(const std::vector<CoinEntry>& coins);

    /**
     * Get manually selected coins
     */
    std::vector<CoinEntry> GetManualSelection() const;

    /**
     * Clear manual selection
     */
    void ClearManualSelection();

    /**
     * Freeze coin (prevent spending)
     *
     * @param txid Transaction ID
     * @param vout Output index
     * @return True if frozen successfully
     */
    bool FreezeCoin(const std::string& txid, uint32_t vout);

    /**
     * Unfreeze coin
     *
     * @param txid Transaction ID
     * @param vout Output index
     * @return True if unfrozen successfully
     */
    bool UnfreezeCoin(const std::string& txid, uint32_t vout);

    /**
     * Check if coin is frozen
     */
    bool IsCoinFrozen(const std::string& txid, uint32_t vout) const;

    /**
     * Get all frozen coins
     */
    std::vector<CoinEntry> GetFrozenCoins() const;

    /**
     * Calculate privacy score for coin
     *
     * @param coin Coin to analyze
     * @return Privacy score (0.0-1.0)
     */
    double CalculatePrivacyScore(const CoinEntry& coin) const;

    /**
     * Update privacy scores for all coins
     */
    void UpdatePrivacyScores();

    /**
     * Get coin by outpoint
     *
     * @param txid Transaction ID
     * @param vout Output index
     * @return Coin entry (empty if not found)
     */
    CoinEntry GetCoin(const std::string& txid, uint32_t vout) const;

    /**
     * Refresh coin list from wallet
     */
    void Refresh();

    /**
     * Get total available balance
     *
     * @param include_frozen Include frozen coins
     * @return Total balance in satoshis
     */
    uint64_t GetTotalBalance(bool include_frozen = false) const;

    /**
     * Get number of available coins
     */
    size_t GetCoinCount(bool include_frozen = false) const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_COINCONTROL_H
