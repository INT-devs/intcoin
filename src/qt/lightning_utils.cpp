// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network Qt Utility Functions Implementation

#include "lightning_utils.h"
#include <QRegularExpression>
#include <cctype>

namespace intcoin {
namespace qt {
namespace lightning_utils {

bool is_valid_hex(const QString& hex_str, size_t expected_length) {
    // Check if string is empty
    if (hex_str.isEmpty()) {
        return false;
    }

    // Check if all characters are hex digits
    QRegularExpression hex_regex("^[0-9a-fA-F]+$");
    if (!hex_regex.match(hex_str).hasMatch()) {
        return false;
    }

    // Check length if specified
    if (expected_length > 0) {
        // Hex string should be 2 chars per byte
        if (hex_str.length() != static_cast<int>(expected_length * 2)) {
            return false;
        }
    }

    // Must be even length (pairs of hex digits)
    if (hex_str.length() % 2 != 0) {
        return false;
    }

    return true;
}

QString bytes_to_hex(const uint8_t* data, size_t length) {
    QString hex_str;
    hex_str.reserve(static_cast<int>(length * 2));

    for (size_t i = 0; i < length; i++) {
        hex_str.append(QString("%1").arg(data[i], 2, 16, QChar('0')));
    }

    return hex_str;
}

std::vector<uint8_t> hex_to_bytes(const QString& hex_str) {
    std::vector<uint8_t> bytes;

    if (!is_valid_hex(hex_str)) {
        return bytes;
    }

    bytes.reserve(hex_str.length() / 2);

    for (int i = 0; i < hex_str.length(); i += 2) {
        QString byte_str = hex_str.mid(i, 2);
        bool ok;
        uint8_t byte = static_cast<uint8_t>(byte_str.toUInt(&ok, 16));
        if (!ok) {
            return std::vector<uint8_t>(); // Return empty on error
        }
        bytes.push_back(byte);
    }

    return bytes;
}

std::optional<Hash256> hex_to_hash256(const QString& hex_str) {
    // Hash256 is 32 bytes = 64 hex characters
    if (!is_valid_hex(hex_str, 32)) {
        return std::nullopt;
    }

    std::vector<uint8_t> bytes = hex_to_bytes(hex_str);
    if (bytes.size() != 32) {
        return std::nullopt;
    }

    Hash256 hash;
    std::copy(bytes.begin(), bytes.end(), hash.begin());

    return hash;
}

QString hash256_to_hex(const Hash256& hash) {
    return bytes_to_hex(hash.data(), hash.size());
}

std::optional<DilithiumPubKey> hex_to_dilithium_pubkey(const QString& hex_str) {
    // Dilithium5 public key is 2592 bytes = 5184 hex characters
    // But we'll accept any valid hex and let the crypto module validate
    if (!is_valid_hex(hex_str)) {
        return std::nullopt;
    }

    std::vector<uint8_t> bytes = hex_to_bytes(hex_str);
    if (bytes.empty()) {
        return std::nullopt;
    }

    // Check if the size matches expected Dilithium5 public key size
    const size_t DILITHIUM5_PUBKEY_SIZE = 2592;
    if (bytes.size() != DILITHIUM5_PUBKEY_SIZE) {
        return std::nullopt;
    }

    // Copy bytes to DilithiumPubKey (which is std::array<uint8_t, 2592>)
    DilithiumPubKey pubkey;
    std::copy(bytes.begin(), bytes.end(), pubkey.begin());

    return pubkey;
}

QString dilithium_pubkey_to_hex(const DilithiumPubKey& pubkey) {
    // Convert public key bytes to hex (DilithiumPubKey is std::array)
    return bytes_to_hex(pubkey.data(), pubkey.size());
}

std::optional<lightning::invoice::Invoice> decode_invoice(const QString& invoice_str) {
    // Decode the BOLT #11 invoice using the invoice module
    return lightning::invoice::Invoice::decode(invoice_str.toStdString());
}

QString format_invoice_details(const lightning::invoice::Invoice& invoice) {
    QString details;

    // Amount
    if (invoice.amount_msat.has_value()) {
        double amount_int = static_cast<double>(*invoice.amount_msat) / 100000000000.0;
        details += QString("Amount: %1 INT (%2 msat)\n")
            .arg(amount_int, 0, 'f', 11)
            .arg(*invoice.amount_msat);
    } else {
        details += "Amount: Any amount\n";
    }

    // Description
    if (!invoice.description.empty()) {
        details += QString("Description: %1\n")
            .arg(QString::fromStdString(invoice.description));
    }

    // Payment hash
    details += QString("Payment Hash: %1\n")
        .arg(bytes_to_hex(invoice.payment_hash.data(), invoice.payment_hash.size()));

    // Node ID
    details += QString("Payee Node ID: %1...\n")
        .arg(bytes_to_hex(invoice.node_id.data(), 32));

    // Expiry
    uint64_t expiry_timestamp = invoice.get_expiry_timestamp();
    details += QString("Expires: %1 (%2 seconds)\n")
        .arg(expiry_timestamp)
        .arg(invoice.expiry_seconds);

    // Min CLTV expiry
    details += QString("Min Final CLTV: %1 blocks\n")
        .arg(invoice.min_final_cltv_expiry);

    // Payment secret
    if (invoice.payment_secret.has_value()) {
        details += QString("Payment Secret: %1\n")
            .arg(bytes_to_hex(invoice.payment_secret->data(), invoice.payment_secret->size()));
    }

    // Route hints
    if (!invoice.route_hints.empty()) {
        details += QString("Route Hints: %1\n").arg(invoice.route_hints.size());
    }

    // Features
    if (!invoice.features.empty()) {
        details += QString("Feature Bits: %1 bytes\n").arg(invoice.features.size());
    }

    // Status
    if (invoice.is_expired()) {
        details += "\nStatus: EXPIRED\n";
    } else {
        details += "\nStatus: Valid\n";
    }

    return details;
}

} // namespace lightning_utils
} // namespace qt
} // namespace intcoin
