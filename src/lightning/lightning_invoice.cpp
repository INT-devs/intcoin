// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/lightning_invoice.h"
#include "intcoin/serialization.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <openssl/rand.h>
#include <openssl/evp.h>

namespace intcoin {
namespace lightning {
namespace invoice {

// ===== Bech32 Implementation =====

namespace bech32 {

const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

std::string encode(const std::string& hrp, const std::vector<uint8_t>& data) {
    auto checksum = create_checksum(hrp, data);

    std::string result = hrp + "1";
    for (uint8_t val : data) {
        if (val >= 32) return "";  // Invalid
        result += CHARSET[val];
    }
    for (uint8_t val : checksum) {
        result += CHARSET[val];
    }

    return result;
}

bool decode(const std::string& str, std::string& hrp, std::vector<uint8_t>& data) {
    // Find separator
    size_t sep_pos = str.rfind('1');
    if (sep_pos == std::string::npos || sep_pos == 0 || sep_pos + 7 > str.length()) {
        return false;
    }

    hrp = str.substr(0, sep_pos);

    // Decode data
    data.clear();
    for (size_t i = sep_pos + 1; i < str.length(); ++i) {
        const char* pos = std::strchr(CHARSET, std::tolower(str[i]));
        if (!pos) return false;
        data.push_back(static_cast<uint8_t>(pos - CHARSET));
    }

    // Verify checksum
    if (!verify_checksum(hrp, data)) {
        return false;
    }

    // Remove checksum
    data.resize(data.size() - 6);
    return true;
}

std::vector<uint8_t> convert_bits_8to5(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;
    uint32_t buffer = 0;
    int bits = 0;

    for (uint8_t byte : data) {
        buffer = (buffer << 8) | byte;
        bits += 8;

        while (bits >= 5) {
            bits -= 5;
            result.push_back(static_cast<uint8_t>((buffer >> bits) & 0x1F));
        }
    }

    // Flush remaining bits
    if (bits > 0) {
        result.push_back(static_cast<uint8_t>((buffer << (5 - bits)) & 0x1F));
    }

    return result;
}

std::vector<uint8_t> convert_bits_5to8(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;
    uint32_t buffer = 0;
    int bits = 0;

    for (uint8_t val : data) {
        if (val >= 32) return {};  // Invalid
        buffer = (buffer << 5) | val;
        bits += 5;

        while (bits >= 8) {
            bits -= 8;
            result.push_back(static_cast<uint8_t>((buffer >> bits) & 0xFF));
        }
    }

    // Don't include padding bits
    if (bits >= 5 || ((buffer << (8 - bits)) & 0xFF)) {
        return {};  // Invalid padding
    }

    return result;
}

uint32_t polymod(const std::vector<uint8_t>& values) {
    uint32_t generator[] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};
    uint32_t chk = 1;

    for (uint8_t val : values) {
        uint8_t top = chk >> 25;
        chk = ((chk & 0x1ffffff) << 5) ^ val;
        for (int i = 0; i < 5; ++i) {
            chk ^= ((top >> i) & 1) ? generator[i] : 0;
        }
    }

    return chk;
}

std::vector<uint8_t> hrp_expand(const std::string& hrp) {
    std::vector<uint8_t> result;
    for (char c : hrp) {
        result.push_back(static_cast<uint8_t>(c) >> 5);
    }
    result.push_back(0);
    for (char c : hrp) {
        result.push_back(static_cast<uint8_t>(c) & 0x1f);
    }
    return result;
}

std::vector<uint8_t> create_checksum(const std::string& hrp,
                                     const std::vector<uint8_t>& data) {
    std::vector<uint8_t> values = hrp_expand(hrp);
    values.insert(values.end(), data.begin(), data.end());
    values.insert(values.end(), 6, 0);

    uint32_t mod = polymod(values) ^ 1;
    std::vector<uint8_t> checksum(6);
    for (int i = 0; i < 6; ++i) {
        checksum[i] = static_cast<uint8_t>((mod >> (5 * (5 - i))) & 0x1f);
    }

    return checksum;
}

bool verify_checksum(const std::string& hrp, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> values = hrp_expand(hrp);
    values.insert(values.end(), data.begin(), data.end());
    return polymod(values) == 1;
}

} // namespace bech32

// ===== RouteHint Implementation =====

std::vector<uint8_t> RouteHint::serialize() const {
    std::vector<uint8_t> data;

    // Node ID (2592 bytes for Dilithium5)
    data.insert(data.end(), node_id.begin(), node_id.end());

    // Short channel ID (32 bytes)
    data.insert(data.end(), short_channel_id.begin(), short_channel_id.end());

    // Fee base (4 bytes)
    for (int i = 0; i < 4; i++) {
        data.push_back(static_cast<uint8_t>((fee_base_msat >> (i * 8)) & 0xFF));
    }

    // Fee proportional (4 bytes)
    for (int i = 0; i < 4; i++) {
        data.push_back(static_cast<uint8_t>((fee_proportional_millionths >> (i * 8)) & 0xFF));
    }

    // CLTV expiry delta (2 bytes)
    data.push_back(static_cast<uint8_t>(cltv_expiry_delta & 0xFF));
    data.push_back(static_cast<uint8_t>((cltv_expiry_delta >> 8) & 0xFF));

    return data;
}

std::optional<RouteHint> RouteHint::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2634) {  // 2592 + 32 + 4 + 4 + 2
        return std::nullopt;
    }

    RouteHint hint;
    size_t offset = 0;

    // Node ID
    std::copy(data.begin() + offset, data.begin() + offset + 2592, hint.node_id.begin());
    offset += 2592;

    // Short channel ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, hint.short_channel_id.begin());
    offset += 32;

    // Fee base
    hint.fee_base_msat = 0;
    for (int i = 0; i < 4; i++) {
        hint.fee_base_msat |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }

    // Fee proportional
    hint.fee_proportional_millionths = 0;
    for (int i = 0; i < 4; i++) {
        hint.fee_proportional_millionths |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }

    // CLTV expiry delta
    hint.cltv_expiry_delta = static_cast<uint16_t>(data[offset]) |
                             (static_cast<uint16_t>(data[offset + 1]) << 8);

    return hint;
}

// ===== TaggedField Implementation =====

std::vector<uint8_t> TaggedField::serialize() const {
    std::vector<uint8_t> result;

    // Field type (1 byte converted to 5-bit)
    result.push_back(static_cast<uint8_t>(type));

    // Data length (variable, in 5-bit chunks)
    uint16_t data_len = static_cast<uint16_t>(data.size());
    result.push_back(static_cast<uint8_t>((data_len >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(data_len & 0xFF));

    // Data
    result.insert(result.end(), data.begin(), data.end());

    return result;
}

std::optional<TaggedField> TaggedField::deserialize(const std::vector<uint8_t>& data,
                                                     size_t& offset) {
    if (offset + 3 > data.size()) {
        return std::nullopt;
    }

    TaggedField field;

    // Field type
    field.type = static_cast<FieldType>(data[offset++]);

    // Data length
    uint16_t data_len = (static_cast<uint16_t>(data[offset]) << 8) |
                        static_cast<uint16_t>(data[offset + 1]);
    offset += 2;

    // Data
    if (offset + data_len > data.size()) {
        return std::nullopt;
    }

    field.data.assign(data.begin() + offset, data.begin() + offset + data_len);
    offset += data_len;

    return field;
}

// ===== Invoice Implementation =====

Invoice::Invoice()
    : timestamp(0)
    , payment_hash{}
    , expiry_seconds(3600)
    , min_final_cltv_expiry(18)
    , network_prefix("lnint") {  // INTcoin Lightning Network prefix
}

std::string Invoice::encode(const crypto::DilithiumKeyPair& keypair) const {
    // Sign the invoice
    Invoice signed_invoice = *this;
    signed_invoice.node_id = keypair.public_key;

    // Build data to sign
    auto tagged_fields = signed_invoice.get_tagged_fields();

    std::vector<uint8_t> data_to_encode;

    // Timestamp (35 bits = 7 * 5-bit groups)
    for (int i = 6; i >= 0; i--) {
        data_to_encode.push_back(static_cast<uint8_t>((timestamp >> (i * 5)) & 0x1F));
    }

    // Tagged fields
    for (const auto& field : tagged_fields) {
        auto field_data = field.serialize();
        auto field_5bit = bech32::convert_bits_8to5(field_data);
        data_to_encode.insert(data_to_encode.end(), field_5bit.begin(), field_5bit.end());
    }

    // Sign the data
    std::vector<uint8_t> sig_data(data_to_encode.begin(), data_to_encode.end());
    signed_invoice.signature = crypto::Dilithium::sign(sig_data, keypair);

    // Add signature to encoded data
    std::vector<uint8_t> sig_vec(signed_invoice.signature.begin(), signed_invoice.signature.end());
    auto sig_5bit = bech32::convert_bits_8to5(sig_vec);
    data_to_encode.insert(data_to_encode.end(), sig_5bit.begin(), sig_5bit.end());

    // Encode with bech32
    std::string hrp = network_prefix;
    if (amount_msat.has_value()) {
        // Encode amount in hrp
        hrp += std::to_string(*amount_msat / 1000);  // Simplified
    }

    return bech32::encode(hrp, data_to_encode);
}

std::optional<Invoice> Invoice::decode(const std::string& invoice_str) {
    std::string hrp;
    std::vector<uint8_t> data_5bit;

    if (!bech32::decode(invoice_str, hrp, data_5bit)) {
        return std::nullopt;
    }

    // Parse hrp for network and amount
    Invoice invoice;
    if (hrp.substr(0, 5) == "lnint") {
        invoice.network_prefix = "lnint";
    } else if (hrp.substr(0, 5) == "lntbi") {  // INTcoin testnet
        invoice.network_prefix = "lntbi";
    } else {
        return std::nullopt;  // Unknown network
    }

    // Convert from 5-bit to 8-bit
    auto data = bech32::convert_bits_5to8(data_5bit);
    if (data.empty()) {
        return std::nullopt;
    }

    size_t offset = 0;

    // Parse timestamp (7 * 5-bit = 35 bits)
    if (data.size() < 7) return std::nullopt;
    invoice.timestamp = 0;
    for (int i = 6; i >= 0; i--) {
        if (offset >= data.size()) return std::nullopt;
        invoice.timestamp |= (static_cast<uint64_t>(data[offset++] & 0x1F) << (i * 5));
    }

    // Parse tagged fields
    while (offset < data.size()) {
        // Check if this is the signature (last field)
        if (data.size() - offset == 4595) {  // Dilithium5 signature size
            // Extract signature
            std::copy(data.begin() + offset, data.end(), invoice.signature.begin());
            break;
        }

        auto field = TaggedField::deserialize(data, offset);
        if (!field) break;

        // Process field based on type
        switch (field->type) {
            case FieldType::PAYMENT_HASH:
                if (field->data.size() == 32) {
                    std::copy(field->data.begin(), field->data.end(), invoice.payment_hash.begin());
                }
                break;
            case FieldType::DESCRIPTION:
                invoice.description = std::string(field->data.begin(), field->data.end());
                break;
            case FieldType::NODE_ID:
                if (field->data.size() == 2592) {
                    std::copy(field->data.begin(), field->data.end(), invoice.node_id.begin());
                }
                break;
            case FieldType::EXPIRY_TIME:
                if (field->data.size() == 4) {
                    invoice.expiry_seconds = 0;
                    for (int i = 0; i < 4; i++) {
                        invoice.expiry_seconds |= (static_cast<uint32_t>(field->data[i]) << (i * 8));
                    }
                }
                break;
            case FieldType::CLTV_EXPIRY:
                if (field->data.size() == 2) {
                    invoice.min_final_cltv_expiry = static_cast<uint16_t>(field->data[0]) |
                                                   (static_cast<uint16_t>(field->data[1]) << 8);
                }
                break;
            case FieldType::FALLBACK_ADDRESS:
                invoice.fallback_address = std::string(field->data.begin(), field->data.end());
                break;
            case FieldType::ROUTE_HINT: {
                auto hint = RouteHint::deserialize(field->data);
                if (hint) {
                    invoice.route_hints.push_back(*hint);
                }
                break;
            }
            case FieldType::PAYMENT_SECRET:
                if (field->data.size() == 32) {
                    Hash256 secret;
                    std::copy(field->data.begin(), field->data.end(), secret.begin());
                    invoice.payment_secret = secret;
                }
                break;
            case FieldType::FEATURES:
                invoice.features = field->data;
                break;
            case FieldType::METADATA:
                invoice.metadata = field->data;
                break;
            default:
                // Unknown field, skip
                break;
        }
    }

    return invoice;
}

std::vector<TaggedField> Invoice::get_tagged_fields() const {
    std::vector<TaggedField> fields;

    // Payment hash (required)
    fields.emplace_back(FieldType::PAYMENT_HASH,
                       std::vector<uint8_t>(payment_hash.begin(), payment_hash.end()));

    // Description
    if (!description.empty()) {
        fields.emplace_back(FieldType::DESCRIPTION,
                           std::vector<uint8_t>(description.begin(), description.end()));
    }

    // Node ID
    fields.emplace_back(FieldType::NODE_ID,
                       std::vector<uint8_t>(node_id.begin(), node_id.end()));

    // Expiry time
    std::vector<uint8_t> expiry_data(4);
    for (int i = 0; i < 4; i++) {
        expiry_data[i] = static_cast<uint8_t>((expiry_seconds >> (i * 8)) & 0xFF);
    }
    fields.emplace_back(FieldType::EXPIRY_TIME, expiry_data);

    // CLTV expiry
    std::vector<uint8_t> cltv_data(2);
    cltv_data[0] = static_cast<uint8_t>(min_final_cltv_expiry & 0xFF);
    cltv_data[1] = static_cast<uint8_t>((min_final_cltv_expiry >> 8) & 0xFF);
    fields.emplace_back(FieldType::CLTV_EXPIRY, cltv_data);

    // Fallback address
    if (fallback_address.has_value()) {
        fields.emplace_back(FieldType::FALLBACK_ADDRESS,
                           std::vector<uint8_t>(fallback_address->begin(), fallback_address->end()));
    }

    // Route hints
    for (const auto& hint : route_hints) {
        fields.emplace_back(FieldType::ROUTE_HINT, hint.serialize());
    }

    // Payment secret
    if (payment_secret.has_value()) {
        fields.emplace_back(FieldType::PAYMENT_SECRET,
                           std::vector<uint8_t>(payment_secret->begin(), payment_secret->end()));
    }

    // Features
    if (!features.empty()) {
        fields.emplace_back(FieldType::FEATURES, features);
    }

    // Metadata
    if (metadata.has_value()) {
        fields.emplace_back(FieldType::METADATA, *metadata);
    }

    return fields;
}

bool Invoice::verify_signature() const {
    // Reconstruct data that was signed
    auto tagged_fields = get_tagged_fields();

    std::vector<uint8_t> data_to_verify;

    // Timestamp
    for (int i = 6; i >= 0; i--) {
        data_to_verify.push_back(static_cast<uint8_t>((timestamp >> (i * 5)) & 0x1F));
    }

    // Tagged fields
    for (const auto& field : tagged_fields) {
        auto field_data = field.serialize();
        auto field_5bit = bech32::convert_bits_8to5(field_data);
        data_to_verify.insert(data_to_verify.end(), field_5bit.begin(), field_5bit.end());
    }

    // Verify signature
    // Convert DilithiumPubKey to vector for verify function
    std::vector<uint8_t> node_id_vec(node_id.begin(), node_id.end());
    return crypto::Dilithium::verify(data_to_verify, signature, node_id);
}

bool Invoice::is_expired() const {
    auto now = std::chrono::system_clock::now();
    auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    return static_cast<uint64_t>(now_seconds) > get_expiry_timestamp();
}

uint64_t Invoice::get_expiry_timestamp() const {
    return timestamp + expiry_seconds;
}

std::string Invoice::to_string() const {
    std::ostringstream oss;
    oss << "Lightning Invoice\n";
    oss << "  Network: " << network_prefix << "\n";
    oss << "  Timestamp: " << utils::format_timestamp(timestamp) << "\n";

    if (amount_msat.has_value()) {
        oss << "  Amount: " << utils::format_amount(*amount_msat) << "\n";
    } else {
        oss << "  Amount: Any\n";
    }

    oss << "  Description: " << description << "\n";
    oss << "  Expiry: " << expiry_seconds << " seconds\n";
    oss << "  Min CLTV: " << min_final_cltv_expiry << " blocks\n";

    if (fallback_address.has_value()) {
        oss << "  Fallback: " << *fallback_address << "\n";
    }

    oss << "  Route Hints: " << route_hints.size() << "\n";

    return oss.str();
}

// ===== InvoiceBuilder Implementation =====

InvoiceBuilder::InvoiceBuilder()
    : has_payment_hash_(false)
    , has_node_id_(false) {
    invoice_.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

InvoiceBuilder& InvoiceBuilder::payment_hash(const Hash256& hash) {
    invoice_.payment_hash = hash;
    has_payment_hash_ = true;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::node_id(const DilithiumPubKey& id) {
    invoice_.node_id = id;
    has_node_id_ = true;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::network(const std::string& net) {
    invoice_.network_prefix = net;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::amount_millisatoshis(uint64_t amount_msat) {
    invoice_.amount_msat = amount_msat;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::description(const std::string& desc) {
    invoice_.description = desc;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::expiry_seconds(uint32_t seconds) {
    invoice_.expiry_seconds = seconds;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::min_final_cltv_expiry(uint16_t blocks) {
    invoice_.min_final_cltv_expiry = blocks;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::fallback_address(const std::string& addr) {
    invoice_.fallback_address = addr;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::route_hint(const RouteHint& hint) {
    invoice_.route_hints.push_back(hint);
    return *this;
}

InvoiceBuilder& InvoiceBuilder::payment_secret(const Hash256& secret) {
    invoice_.payment_secret = secret;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::features(const std::vector<uint8_t>& feat) {
    invoice_.features = feat;
    return *this;
}

InvoiceBuilder& InvoiceBuilder::metadata(const std::vector<uint8_t>& meta) {
    invoice_.metadata = meta;
    return *this;
}

std::optional<Invoice> InvoiceBuilder::build(const crypto::DilithiumKeyPair& keypair) const {
    if (!has_payment_hash_) {
        return std::nullopt;
    }

    Invoice result = invoice_;
    result.node_id = keypair.public_key;

    // Sign the invoice
    auto tagged_fields = result.get_tagged_fields();
    std::vector<uint8_t> data_to_sign;

    // Timestamp
    for (int i = 6; i >= 0; i--) {
        data_to_sign.push_back(static_cast<uint8_t>((result.timestamp >> (i * 5)) & 0x1F));
    }

    // Tagged fields
    for (const auto& field : tagged_fields) {
        auto field_data = field.serialize();
        auto field_5bit = bech32::convert_bits_8to5(field_data);
        data_to_sign.insert(data_to_sign.end(), field_5bit.begin(), field_5bit.end());
    }

    result.signature = crypto::Dilithium::sign(data_to_sign, keypair);

    return result;
}

// ===== Utility Functions =====

namespace utils {

std::vector<uint8_t> generate_preimage() {
    std::vector<uint8_t> preimage(32);
    RAND_bytes(preimage.data(), 32);
    return preimage;
}

Hash256 compute_payment_hash(const std::vector<uint8_t>& preimage) {
    Hash256 hash;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    unsigned int hash_len = 0;

    EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr);
    EVP_DigestUpdate(ctx, preimage.data(), preimage.size());
    EVP_DigestFinal_ex(ctx, hash.data(), &hash_len);
    EVP_MD_CTX_free(ctx);

    return hash;
}

Hash256 generate_payment_secret() {
    Hash256 secret;
    RAND_bytes(secret.data(), 32);
    return secret;
}

std::string format_amount(uint64_t amount_msat) {
    double amount_int = static_cast<double>(amount_msat) / 1000000.0;  // msat to INT
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << amount_int << " INT";
    return oss.str();
}

std::optional<uint64_t> parse_amount(const std::string& amount_str) {
    // Simple parser: "1.234 INT"
    std::istringstream iss(amount_str);
    double amount;
    std::string unit;

    if (!(iss >> amount >> unit)) {
        return std::nullopt;
    }

    if (unit != "INT" && unit != "int") {
        return std::nullopt;
    }

    return static_cast<uint64_t>(amount * 1000000.0);  // INT to msat
}

std::string format_timestamp(uint64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::gmtime(&time);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace utils

} // namespace invoice
} // namespace lightning
} // namespace intcoin
