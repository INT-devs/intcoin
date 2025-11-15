// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Channel Splicing Implementation
// Dynamic channel capacity adjustments without closing
//
// Splicing allows adding (splice-in) or removing (splice-out) funds from
// an existing Lightning channel without closing and reopening, maintaining
// channel history and minimizing downtime.

#ifndef INTCOIN_SPLICING_H
#define INTCOIN_SPLICING_H

#include "lightning.h"
#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <optional>

namespace intcoin {
namespace splicing {

/**
 * Splicing protocol version
 */
static constexpr uint32_t SPLICING_VERSION = 1;

/**
 * Minimum splice amount (in satoshis)
 */
static constexpr uint64_t MIN_SPLICE_AMOUNT = 10000;

/**
 * Splice type
 */
enum class SpliceType {
    SPLICE_IN,      // Add funds to channel
    SPLICE_OUT      // Remove funds from channel
};

/**
 * Splice state
 */
enum class SpliceState {
    PROPOSED,       // Splice proposed
    ACCEPTED,       // Splice accepted by peer
    SIGNED,         // Splice transaction signed
    BROADCAST,      // Splice transaction broadcast
    CONFIRMED,      // Splice confirmed on-chain
    ACTIVE,         // Splice active, channel updated
    FAILED          // Splice failed
};

/**
 * Splice operation
 */
struct SpliceOperation {
    Hash256 splice_id;                  // Unique splice identifier
    Hash256 channel_id;                 // Channel being spliced

    SpliceType type;                    // Splice type (in/out)
    SpliceState state;                  // Current state

    uint64_t amount_sat;                // Amount being spliced
    uint64_t fee_sat;                   // On-chain fee

    // Old channel state
    uint64_t old_local_balance_sat;
    uint64_t old_remote_balance_sat;
    uint64_t old_capacity_sat;

    // New channel state
    uint64_t new_local_balance_sat;
    uint64_t new_remote_balance_sat;
    uint64_t new_capacity_sat;

    // Splice transaction
    Transaction splice_tx;               // On-chain splice transaction
    uint32_t confirmation_height;       // Block height of confirmation

    uint32_t created_at;                // Block height when created
    uint32_t completed_at;              // Block height when completed

    SpliceOperation()
        : type(SpliceType::SPLICE_IN),
          state(SpliceState::PROPOSED),
          amount_sat(0), fee_sat(0),
          old_local_balance_sat(0), old_remote_balance_sat(0), old_capacity_sat(0),
          new_local_balance_sat(0), new_remote_balance_sat(0), new_capacity_sat(0),
          confirmation_height(0),
          created_at(0), completed_at(0) {}

    std::vector<uint8_t> serialize() const;
    static SpliceOperation deserialize(const std::vector<uint8_t>& data);
};

/**
 * Splice manager
 * Manages channel splicing operations
 */
class SpliceManager {
public:
    SpliceManager();
    ~SpliceManager() = default;

    // ========================================================================
    // Splice-In (Add Funds)
    // ========================================================================

    /**
     * Initiate splice-in operation
     * Adds funds to an existing channel
     *
     * @param channel_id Channel to splice
     * @param amount_sat Amount to add
     * @return Splice ID if successful
     */
    std::optional<Hash256> initiate_splice_in(
        const Hash256& channel_id,
        uint64_t amount_sat
    );

    // ========================================================================
    // Splice-Out (Remove Funds)
    // ========================================================================

    /**
     * Initiate splice-out operation
     * Removes funds from an existing channel
     *
     * @param channel_id Channel to splice
     * @param amount_sat Amount to remove
     * @param destination Destination address for removed funds
     * @return Splice ID if successful
     */
    std::optional<Hash256> initiate_splice_out(
        const Hash256& channel_id,
        uint64_t amount_sat,
        const Address& destination
    );

    // ========================================================================
    // Splice Management
    // ========================================================================

    /**
     * Accept splice proposal from peer
     */
    bool accept_splice(const Hash256& splice_id);

    /**
     * Sign splice transaction
     */
    bool sign_splice(const Hash256& splice_id, const DilithiumSignature& signature);

    /**
     * Broadcast splice transaction
     */
    bool broadcast_splice(const Hash256& splice_id);

    /**
     * Confirm splice (called when transaction confirms)
     */
    bool confirm_splice(const Hash256& splice_id, uint32_t confirmation_height);

    /**
     * Cancel splice
     */
    bool cancel_splice(const Hash256& splice_id);

    /**
     * Get splice details
     */
    std::optional<SpliceOperation> get_splice(const Hash256& splice_id) const;

    /**
     * List all splices
     */
    std::vector<SpliceOperation> list_splices() const;

    /**
     * List splices for channel
     */
    std::vector<SpliceOperation> list_channel_splices(const Hash256& channel_id) const;

    struct SpliceStats {
        size_t total_splices;
        size_t successful_splices;
        size_t failed_splices;
        uint64_t total_spliced_in_sat;
        uint64_t total_spliced_out_sat;
        uint64_t total_fees_sat;
    };

    SpliceStats get_stats() const;

private:
    std::map<Hash256, SpliceOperation> splices_;
    uint32_t current_height_;
    mutable std::mutex mutex_;

    Hash256 generate_splice_id() const;
    Transaction create_splice_transaction(const SpliceOperation& splice) const;
    bool validate_splice_amount(const Hash256& channel_id, uint64_t amount, SpliceType type) const;
};

} // namespace splicing
} // namespace intcoin

#endif // INTCOIN_SPLICING_H
