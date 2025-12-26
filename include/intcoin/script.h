/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
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
    OP_0 = 0x00,            // Push empty byte array
    OP_1 = 0x51,            // Push value 1
    OP_2 = 0x52,            // Push value 2

    // Control flow
    OP_IF = 0x63,           // Execute if top of stack is true
    OP_NOTIF = 0x64,        // Execute if top of stack is false
    OP_ELSE = 0x67,         // Execute else branch
    OP_ENDIF = 0x68,        // End if block
    OP_VERIFY = 0x69,       // Verify and consume

    // Stack operations
    OP_DROP = 0x75,         // Remove top stack item
    OP_DUP = 0x76,          // Duplicate top stack item
    OP_SWAP = 0x7C,         // Swap top two items
    OP_SIZE = 0x82,         // Push size of top item

    // Logic
    OP_EQUAL = 0x87,        // Check equality
    OP_EQUALVERIFY = 0x88,  // OP_EQUAL followed by OP_VERIFY

    // Special
    OP_RETURN = 0x6A,       // Mark output as unspendable (data storage)

    // Timelock operations (BOLT #3 compatibility)
    OP_CHECKLOCKTIMEVERIFY = 0xB1,   // Verify locktime (CLTV)
    OP_CHECKSEQUENCEVERIFY = 0xB2,   // Verify sequence (CSV)

    // Crypto operations (Bitcoin-compatible opcodes, INTcoin crypto)
    OP_HASH = 0xA9,          // SHA3-256 hash (vs Bitcoin's HASH160)
    OP_CHECKSIG = 0xAC,      // Verify Dilithium signature (vs Bitcoin's ECDSA)
    OP_CHECKMULTISIG = 0xAE, // Verify M-of-N multisig
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

    /// Create BOLT #3 to_local script with CSV delay
    /// @param revocation_pubkey Public key for revocation path (immediate spend by counterparty)
    /// @param local_delayed_pubkey Public key for delayed path (spend after CSV delay)
    /// @param to_self_delay CSV delay in blocks
    static Script CreateToLocalScript(const PublicKey& revocation_pubkey,
                                       const PublicKey& local_delayed_pubkey,
                                       uint16_t to_self_delay);

    /// Create BOLT #3 to_remote script (simple P2PKH for remote party)
    /// @param remote_pubkey Remote party's public key
    static Script CreateToRemoteScript(const PublicKey& remote_pubkey);

    /// Create BOLT #3 offered HTLC script (we offer, remote can claim with preimage or we timeout)
    /// @param revocation_pubkey Revocation public key (for penalty)
    /// @param local_htlcpubkey Local HTLC public key (for timeout path)
    /// @param remote_htlcpubkey Remote HTLC public key (for success path)
    /// @param payment_hash Payment hash (SHA3-256)
    /// @param cltv_expiry CLTV expiry (absolute block height)
    static Script CreateOfferedHTLCScript(const PublicKey& revocation_pubkey,
                                          const PublicKey& local_htlcpubkey,
                                          const PublicKey& remote_htlcpubkey,
                                          const uint256& payment_hash,
                                          uint32_t cltv_expiry);

    /// Create BOLT #3 received HTLC script (remote offers, we can claim with preimage or remote timeouts)
    /// @param revocation_pubkey Revocation public key (for penalty)
    /// @param local_htlcpubkey Local HTLC public key (for success path)
    /// @param remote_htlcpubkey Remote HTLC public key (for timeout path)
    /// @param payment_hash Payment hash (SHA3-256)
    /// @param cltv_expiry CLTV expiry (absolute block height)
    static Script CreateReceivedHTLCScript(const PublicKey& revocation_pubkey,
                                           const PublicKey& local_htlcpubkey,
                                           const PublicKey& remote_htlcpubkey,
                                           const uint256& payment_hash,
                                           uint32_t cltv_expiry);

    /// Create M-of-N multisig script
    /// @param m Required signatures (M)
    /// @param pubkeys Public keys (N total, must have at least M)
    /// @return Multisig script: <M> <pubkey1> ... <pubkeyN> <N> OP_CHECKMULTISIG
    static Script CreateMultisig(uint8_t m, const std::vector<PublicKey>& pubkeys);

    /// Create multisig script_sig (unlocking script)
    /// @param signatures Signatures to include (must be in correct order)
    /// @return Script_sig: OP_0 <sig1> <sig2> ... <sigM> (OP_0 for Bitcoin bug compatibility)
    static Script CreateMultisigScriptSig(const std::vector<Signature>& signatures);

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
