// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network invoice implementation (BOLT #11).
// Payment request format for Lightning payments.

#ifndef INTCOIN_LIGHTNING_INVOICE_H
#define INTCOIN_LIGHTNING_INVOICE_H

#include "primitives.h"
#include "crypto.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace intcoin {
namespace lightning {
namespace invoice {

/**
 * Invoice field types (BOLT #11)
 */
enum class FieldType : uint8_t {
    PAYMENT_HASH = 1,              // 'p' - Payment hash
    DESCRIPTION = 13,              // 'd' - Short description
    NODE_ID = 19,                  // 'n' - Node public key
    EXPIRY_TIME = 24,              // 'x' - Expiry time in seconds
    CLTV_EXPIRY = 9,               // 'c' - Min final CLTV expiry
    FALLBACK_ADDRESS = 6,          // 'f' - On-chain fallback address
    ROUTE_HINT = 18,               // 'r' - Routing hint
    PAYMENT_SECRET = 16,           // 's' - Payment secret
    FEATURES = 5,                  // '9' - Feature bits
    METADATA = 27                  // 'm' - Payment metadata
};

/**
 * Routing hint for a payment route
 */
struct RouteHint {
    DilithiumPubKey node_id;           // Node public key
    Hash256 short_channel_id;          // Short channel ID
    uint32_t fee_base_msat;            // Base fee in millisatoshis
    uint32_t fee_proportional_millionths;  // Proportional fee
    uint16_t cltv_expiry_delta;        // CLTV expiry delta

    RouteHint() : short_channel_id{}, fee_base_msat(0),
                  fee_proportional_millionths(0), cltv_expiry_delta(0) {}

    std::vector<uint8_t> serialize() const;
    static std::optional<RouteHint> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Tagged field in invoice
 */
struct TaggedField {
    FieldType type;
    std::vector<uint8_t> data;

    TaggedField() : type(FieldType::PAYMENT_HASH) {}
    TaggedField(FieldType t, const std::vector<uint8_t>& d) : type(t), data(d) {}

    std::vector<uint8_t> serialize() const;
    static std::optional<TaggedField> deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

/**
 * Lightning Network invoice (BOLT #11)
 */
class Invoice {
public:
    // Required fields
    uint64_t timestamp;                        // Creation timestamp (UNIX)
    Hash256 payment_hash;                      // Payment hash (32 bytes)
    DilithiumPubKey node_id;                   // Payee node ID
    DilithiumSignature signature;              // Invoice signature

    // Optional fields
    std::optional<uint64_t> amount_msat;       // Amount in millisatoshis
    std::string description;                   // Human-readable description
    uint32_t expiry_seconds;                   // Expiry time (default 3600)
    uint16_t min_final_cltv_expiry;            // Min final CLTV (default 18)
    std::optional<std::string> fallback_address;  // On-chain fallback
    std::vector<RouteHint> route_hints;        // Routing hints
    std::optional<Hash256> payment_secret;     // Payment secret for MPP
    std::vector<uint8_t> features;             // Feature bits
    std::optional<std::vector<uint8_t>> metadata;  // Payment metadata

    // Network identifier
    std::string network_prefix;                // "lnbc" for mainnet, "lntb" for testnet

    Invoice();

    /**
     * Encode invoice to BOLT #11 string format
     *
     * @param keypair Signing key for the invoice
     * @return Encoded invoice string (bech32)
     */
    std::string encode(const crypto::DilithiumKeyPair& keypair) const;

    /**
     * Decode invoice from BOLT #11 string format
     *
     * @param invoice_str Encoded invoice string
     * @return Decoded invoice, or nullopt if invalid
     */
    static std::optional<Invoice> decode(const std::string& invoice_str);

    /**
     * Verify invoice signature
     *
     * @return True if signature is valid
     */
    bool verify_signature() const;

    /**
     * Check if invoice is expired
     *
     * @return True if invoice has expired
     */
    bool is_expired() const;

    /**
     * Get expiration timestamp
     *
     * @return Unix timestamp when invoice expires
     */
    uint64_t get_expiry_timestamp() const;

    /**
     * Get human-readable summary
     *
     * @return Formatted invoice summary
     */
    std::string to_string() const;

    // Encode tagged fields
    std::vector<TaggedField> get_tagged_fields() const;

private:

    // Sign invoice data
    void sign(const crypto::DilithiumKeyPair& keypair);

    // Compute signature hash
    Hash256 signature_hash() const;
};

/**
 * Invoice builder with fluent interface
 */
class InvoiceBuilder {
public:
    InvoiceBuilder();

    // Required fields
    InvoiceBuilder& payment_hash(const Hash256& hash);
    InvoiceBuilder& node_id(const DilithiumPubKey& id);
    InvoiceBuilder& network(const std::string& net);

    // Optional fields
    InvoiceBuilder& amount_millisatoshis(uint64_t amount_msat);
    InvoiceBuilder& description(const std::string& desc);
    InvoiceBuilder& expiry_seconds(uint32_t seconds);
    InvoiceBuilder& min_final_cltv_expiry(uint16_t blocks);
    InvoiceBuilder& fallback_address(const std::string& addr);
    InvoiceBuilder& route_hint(const RouteHint& hint);
    InvoiceBuilder& payment_secret(const Hash256& secret);
    InvoiceBuilder& features(const std::vector<uint8_t>& feat);
    InvoiceBuilder& metadata(const std::vector<uint8_t>& meta);

    // Build and sign the invoice
    std::optional<Invoice> build(const crypto::DilithiumKeyPair& keypair) const;

private:
    Invoice invoice_;
    bool has_payment_hash_;
    bool has_node_id_;
};

/**
 * Bech32 encoding/decoding for invoices
 */
namespace bech32 {

/**
 * Bech32 character set
 */
extern const char* CHARSET;

/**
 * Convert data to bech32 encoding
 *
 * @param hrp Human-readable prefix
 * @param data Data to encode (5-bit groups)
 * @return Bech32 encoded string
 */
std::string encode(const std::string& hrp, const std::vector<uint8_t>& data);

/**
 * Decode bech32 string
 *
 * @param str Bech32 encoded string
 * @param hrp Output human-readable prefix
 * @param data Output decoded data (5-bit groups)
 * @return True if decoding succeeded
 */
bool decode(const std::string& str, std::string& hrp, std::vector<uint8_t>& data);

/**
 * Convert 8-bit data to 5-bit groups
 *
 * @param data Input 8-bit data
 * @return Output 5-bit groups
 */
std::vector<uint8_t> convert_bits_8to5(const std::vector<uint8_t>& data);

/**
 * Convert 5-bit groups to 8-bit data
 *
 * @param data Input 5-bit groups
 * @return Output 8-bit data
 */
std::vector<uint8_t> convert_bits_5to8(const std::vector<uint8_t>& data);

/**
 * Compute bech32 checksum
 *
 * @param hrp Human-readable prefix
 * @param data Data (5-bit groups)
 * @return 6-character checksum
 */
std::vector<uint8_t> create_checksum(const std::string& hrp,
                                     const std::vector<uint8_t>& data);

/**
 * Verify bech32 checksum
 *
 * @param hrp Human-readable prefix
 * @param data Data including checksum (5-bit groups)
 * @return True if checksum is valid
 */
bool verify_checksum(const std::string& hrp, const std::vector<uint8_t>& data);

} // namespace bech32

/**
 * Invoice utilities
 */
namespace utils {

/**
 * Generate random payment preimage
 *
 * @return 32-byte random preimage
 */
std::vector<uint8_t> generate_preimage();

/**
 * Compute payment hash from preimage
 *
 * @param preimage Payment preimage (32 bytes)
 * @return Payment hash (SHA3-256)
 */
Hash256 compute_payment_hash(const std::vector<uint8_t>& preimage);

/**
 * Generate random payment secret
 *
 * @return 32-byte random secret
 */
Hash256 generate_payment_secret();

/**
 * Format amount in human-readable form
 *
 * @param amount_msat Amount in millisatoshis
 * @return Formatted string (e.g., "1.234 INT")
 */
std::string format_amount(uint64_t amount_msat);

/**
 * Parse amount from string
 *
 * @param amount_str Amount string (e.g., "1.234 INT")
 * @return Amount in millisatoshis, or nullopt if invalid
 */
std::optional<uint64_t> parse_amount(const std::string& amount_str);

/**
 * Format timestamp in ISO 8601
 *
 * @param timestamp Unix timestamp
 * @return ISO 8601 formatted string
 */
std::string format_timestamp(uint64_t timestamp);

} // namespace utils

} // namespace invoice
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_INVOICE_H
