// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #11: Invoice Protocol Implementation

#include "bolt_invoice.h"
#include "intcoin/util.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace intcoin {
namespace bolt {

// ============================================================================
// Bech32 Encoding Constants
// ============================================================================

namespace {
    const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    const uint32_t GENERATOR[] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};

    // Polymod for Bech32 checksum
    uint32_t PolyMod(const std::vector<uint8_t>& values) {
        uint32_t chk = 1;
        for (uint8_t v : values) {
            uint8_t top = chk >> 25;
            chk = (chk & 0x1ffffff) << 5 ^ v;
            for (int i = 0; i < 5; i++) {
                if ((top >> i) & 1) {
                    chk ^= GENERATOR[i];
                }
            }
        }
        return chk;
    }

    // Expand HRP for checksum calculation
    std::vector<uint8_t> ExpandHRP(const std::string& hrp) {
        std::vector<uint8_t> result;
        for (char c : hrp) {
            result.push_back(c >> 5);
        }
        result.push_back(0);
        for (char c : hrp) {
            result.push_back(c & 31);
        }
        return result;
    }

    // Create checksum
    std::vector<uint8_t> CreateChecksum(const std::string& hrp, const std::vector<uint8_t>& data) {
        auto values = ExpandHRP(hrp);
        values.insert(values.end(), data.begin(), data.end());
        values.insert(values.end(), 6, 0);

        uint32_t mod = PolyMod(values) ^ 1;

        std::vector<uint8_t> checksum(6);
        for (int i = 0; i < 6; i++) {
            checksum[i] = (mod >> (5 * (5 - i))) & 31;
        }
        return checksum;
    }

    // Verify checksum
    bool VerifyChecksum(const std::string& hrp, const std::vector<uint8_t>& data) {
        auto values = ExpandHRP(hrp);
        values.insert(values.end(), data.begin(), data.end());
        return PolyMod(values) == 1;
    }

    // Convert between bit groups
    std::vector<uint8_t> ConvertBits(const std::vector<uint8_t>& data, int frombits, int tobits, bool pad) {
        std::vector<uint8_t> result;
        int acc = 0;
        int bits = 0;
        int maxv = (1 << tobits) - 1;
        int max_acc = (1 << (frombits + tobits - 1)) - 1;

        for (uint8_t value : data) {
            acc = ((acc << frombits) | value) & max_acc;
            bits += frombits;
            while (bits >= tobits) {
                bits -= tobits;
                result.push_back((acc >> bits) & maxv);
            }
        }

        if (pad) {
            if (bits > 0) {
                result.push_back((acc << (tobits - bits)) & maxv);
            }
        } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
            return {};  // Invalid padding
        }

        return result;
    }

    // Helper to write BigSize encoding
    void WriteBigSize(std::vector<uint8_t>& data, uint64_t value) {
        if (value < 253) {
            data.push_back(static_cast<uint8_t>(value));
        } else if (value < 65536) {
            data.push_back(253);
            data.push_back((value >> 8) & 0xFF);
            data.push_back(value & 0xFF);
        } else if (value < 4294967296ULL) {
            data.push_back(254);
            data.push_back((value >> 24) & 0xFF);
            data.push_back((value >> 16) & 0xFF);
            data.push_back((value >> 8) & 0xFF);
            data.push_back(value & 0xFF);
        } else {
            data.push_back(255);
            for (int i = 7; i >= 0; i--) {
                data.push_back((value >> (i * 8)) & 0xFF);
            }
        }
    }

    // Helper to read BigSize encoding
    uint64_t ReadBigSize(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset >= data.size()) return 0;

        uint8_t first = data[offset++];
        if (first < 253) {
            return first;
        } else if (first == 253) {
            if (offset + 2 > data.size()) return 0;
            uint64_t value = (static_cast<uint64_t>(data[offset]) << 8) | data[offset + 1];
            offset += 2;
            return value;
        } else if (first == 254) {
            if (offset + 4 > data.size()) return 0;
            uint64_t value = 0;
            for (int i = 0; i < 4; i++) {
                value = (value << 8) | data[offset++];
            }
            return value;
        } else {
            if (offset + 8 > data.size()) return 0;
            uint64_t value = 0;
            for (int i = 0; i < 8; i++) {
                value = (value << 8) | data[offset++];
            }
            return value;
        }
    }
}

// ============================================================================
// RouteHint Implementation
// ============================================================================

std::vector<uint8_t> RouteHint::Serialize() const {
    std::vector<uint8_t> data;
    auto pubkey_bytes = node_id.Serialize();
    data.insert(data.end(), pubkey_bytes.begin(), pubkey_bytes.end());

    for (int i = 7; i >= 0; i--) {
        data.push_back((short_channel_id >> (i * 8)) & 0xFF);
    }

    for (int i = 3; i >= 0; i--) {
        data.push_back((fee_base_msat >> (i * 8)) & 0xFF);
    }

    for (int i = 3; i >= 0; i--) {
        data.push_back((fee_proportional_millionths >> (i * 8)) & 0xFF);
    }

    data.push_back((cltv_expiry_delta >> 8) & 0xFF);
    data.push_back(cltv_expiry_delta & 0xFF);

    return data;
}

Result<RouteHint> RouteHint::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 33 + 8 + 4 + 4 + 2) {
        return Result<RouteHint>::Error("Insufficient data for route hint");
    }

    RouteHint hint;
    size_t offset = 0;

    std::vector<uint8_t> pubkey_bytes(data.begin(), data.begin() + 33);
    auto pubkey_result = PublicKey::Deserialize(pubkey_bytes);
    if (pubkey_result.IsOk()) {
        hint.node_id = pubkey_result.Unwrap();
    }
    offset += 33;

    hint.short_channel_id = 0;
    for (int i = 0; i < 8; i++) {
        hint.short_channel_id = (hint.short_channel_id << 8) | data[offset++];
    }

    hint.fee_base_msat = 0;
    for (int i = 0; i < 4; i++) {
        hint.fee_base_msat = (hint.fee_base_msat << 8) | data[offset++];
    }

    hint.fee_proportional_millionths = 0;
    for (int i = 0; i < 4; i++) {
        hint.fee_proportional_millionths = (hint.fee_proportional_millionths << 8) | data[offset++];
    }

    hint.cltv_expiry_delta = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];

    return Result<RouteHint>::Ok(hint);
}

// ============================================================================
// FallbackAddress Implementation
// ============================================================================

std::vector<uint8_t> FallbackAddress::Serialize() const {
    std::vector<uint8_t> data;
    data.push_back(version);
    data.insert(data.end(), program.begin(), program.end());
    return data;
}

Result<FallbackAddress> FallbackAddress::Deserialize(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return Result<FallbackAddress>::Error("Empty fallback address data");
    }

    FallbackAddress addr;
    addr.version = data[0];
    addr.program = std::vector<uint8_t>(data.begin() + 1, data.end());

    return Result<FallbackAddress>::Ok(addr);
}

// ============================================================================
// LightningInvoice Implementation
// ============================================================================

LightningInvoice::LightningInvoice() : timestamp(0), testnet(false) {}

LightningInvoice& LightningInvoice::SetAmount(uint64_t msat) {
    amount_msat = msat;
    return *this;
}

LightningInvoice& LightningInvoice::SetDescription(const std::string& desc) {
    description = desc;
    return *this;
}

LightningInvoice& LightningInvoice::SetDescriptionHash(const uint256& hash) {
    description_hash = hash;
    return *this;
}

LightningInvoice& LightningInvoice::SetExpiry(uint32_t seconds) {
    expiry_seconds = seconds;
    return *this;
}

LightningInvoice& LightningInvoice::SetMinFinalCLTVExpiry(uint32_t blocks) {
    min_final_cltv_expiry = blocks;
    return *this;
}

LightningInvoice& LightningInvoice::SetFallbackAddress(const FallbackAddress& addr) {
    fallback_address = addr;
    return *this;
}

LightningInvoice& LightningInvoice::AddRouteHint(const RouteHint& hint) {
    route_hints.push_back(hint);
    return *this;
}

LightningInvoice& LightningInvoice::SetPaymentSecret(const uint256& secret) {
    payment_secret = secret;
    return *this;
}

LightningInvoice& LightningInvoice::SetFeatures(const std::vector<uint8_t>& feat) {
    features = feat;
    return *this;
}

LightningInvoice& LightningInvoice::SetPaymentMetadata(const std::vector<uint8_t>& metadata) {
    payment_metadata = metadata;
    return *this;
}

std::string LightningInvoice::EncodeAmount(std::optional<uint64_t> amount_msat) {
    if (!amount_msat.has_value()) {
        return "";
    }

    uint64_t msat = amount_msat.value();

    // Convert to smallest denomination
    char multiplier = 'p';  // pico (0.001 satoshi)
    uint64_t value = msat / 10;

    if (value % 1000 == 0) {
        value /= 1000;
        multiplier = 'n';  // nano (0.001 satoshi)
    }
    if (value % 1000 == 0) {
        value /= 1000;
        multiplier = 'u';  // micro (satoshi)
    }
    if (value % 1000 == 0) {
        value /= 1000;
        multiplier = 'm';  // milli (0.001 INT)
    }

    std::ostringstream ss;
    ss << value << multiplier;
    return ss.str();
}

std::string LightningInvoice::Encode() const {
    // Build HRP (Human Readable Part)
    std::string hrp = testnet ? TESTNET_INVOICE_HRP : MAINNET_INVOICE_HRP;
    hrp += EncodeAmount(amount_msat);

    // Build data part (tagged fields)
    std::vector<uint8_t> data;

    // Timestamp (35 bits = 7 5-bit groups)
    uint64_t ts = timestamp;
    for (int i = 6; i >= 0; i--) {
        data.push_back((ts >> (i * 5)) & 31);
    }

    // Tagged fields
    auto write_tagged = [&](uint8_t tag, const std::vector<uint8_t>& value) {
        data.push_back(tag);

        // Length in 5-bit groups
        auto bits5 = ConvertBits(value, 8, 5, true);
        uint16_t len = bits5.size();
        data.push_back((len >> 5) & 31);
        data.push_back(len & 31);

        data.insert(data.end(), bits5.begin(), bits5.end());
    };

    // Payment hash (p)
    write_tagged(static_cast<uint8_t>(InvoiceTag::PAYMENT_HASH),
                 std::vector<uint8_t>(payment_hash.data(), payment_hash.data() + 32));

    // Description (d)
    if (description.has_value()) {
        std::vector<uint8_t> desc_bytes(description->begin(), description->end());
        write_tagged(static_cast<uint8_t>(InvoiceTag::DESCRIPTION), desc_bytes);
    }

    // Payee public key (n)
    write_tagged(static_cast<uint8_t>(InvoiceTag::PAYEE_PUBLIC_KEY), payee_pubkey.Serialize());

    // Description hash (h)
    if (description_hash.has_value()) {
        write_tagged(static_cast<uint8_t>(InvoiceTag::DESCRIPTION_HASH),
                    std::vector<uint8_t>(description_hash->data(), description_hash->data() + 32));
    }

    // Expiry (x)
    if (expiry_seconds.has_value()) {
        std::vector<uint8_t> expiry_bytes;
        uint32_t exp = expiry_seconds.value();
        for (int i = 3; i >= 0; i--) {
            expiry_bytes.push_back((exp >> (i * 8)) & 0xFF);
        }
        write_tagged(static_cast<uint8_t>(InvoiceTag::EXPIRY_TIME), expiry_bytes);
    }

    // Min final CLTV expiry (c)
    if (min_final_cltv_expiry.has_value()) {
        std::vector<uint8_t> cltv_bytes;
        uint32_t cltv = min_final_cltv_expiry.value();
        for (int i = 3; i >= 0; i--) {
            cltv_bytes.push_back((cltv >> (i * 8)) & 0xFF);
        }
        write_tagged(static_cast<uint8_t>(InvoiceTag::MIN_FINAL_CLTV_EXPIRY), cltv_bytes);
    }

    // Route hints (r)
    for (const auto& hint : route_hints) {
        write_tagged(static_cast<uint8_t>(InvoiceTag::ROUTE_HINT), hint.Serialize());
    }

    // Payment secret (s)
    if (payment_secret.has_value()) {
        write_tagged(static_cast<uint8_t>(InvoiceTag::PAYMENT_SECRET),
                    std::vector<uint8_t>(payment_secret->data(), payment_secret->data() + 32));
    }

    // Features (9)
    if (features.has_value()) {
        write_tagged(static_cast<uint8_t>(InvoiceTag::FEATURES), features.value());
    }

    // Signature (520 bits = 104 5-bit groups) - append after checksum calculation
    auto signing_data = data;

    // Add checksum
    auto checksum = CreateChecksum(hrp, data);
    data.insert(data.end(), checksum.begin(), checksum.end());

    // Encode to Bech32
    std::ostringstream result;
    result << hrp << "1";
    for (uint8_t d : data) {
        result << CHARSET[d];
    }

    return result.str();
}

Result<LightningInvoice> LightningInvoice::Decode(const std::string& bolt11_string) {
    // Find separator
    size_t sep_pos = bolt11_string.rfind('1');
    if (sep_pos == std::string::npos || sep_pos == 0) {
        return Result<LightningInvoice>::Error("Invalid invoice format");
    }

    std::string hrp = bolt11_string.substr(0, sep_pos);
    std::string data_str = bolt11_string.substr(sep_pos + 1);

    // Decode data part
    std::vector<uint8_t> data;
    for (char c : data_str) {
        const char* pos = std::strchr(CHARSET, c);
        if (!pos) {
            return Result<LightningInvoice>::Error("Invalid character in invoice");
        }
        data.push_back(static_cast<uint8_t>(pos - CHARSET));
    }

    // Verify checksum
    if (!VerifyChecksum(hrp, data)) {
        return Result<LightningInvoice>::Error("Invalid checksum");
    }

    // Remove checksum
    data.resize(data.size() - 6);

    LightningInvoice invoice;

    // Parse HRP
    if (hrp.find(MAINNET_INVOICE_HRP) == 0) {
        invoice.testnet = false;
        // Parse amount if present
    } else if (hrp.find(TESTNET_INVOICE_HRP) == 0) {
        invoice.testnet = true;
    } else {
        return Result<LightningInvoice>::Error("Invalid HRP");
    }

    // Parse timestamp (first 35 bits = 7 5-bit groups)
    if (data.size() < 7) {
        return Result<LightningInvoice>::Error("Insufficient data");
    }

    invoice.timestamp = 0;
    for (int i = 0; i < 7; i++) {
        invoice.timestamp = (invoice.timestamp << 5) | data[i];
    }

    size_t offset = 7;

    // Parse tagged fields
    while (offset < data.size()) {
        if (offset + 3 > data.size()) break;

        uint8_t tag = data[offset++];
        uint16_t len = (static_cast<uint16_t>(data[offset]) << 5) | data[offset + 1];
        offset += 2;

        if (offset + len > data.size()) break;

        std::vector<uint8_t> field_data5(data.begin() + offset, data.begin() + offset + len);
        auto field_data8 = ConvertBits(field_data5, 5, 8, false);
        offset += len;

        InvoiceTag tag_type = static_cast<InvoiceTag>(tag);

        switch (tag_type) {
            case InvoiceTag::PAYMENT_HASH:
                if (field_data8.size() == 32) {
                    std::memcpy(invoice.payment_hash.data(), field_data8.data(), 32);
                }
                break;

            case InvoiceTag::DESCRIPTION:
                invoice.description = std::string(field_data8.begin(), field_data8.end());
                break;

            case InvoiceTag::PAYEE_PUBLIC_KEY:
                if (field_data8.size() == 33) {
                    auto pubkey_result = PublicKey::Deserialize(field_data8);
                    if (pubkey_result.IsOk()) {
                        invoice.payee_pubkey = pubkey_result.Unwrap();
                    }
                }
                break;

            case InvoiceTag::PAYMENT_SECRET:
                if (field_data8.size() == 32) {
                    uint256 secret;
                    std::memcpy(secret.data(), field_data8.data(), 32);
                    invoice.payment_secret = secret;
                }
                break;

            default:
                break;
        }
    }

    return Result<LightningInvoice>::Ok(invoice);
}

Result<void> LightningInvoice::Sign(const SecretKey& node_privkey) {
    auto signing_data = GetSigningData();
    auto sig_result = node_privkey.Sign(signing_data);
    if (sig_result.IsError()) {
        return Result<void>::Error("Failed to sign invoice");
    }
    signature = sig_result.Unwrap();
    return Result<void>::Ok();
}

bool LightningInvoice::Verify() const {
    auto signing_data = GetSigningData();
    return payee_pubkey.Verify(signing_data, signature);
}

bool LightningInvoice::IsExpired() const {
    auto now = std::chrono::system_clock::now();
    return now > GetExpiryTime();
}

std::chrono::system_clock::time_point LightningInvoice::GetExpiryTime() const {
    uint32_t expiry = expiry_seconds.value_or(3600);  // Default 1 hour
    auto creation = std::chrono::system_clock::from_time_t(timestamp);
    return creation + std::chrono::seconds(expiry);
}

std::vector<uint8_t> LightningInvoice::GetSigningData() const {
    std::string hrp = testnet ? TESTNET_INVOICE_HRP : MAINNET_INVOICE_HRP;
    hrp += EncodeAmount(amount_msat);

    std::vector<uint8_t> data;
    uint64_t ts = timestamp;
    for (int i = 6; i >= 0; i--) {
        data.push_back((ts >> (i * 5)) & 31);
    }

    // Add all tagged fields (same as Encode but without signature)
    // Simplified for now - would need full implementation

    return data;
}

// ============================================================================
// InvoiceBuilder Implementation
// ============================================================================

InvoiceBuilder::InvoiceBuilder(const PublicKey& payee_pubkey, bool testnet) {
    invoice_.payee_pubkey = payee_pubkey;
    invoice_.testnet = testnet;
    invoice_.timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

InvoiceBuilder& InvoiceBuilder::WithPaymentHash(const uint256& hash) {
    invoice_.payment_hash = hash;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::WithAmount(uint64_t msat) {
    invoice_.amount_msat = msat;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::WithDescription(const std::string& desc) {
    invoice_.description = desc;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::WithExpiry(uint32_t seconds) {
    invoice_.expiry_seconds = seconds;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::WithMinFinalCLTV(uint32_t blocks) {
    invoice_.min_final_cltv_expiry = blocks;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::WithPaymentSecret(const uint256& secret) {
    invoice_.payment_secret = secret;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::WithRouteHint(const RouteHint& hint) {
    invoice_.route_hints.push_back(hint);
    return *this;
}

InvoiceBuilder& InvoiceBuilder::WithFallbackAddress(const FallbackAddress& addr) {
    invoice_.fallback_address = addr;
    return *this;
}

Result<LightningInvoice> InvoiceBuilder::Build(const SecretKey& node_privkey) {
    auto sign_result = invoice_.Sign(node_privkey);
    if (sign_result.IsError()) {
        return Result<LightningInvoice>::Error(sign_result.ErrorMessage());
    }
    return Result<LightningInvoice>::Ok(invoice_);
}

} // namespace bolt
} // namespace intcoin
