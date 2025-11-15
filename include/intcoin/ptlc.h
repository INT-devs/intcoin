// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Point Time Locked Contracts (PTLCs)
// Enhanced privacy using adaptor signatures
//
// PTLCs replace HTLCs (Hash Time Locked Contracts) with a more private
// alternative using adaptor signatures and elliptic curve points instead
// of hash preimages. This improves privacy by making payments indistinguishable
// from regular transactions and enables more advanced features like stuckless
// payments and payment decorrelation.

#ifndef INTCOIN_PTLC_H
#define INTCOIN_PTLC_H

#include "lightning.h"
#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <optional>

namespace intcoin {
namespace ptlc {

/**
 * PTLC protocol version
 */
static constexpr uint32_t PTLC_VERSION = 1;

/**
 * Adaptor signature
 * A signature that is valid only when combined with a secret scalar
 */
struct AdaptorSignature {
    DilithiumSignature partial_sig;     // Partial signature
    Hash256 adaptor_point;              // Public adaptor point (T = t*G)

    std::vector<uint8_t> serialize() const;
    static AdaptorSignature deserialize(const std::vector<uint8_t>& data);

    // Verify adaptor signature is valid for given point
    bool verify(
        const DilithiumPubKey& pubkey,
        const Hash256& message,
        const Hash256& adaptor_point
    ) const;
};

/**
 * Complete signature from adaptor
 * Created by adding the adaptor secret to the partial signature
 */
struct CompletedSignature {
    DilithiumSignature complete_sig;    // Complete signature
    Hash256 secret_scalar;              // Secret scalar (t) that was added

    std::vector<uint8_t> serialize() const;
    static CompletedSignature deserialize(const std::vector<uint8_t>& data);
};

/**
 * PTLC (Point Time Locked Contract)
 * Privacy-preserving payment contract using adaptor signatures
 */
struct PTLC {
    Hash256 ptlc_id;                    // Unique PTLC identifier

    // Payment details
    uint64_t amount_sat;                // Payment amount
    uint32_t timeout_height;            // Absolute timeout (block height)
    uint32_t timeout_seconds;           // Relative timeout (seconds)

    // Cryptographic commitment
    Hash256 payment_point;              // Payment point (P = p*G)
    // Secret scalar p is the "payment secret" (like preimage in HTLCs)

    // Adaptor signatures
    AdaptorSignature sender_adaptor;    // Sender's adaptor signature
    AdaptorSignature receiver_adaptor;  // Receiver's adaptor signature

    // State
    bool claimed;                       // Has payment been claimed?
    bool timed_out;                     // Has timeout been reached?

    std::optional<Hash256> payment_secret;  // Revealed secret (p) when claimed

    uint64_t created_at;                // Timestamp

    PTLC()
        : amount_sat(0),
          timeout_height(0), timeout_seconds(0),
          claimed(false), timed_out(false),
          created_at(0) {}

    std::vector<uint8_t> serialize() const;
    static PTLC deserialize(const std::vector<uint8_t>& data);
};

/**
 * PTLC state in a channel
 */
enum class PTLCState {
    PROPOSED,       // PTLC proposed but not added
    ACTIVE,         // PTLC active in channel
    CLAIMED,        // Payment claimed (secret revealed)
    FAILED,         // Payment failed (routing error)
    TIMED_OUT       // Payment timed out
};

/**
 * Channel PTLC
 * PTLC as it exists in a Lightning channel
 */
struct ChannelPTLC {
    Hash256 ptlc_id;                    // PTLC identifier
    PTLCState state;                    // Current state

    // Direction
    bool outgoing;                      // True if outgoing, false if incoming

    // Payment details
    uint64_t amount_sat;
    uint32_t timeout_height;

    // Cryptographic data
    Hash256 payment_point;
    AdaptorSignature adaptor_sig;

    // Revealed data (when claimed)
    std::optional<Hash256> payment_secret;
    std::optional<CompletedSignature> completed_sig;

    ChannelPTLC()
        : state(PTLCState::PROPOSED),
          outgoing(false),
          amount_sat(0),
          timeout_height(0) {}

    std::vector<uint8_t> serialize() const;
    static ChannelPTLC deserialize(const std::vector<uint8_t>& data);
};

/**
 * PTLC payment
 * Multi-hop payment using PTLCs
 */
struct PTLCPayment {
    Hash256 payment_id;                 // Unique payment identifier
    Hash256 payment_secret;             // Secret scalar (p)
    Hash256 payment_point;              // Public point (P = p*G)

    uint64_t amount_sat;                // Total amount
    uint32_t timeout_blocks;            // Timeout in blocks

    // Route
    std::vector<lightning::RouteHop> route;

    // PTLCs for each hop
    std::vector<ChannelPTLC> ptlcs;

    // State
    enum class State {
        PENDING,
        IN_FLIGHT,
        SUCCEEDED,
        FAILED
    } state;

    uint64_t created_at;
    uint64_t completed_at;

    PTLCPayment()
        : amount_sat(0), timeout_blocks(0),
          state(State::PENDING),
          created_at(0), completed_at(0) {}

    std::vector<uint8_t> serialize() const;
    static PTLCPayment deserialize(const std::vector<uint8_t>& data);
};

/**
 * PTLC manager
 * Manages Point Time Locked Contracts for Lightning payments
 */
class PTLCManager {
public:
    PTLCManager();
    ~PTLCManager() = default;

    // ========================================================================
    // Adaptor Signature Operations
    // ========================================================================

    /**
     * Create adaptor signature
     * Creates a signature that's valid only with the adaptor secret
     *
     * @param privkey Private key for signing
     * @param message Message to sign
     * @param adaptor_point Public adaptor point (T = t*G)
     * @return Adaptor signature
     */
    AdaptorSignature create_adaptor_signature(
        const DilithiumPrivKey& privkey,
        const Hash256& message,
        const Hash256& adaptor_point
    ) const;

    /**
     * Complete adaptor signature
     * Adds the secret scalar to create a valid signature
     *
     * @param adaptor_sig Adaptor signature
     * @param secret_scalar Secret scalar (t)
     * @return Completed signature
     */
    CompletedSignature complete_adaptor_signature(
        const AdaptorSignature& adaptor_sig,
        const Hash256& secret_scalar
    ) const;

    /**
     * Extract secret from completed signature
     * Recovers the secret scalar from adaptor and completed signatures
     *
     * @param adaptor_sig Original adaptor signature
     * @param completed_sig Completed signature
     * @return Secret scalar
     */
    Hash256 extract_secret(
        const AdaptorSignature& adaptor_sig,
        const CompletedSignature& completed_sig
    ) const;

    // ========================================================================
    // PTLC Payment Operations
    // ========================================================================

    /**
     * Create PTLC payment
     * Initiates a payment using PTLCs instead of HTLCs
     *
     * @param destination Destination node
     * @param amount_sat Payment amount
     * @param route Payment route
     * @param timeout_blocks Timeout in blocks
     * @return Payment ID if successful
     */
    std::optional<Hash256> create_ptlc_payment(
        const DilithiumPubKey& destination,
        uint64_t amount_sat,
        const std::vector<lightning::RouteHop>& route,
        uint32_t timeout_blocks = 144
    );

    /**
     * Send PTLC payment
     * Sends PTLCs on all hops
     *
     * @param payment_id Payment ID
     * @return True if sent successfully
     */
    bool send_ptlc_payment(const Hash256& payment_id);

    /**
     * Claim PTLC
     * Claims payment by revealing the secret
     *
     * @param ptlc_id PTLC ID
     * @param payment_secret Payment secret (p)
     * @return True if claimed successfully
     */
    bool claim_ptlc(
        const Hash256& ptlc_id,
        const Hash256& payment_secret
    );

    /**
     * Fail PTLC
     * Fails payment (routing error, insufficient funds, etc.)
     *
     * @param ptlc_id PTLC ID
     * @param error Error message
     * @return True if failed successfully
     */
    bool fail_ptlc(
        const Hash256& ptlc_id,
        const std::string& error
    );

    /**
     * Timeout PTLC
     * Reclaims funds after timeout
     *
     * @param ptlc_id PTLC ID
     * @return True if timed out successfully
     */
    bool timeout_ptlc(const Hash256& ptlc_id);

    // ========================================================================
    // Channel PTLC Management
    // ========================================================================

    /**
     * Add PTLC to channel
     * Adds outgoing or incoming PTLC
     *
     * @param channel_id Channel ID
     * @param ptlc PTLC to add
     * @param outgoing True if outgoing, false if incoming
     * @return True if added successfully
     */
    bool add_channel_ptlc(
        const Hash256& channel_id,
        const ChannelPTLC& ptlc,
        bool outgoing
    );

    /**
     * Remove PTLC from channel
     * Removes settled or failed PTLC
     *
     * @param channel_id Channel ID
     * @param ptlc_id PTLC ID
     * @return True if removed successfully
     */
    bool remove_channel_ptlc(
        const Hash256& channel_id,
        const Hash256& ptlc_id
    );

    /**
     * List channel PTLCs
     *
     * @param channel_id Channel ID
     * @return List of PTLCs in channel
     */
    std::vector<ChannelPTLC> list_channel_ptlcs(
        const Hash256& channel_id
    ) const;

    // ========================================================================
    // Payment Queries
    // ========================================================================

    /**
     * Get payment details
     */
    std::optional<PTLCPayment> get_payment(const Hash256& payment_id) const;

    /**
     * List all payments
     */
    std::vector<PTLCPayment> list_payments() const;

    /**
     * List payments by state
     */
    std::vector<PTLCPayment> list_payments_by_state(
        PTLCPayment::State state
    ) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    struct PTLCStats {
        size_t total_payments;
        size_t successful_payments;
        size_t failed_payments;
        uint64_t total_volume_sat;
        double success_rate;
        double avg_payment_time_seconds;
    };

    PTLCStats get_stats() const;

private:
    // Payment storage
    std::map<Hash256, PTLCPayment> payments_;

    // Channel PTLCs
    std::map<Hash256, std::vector<ChannelPTLC>> channel_ptlcs_;  // channel_id -> ptlcs

    uint32_t current_height_;
    mutable std::mutex mutex_;

    // Helper methods
    Hash256 generate_payment_id() const;
    Hash256 generate_payment_secret() const;
    Hash256 compute_payment_point(const Hash256& secret) const;

    Hash256 generate_adaptor_point() const;
    Hash256 point_add(const Hash256& p1, const Hash256& p2) const;
    Hash256 scalar_mult(const Hash256& scalar, const Hash256& point) const;

    bool verify_payment_secret(
        const Hash256& payment_point,
        const Hash256& payment_secret
    ) const;
};

/**
 * PTLC advantages over HTLCs:
 *
 * 1. Better Privacy:
 *    - No hash correlation across hops
 *    - Payments look like regular transactions
 *    - Routing nodes can't correlate payments
 *
 * 2. Scriptless Scripts:
 *    - No need for hash locks in scripts
 *    - Smaller transactions
 *    - Lower fees
 *
 * 3. Stuckless Payments:
 *    - Can cancel in-flight payments
 *    - Better for payment reliability
 *    - Reduced capital lock-up
 *
 * 4. Payment Decorrelation:
 *    - Each hop uses different point
 *    - Can't link sender to receiver
 *    - Enhanced anonymity
 *
 * 5. Multi-Hop Locks:
 *    - Can create complex payment conditions
 *    - Atomic multi-path payments
 *    - Payment splitting
 *
 * Technical Requirements:
 * - Schnorr signatures or adaptor signature scheme
 * - Point arithmetic on elliptic curves
 * - SIGHASH_NOINPUT for simplified updates (works well with Eltoo)
 */

/**
 * PTLC vs HTLC comparison:
 *
 * HTLC (Hash Time Locked Contract):
 * - Uses hash preimage (H(r) = h)
 * - Hash is same across all hops (linkable)
 * - Requires HTLC script in commitment tx
 * - Larger transaction size
 * - Stuck funds if payment hangs
 *
 * PTLC (Point Time Locked Contract):
 * - Uses elliptic curve point (P = p*G)
 * - Different point for each hop (unlinkable)
 * - Scriptless - uses adaptor signatures
 * - Smaller transaction size
 * - Can cancel stuck payments
 * - Better privacy (decorrelated)
 */

} // namespace ptlc
} // namespace intcoin

#endif // INTCOIN_PTLC_H
