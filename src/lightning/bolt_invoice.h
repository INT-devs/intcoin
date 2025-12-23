// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #11: Invoice Protocol for Lightning Payments (Full Implementation)

#ifndef INTCOIN_BOLT_INVOICE_H
#define INTCOIN_BOLT_INVOICE_H

#include "intcoin/types.h"
#include "intcoin/crypto.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace intcoin {
namespace bolt {

// ============================================================================
// BOLT #11: Invoice Protocol (Bech32-encoded)
// ============================================================================

// Invoice HRP (Human Readable Part) for INTcoin
constexpr const char* MAINNET_INVOICE_HRP = "lint";    // Lightning INTcoin
constexpr const char* TESTNET_INVOICE_HRP = "linti";   // Lightning INTcoin Testnet

// Tagged fields
enum class InvoiceTag : uint8_t {
    PAYMENT_HASH = 1,           // p: SHA256 payment_hash
    DESCRIPTION = 13,            // d: Short description
    PAYEE_PUBLIC_KEY = 19,      // n: Payee node public key
    DESCRIPTION_HASH = 23,       // h: SHA256 of description
    EXPIRY_TIME = 6,            // x: Expiry time in seconds
    MIN_FINAL_CLTV_EXPIRY = 24, // c: Min final CLTV expiry
    FALLBACK_ADDRESS = 9,        // f: Fallback on-chain address
    ROUTE_HINT = 3,              // r: Routing hint for private channels
    PAYMENT_SECRET = 16,         // s: Payment secret
    FEATURES = 5,                // 9: Feature bits
    PAYMENT_METADATA = 27,       // m: Payment metadata
};

// Routing hint for private channels
struct RouteHint {
    PublicKey node_id;
    uint64_t short_channel_id;
    uint32_t fee_base_msat;
    uint32_t fee_proportional_millionths;
    uint16_t cltv_expiry_delta;

    std::vector<uint8_t> Serialize() const;
    static Result<RouteHint> Deserialize(const std::vector<uint8_t>& data);
};

// Fallback address
struct FallbackAddress {
    uint8_t version;  // Witness version
    std::vector<uint8_t> program;  // Witness program

    std::vector<uint8_t> Serialize() const;
    static Result<FallbackAddress> Deserialize(const std::vector<uint8_t>& data);
};

// Invoice tagged field
struct InvoiceField {
    InvoiceTag tag;
    std::vector<uint8_t> data;

    std::vector<uint8_t> Serialize() const;
    static Result<InvoiceField> Deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

// BOLT #11 Invoice
class LightningInvoice {
public:
    LightningInvoice();

    // Required fields
    uint256 payment_hash;
    PublicKey payee_pubkey;

    // Optional fields with defaults
    std::optional<uint64_t> amount_msat;           // Amount in millisatoshis
    std::optional<std::string> description;        // Payment description
    std::optional<uint256> description_hash;       // Hash of description
    std::optional<uint32_t> expiry_seconds;        // Expiry (default 3600)
    std::optional<uint32_t> min_final_cltv_expiry; // Min final CLTV (default 18)
    std::optional<FallbackAddress> fallback_address;  // On-chain fallback
    std::vector<RouteHint> route_hints;            // Routing hints
    std::optional<uint256> payment_secret;         // Payment secret
    std::optional<std::vector<uint8_t>> features;  // Feature bits
    std::optional<std::vector<uint8_t>> payment_metadata;  // Metadata

    // Metadata
    uint64_t timestamp;         // Creation timestamp
    Signature signature;        // Invoice signature
    bool testnet;              // Testnet invoice

    // Builder pattern
    LightningInvoice& SetAmount(uint64_t msat);
    LightningInvoice& SetDescription(const std::string& desc);
    LightningInvoice& SetDescriptionHash(const uint256& hash);
    LightningInvoice& SetExpiry(uint32_t seconds);
    LightningInvoice& SetMinFinalCLTVExpiry(uint32_t blocks);
    LightningInvoice& SetFallbackAddress(const FallbackAddress& addr);
    LightningInvoice& AddRouteHint(const RouteHint& hint);
    LightningInvoice& SetPaymentSecret(const uint256& secret);
    LightningInvoice& SetFeatures(const std::vector<uint8_t>& feat);
    LightningInvoice& SetPaymentMetadata(const std::vector<uint8_t>& metadata);

    // Encode to BOLT #11 string (Bech32 encoded)
    std::string Encode() const;

    // Decode from BOLT #11 string
    static Result<LightningInvoice> Decode(const std::string& bolt11_string);

    // Sign invoice
    Result<void> Sign(const SecretKey& node_privkey);

    // Verify invoice signature
    bool Verify() const;

    // Check if expired
    bool IsExpired() const;

    // Get expiry time
    std::chrono::system_clock::time_point GetExpiryTime() const;

private:
    // Encode amount in hrp
    static std::string EncodeAmount(std::optional<uint64_t> amount_msat);

    // Decode amount from hrp
    static Result<std::optional<uint64_t>> DecodeAmount(const std::string& hrp);

    // Create signing data
    std::vector<uint8_t> GetSigningData() const;

    // Bech32 encoding/decoding helpers
    static std::string Bech32Encode(const std::string& hrp,
                                     const std::vector<uint8_t>& data);
    static Result<std::pair<std::string, std::vector<uint8_t>>> Bech32Decode(
        const std::string& encoded);

    // Convert 5-bit to 8-bit encoding
    static std::vector<uint8_t> ConvertBits(const std::vector<uint8_t>& data,
                                             int frombits, int tobits, bool pad);
};

// Invoice builder for easy construction
class InvoiceBuilder {
public:
    InvoiceBuilder(const PublicKey& payee_pubkey, bool testnet = false);

    InvoiceBuilder& WithPaymentHash(const uint256& hash);
    InvoiceBuilder& WithAmount(uint64_t msat);
    InvoiceBuilder& WithDescription(const std::string& desc);
    InvoiceBuilder& WithExpiry(uint32_t seconds);
    InvoiceBuilder& WithMinFinalCLTV(uint32_t blocks);
    InvoiceBuilder& WithPaymentSecret(const uint256& secret);
    InvoiceBuilder& WithRouteHint(const RouteHint& hint);
    InvoiceBuilder& WithFallbackAddress(const FallbackAddress& addr);

    // Build and sign invoice
    Result<LightningInvoice> Build(const SecretKey& node_privkey);

private:
    LightningInvoice invoice_;
};

// ============================================================================
// BOLT #12: Offers Protocol (Experimental)
// ============================================================================

// Offer TLV types
enum class OfferTLV : uint64_t {
    CHAINS = 2,
    CURRENCY = 6,
    AMOUNT = 8,
    DESCRIPTION = 10,
    FEATURES = 12,
    ABSOLUTE_EXPIRY = 14,
    PATHS = 16,
    ISSUER = 18,
    QUANTITY_MIN = 20,
    QUANTITY_MAX = 22,
    NODE_ID = 24,
    SIGNATURE = 240,
};

// Offer (reusable payment request)
class Offer {
public:
    Offer();

    // Offer fields
    std::optional<std::vector<uint256>> chains;     // Supported chains
    std::optional<std::string> currency;            // Fiat currency
    std::optional<uint64_t> amount_msat;           // Amount
    std::string description;                        // Offer description
    std::optional<std::vector<uint8_t>> features;  // Feature bits
    std::optional<uint64_t> absolute_expiry;       // Absolute expiry timestamp
    std::optional<std::string> issuer;             // Issuer name
    std::optional<uint64_t> quantity_min;          // Min quantity
    std::optional<uint64_t> quantity_max;          // Max quantity
    PublicKey node_id;                             // Node public key
    Signature signature;                            // Offer signature

    // Encode to BOLT #12 string (Bech32 encoded with "lno" HRP)
    std::string Encode() const;

    // Decode from BOLT #12 string
    static Result<Offer> Decode(const std::string& offer_string);

    // Sign offer
    Result<void> Sign(const SecretKey& node_privkey);

    // Verify offer signature
    bool Verify() const;

    // Check if expired
    bool IsExpired() const;

private:
    std::vector<uint8_t> GetSigningData() const;
};

// Invoice request (response to offer)
class InvoiceRequest {
public:
    InvoiceRequest();

    // Original offer
    Offer offer;

    // Request fields
    std::optional<uint64_t> quantity;        // Requested quantity
    std::optional<std::string> payer_note;   // Note from payer
    PublicKey payer_key;                     // Payer node key
    Signature signature;                      // Request signature

    // Encode to BOLT #12 string
    std::string Encode() const;

    // Decode from BOLT #12 string
    static Result<InvoiceRequest> Decode(const std::string& request_string);

    // Sign request
    Result<void> Sign(const SecretKey& payer_privkey);

    // Verify request
    bool Verify() const;
};

} // namespace bolt
} // namespace intcoin

#endif // INTCOIN_BOLT_INVOICE_H
