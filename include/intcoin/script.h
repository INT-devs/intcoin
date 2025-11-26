/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Script System (Simple UTXO scripts)
 */

#ifndef INTCOIN_SCRIPT_H
#define INTCOIN_SCRIPT_H

#include "types.h"
#include <vector>
#include <string>

namespace intcoin {

// ============================================================================
// Script Opcodes
// ============================================================================

enum class OpCode : uint8_t {
    // Push data
    OP_PUSHDATA = 0x01,     // Push N bytes

    // Crypto operations
    OP_DUP = 0x10,           // Duplicate top stack item
    OP_HASH = 0x11,          // SHA3-256 hash
    OP_CHECKSIG = 0x12,      // Verify Dilithium signature

    // Stack operations
    OP_DROP = 0x20,          // Remove top stack item
    OP_SWAP = 0x21,          // Swap top two items

    // Logic
    OP_EQUAL = 0x30,         // Check equality
    OP_VERIFY = 0x31,        // Verify and consume

    // Special
    OP_RETURN = 0xFF,        // Mark output as unspendable (data storage)
};

// ============================================================================
// Script Types
// ============================================================================

enum class ScriptType {
    UNKNOWN,        // Unknown script type
    P2PKH,          // Pay-to-Public-Key-Hash
    P2PK,           // Pay-to-Public-Key
    OP_RETURN,      // Data storage (unspendable)
    MULTISIG,       // Multisignature (future)
};

// ============================================================================
// Script
// ============================================================================

class Script {
public:
    /// Raw script bytes
    std::vector<uint8_t> bytes;

    /// Default constructor (empty script)
    Script() = default;

    /// Constructor from bytes
    explicit Script(std::vector<uint8_t> data) : bytes(std::move(data)) {}

    /// Create P2PKH script (Pay-to-Public-Key-Hash)
    static Script CreateP2PKH(const uint256& pubkey_hash);

    /// Create P2PK script (Pay-to-Public-Key)
    static Script CreateP2PK(const PublicKey& pubkey);

    /// Create OP_RETURN script (data storage)
    static Script CreateOpReturn(const std::vector<uint8_t>& data);

    /// Check if this is a P2PKH script
    bool IsP2PKH() const;

    /// Check if this is a P2PK script
    bool IsP2PK() const;

    /// Check if this is an OP_RETURN script
    bool IsOpReturn() const;

    /// Extract public key hash from P2PKH script
    std::optional<uint256> GetP2PKHHash() const;

    /// Extract public key from P2PK script
    std::optional<PublicKey> GetP2PKPublicKey() const;

    /// Get size in bytes
    size_t GetSize() const { return bytes.size(); }

    /// Check if empty
    bool IsEmpty() const { return bytes.empty(); }

    /// Serialize
    std::vector<uint8_t> Serialize() const { return bytes; }

    /// Deserialize
    static Script Deserialize(const std::vector<uint8_t>& data) {
        return Script(data);
    }

    /// Convert to human-readable string
    std::string ToString() const;
};

// ============================================================================
// Script Execution
// ============================================================================

/// Script execution result
struct ScriptExecutionResult {
    bool success;
    std::string error;

    static ScriptExecutionResult Ok() {
        return {true, ""};
    }

    static ScriptExecutionResult Error(std::string err) {
        return {false, std::move(err)};
    }
};

/// Execute script (simplified interpreter)
ScriptExecutionResult ExecuteScript(const Script& script_sig,
                                   const Script& script_pubkey,
                                   const class Transaction& tx,
                                   size_t input_index);

} // namespace intcoin

#endif // INTCOIN_SCRIPT_H
