// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_LIGHTNING_INVOICE_H
#define INTCOIN_LIGHTNING_INVOICE_H

#include "../primitives.h"
#include "../crypto/crypto.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace intcoin {
namespace lightning {

/**
 * Lightning invoice (BOLT #11)
 *
 * Payment request with amount, description, and payment hash
 */
class Invoice {
public:
    Invoice();
    ~Invoice();

    // Invoice creation
    static Invoice create(const PublicKey& payee,
                         uint64_t amount_msat,
                         const std::string& description,
                         uint32_t expiry_seconds = 3600);

    static Invoice create_with_hash(const PublicKey& payee,
                                    uint64_t amount_msat,
                                    const Hash256& payment_hash,
                                    const std::string& description,
                                    uint32_t expiry_seconds = 3600);

    // Parsing
    static std::optional<Invoice> from_string(const std::string& invoice_str);

    // Encoding
    std::string to_string() const;
    std::string to_bech32() const;

    // Fields
    PublicKey get_payee() const { return payee_; }
    uint64_t get_amount() const { return amount_msat_; }
    Hash256 get_payment_hash() const { return payment_hash_; }
    Hash256 get_payment_secret() const { return payment_secret_; }
    std::string get_description() const { return description_; }
    uint32_t get_timestamp() const { return timestamp_; }
    uint32_t get_expiry() const { return expiry_; }
    uint32_t get_min_final_cltv_expiry() const { return min_final_cltv_expiry_; }

    // Route hints
    struct RouteHint {
        PublicKey node_id;
        Hash256 short_channel_id;
        uint32_t fee_base_msat;
        uint32_t fee_proportional_millionths;
        uint16_t cltv_expiry_delta;
    };

    void add_route_hint(const RouteHint& hint);
    std::vector<RouteHint> get_route_hints() const { return route_hints_; }

    // Validation
    bool is_expired() const;
    bool verify_signature() const;
    bool sign(const PrivateKey& key);

    // Features
    void set_feature(uint32_t feature_bit) { features_ |= (1ULL << feature_bit); }
    bool has_feature(uint32_t feature_bit) const {
        return (features_ & (1ULL << feature_bit)) != 0;
    }

    // Fallback address
    void set_fallback_address(const std::string& address) {
        fallback_address_ = address;
    }
    std::optional<std::string> get_fallback_address() const {
        return fallback_address_;
    }

private:
    // Required fields
    PublicKey payee_;               // Destination node
    Hash256 payment_hash_;          // Payment preimage hash
    uint64_t amount_msat_;          // Amount in millisatoshis
    uint32_t timestamp_;            // Creation timestamp

    // Optional fields
    std::string description_;       // Payment description
    Hash256 payment_secret_;        // Payment secret for MPP
    uint32_t expiry_;              // Expiry in seconds
    uint32_t min_final_cltv_expiry_; // Minimum final CLTV
    std::vector<RouteHint> route_hints_;
    std::optional<std::string> fallback_address_;
    uint64_t features_;            // Feature bits

    // Signature
    std::vector<uint8_t> signature_;

    // Encoding helpers
    std::vector<uint8_t> encode_data() const;
    bool decode_data(const std::vector<uint8_t>& data);
    Hash256 calculate_signing_hash() const;
};

/**
 * Invoice builder for convenient creation
 */
class InvoiceBuilder {
public:
    InvoiceBuilder(const PublicKey& payee);

    InvoiceBuilder& amount(uint64_t msat);
    InvoiceBuilder& description(const std::string& desc);
    InvoiceBuilder& payment_hash(const Hash256& hash);
    InvoiceBuilder& payment_secret(const Hash256& secret);
    InvoiceBuilder& expiry(uint32_t seconds);
    InvoiceBuilder& min_final_cltv(uint32_t blocks);
    InvoiceBuilder& route_hint(const Invoice::RouteHint& hint);
    InvoiceBuilder& fallback_address(const std::string& address);
    InvoiceBuilder& feature(uint32_t feature_bit);

    Invoice build() const;
    Invoice build_and_sign(const PrivateKey& key) const;

private:
    Invoice invoice_;
};

/**
 * Invoice manager
 *
 * Tracks invoices and payment status
 */
class InvoiceManager {
public:
    InvoiceManager();
    ~InvoiceManager();

    // Invoice operations
    Invoice create_invoice(uint64_t amount_msat,
                          const std::string& description,
                          uint32_t expiry_seconds = 3600);

    bool add_invoice(const Invoice& invoice);
    std::optional<Invoice> get_invoice(const Hash256& payment_hash) const;
    bool delete_invoice(const Hash256& payment_hash);

    // Payment tracking
    enum class PaymentStatus {
        PENDING,
        PAID,
        EXPIRED,
        CANCELLED
    };

    struct PaymentInfo {
        Hash256 payment_hash;
        Invoice invoice;
        PaymentStatus status;
        uint64_t amount_paid;
        uint32_t paid_at;

        PaymentInfo() : amount_paid(0), paid_at(0),
                       status(PaymentStatus::PENDING) {}
    };

    bool mark_invoice_paid(const Hash256& payment_hash,
                          uint64_t amount_paid = 0);
    bool cancel_invoice(const Hash256& payment_hash);
    PaymentStatus get_payment_status(const Hash256& payment_hash) const;
    std::optional<PaymentInfo> get_payment_info(const Hash256& payment_hash) const;

    // Queries
    std::vector<Invoice> get_all_invoices() const;
    std::vector<Invoice> get_pending_invoices() const;
    std::vector<Invoice> get_paid_invoices() const;
    void cleanup_expired_invoices();

    // Statistics
    size_t get_invoice_count() const;
    size_t get_paid_count() const;
    uint64_t get_total_received() const;

private:
    std::unordered_map<Hash256, PaymentInfo> invoices_;
    mutable std::mutex invoices_mutex_;

    PublicKey node_key_;
    PrivateKey signing_key_;
};

/**
 * Preimage generator
 *
 * Generates and manages payment preimages securely
 */
class PreimageGenerator {
public:
    PreimageGenerator();

    // Generate random preimage
    Hash256 generate_preimage();

    // Get hash of preimage
    static Hash256 hash_preimage(const Hash256& preimage);

    // Store preimage securely
    void store_preimage(const Hash256& hash, const Hash256& preimage);
    std::optional<Hash256> get_preimage(const Hash256& hash) const;
    bool has_preimage(const Hash256& hash) const;

    // Cleanup
    void remove_preimage(const Hash256& hash);
    void cleanup_old_preimages(uint32_t max_age_seconds);

private:
    std::unordered_map<Hash256, Hash256> preimages_;  // hash -> preimage
    std::unordered_map<Hash256, uint32_t> timestamps_; // hash -> timestamp
    mutable std::mutex preimages_mutex_;
};

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_INVOICE_H
