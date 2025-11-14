// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network Qt Utility Functions

#ifndef INTCOIN_QT_LIGHTNING_UTILS_H
#define INTCOIN_QT_LIGHTNING_UTILS_H

#include "intcoin/primitives.h"
#include "intcoin/crypto.h"
#include "intcoin/lightning_invoice.h"
#include <QString>
#include <optional>

namespace intcoin {
namespace qt {
namespace lightning_utils {

/**
 * Convert hex string to Hash256
 * @param hex_str Hexadecimal string (64 characters for 32 bytes)
 * @return Hash256 if valid, nullopt otherwise
 */
std::optional<Hash256> hex_to_hash256(const QString& hex_str);

/**
 * Convert Hash256 to hex string
 * @param hash Hash256 value
 * @return Hexadecimal string representation
 */
QString hash256_to_hex(const Hash256& hash);

/**
 * Convert hex string to DilithiumPubKey
 * @param hex_str Hexadecimal string (variable length for Dilithium5 public key)
 * @return DilithiumPubKey if valid, nullopt otherwise
 */
std::optional<DilithiumPubKey> hex_to_dilithium_pubkey(const QString& hex_str);

/**
 * Convert DilithiumPubKey to hex string
 * @param pubkey DilithiumPubKey value
 * @return Hexadecimal string representation
 */
QString dilithium_pubkey_to_hex(const DilithiumPubKey& pubkey);

/**
 * Validate hex string format
 * @param hex_str String to validate
 * @param expected_length Expected length in bytes (0 = any length)
 * @return true if valid hex string
 */
bool is_valid_hex(const QString& hex_str, size_t expected_length = 0);

/**
 * Convert byte array to hex string
 * @param data Byte array
 * @param length Length of data
 * @return Hexadecimal string
 */
QString bytes_to_hex(const uint8_t* data, size_t length);

/**
 * Convert hex string to byte vector
 * @param hex_str Hexadecimal string
 * @return Byte vector if valid, empty vector otherwise
 */
std::vector<uint8_t> hex_to_bytes(const QString& hex_str);

/**
 * Decode and validate Lightning invoice (BOLT #11)
 * @param invoice_str Encoded invoice string
 * @return Decoded invoice if valid, nullopt otherwise
 */
std::optional<lightning::invoice::Invoice> decode_invoice(const QString& invoice_str);

/**
 * Format invoice for display
 * @param invoice Decoded invoice
 * @return Human-readable formatted string
 */
QString format_invoice_details(const lightning::invoice::Invoice& invoice);

} // namespace lightning_utils
} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_LIGHTNING_UTILS_H
