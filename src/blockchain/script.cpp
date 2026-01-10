/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Script System Implementation
 */

#include "intcoin/script.h"
#include "intcoin/util.h"
#include "intcoin/transaction.h"
#include "intcoin/crypto.h"

namespace intcoin {

// ============================================================================
// Script Implementation
// ============================================================================

Script Script::CreateP2PKH(const uint256& pubkey_hash) {
    Script script;
    // P2PKH: OP_DUP OP_HASH <32-byte hash> OP_EQUALVERIFY OP_CHECKSIG
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_DUP));
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_HASH));
    script.bytes.push_back(32);  // Hash length
    script.bytes.insert(script.bytes.end(), pubkey_hash.begin(), pubkey_hash.end());
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_EQUALVERIFY));
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));
    return script;
}

Script Script::CreateP2PK(const PublicKey& pubkey) {
    Script script;
    // P2PK: <pubkey length> <pubkey bytes> OP_CHECKSIG
    // Dilithium3 pubkeys are 1952 bytes
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    // Push 2-byte length for large data (1952 bytes)
    uint16_t len = static_cast<uint16_t>(pubkey.size());
    script.bytes.push_back(len & 0xFF);
    script.bytes.push_back((len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), pubkey.begin(), pubkey.end());
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));
    return script;
}

Script Script::CreateOpReturn(const std::vector<uint8_t>& data) {
    Script script;
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_RETURN));
    script.bytes.insert(script.bytes.end(), data.begin(), data.end());
    return script;
}

Script Script::CreateToLocalScript(const PublicKey& revocation_pubkey,
                                     const PublicKey& local_delayed_pubkey,
                                     uint16_t to_self_delay) {
    // BOLT #3 to_local script:
    // OP_IF
    //     <revocation_pubkey>
    // OP_ELSE
    //     <to_self_delay> OP_CHECKSEQUENCEVERIFY OP_DROP
    //     <local_delayed_pubkey>
    // OP_ENDIF
    // OP_CHECKSIG

    Script script;

    // OP_IF
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_IF));

    // Revocation path: <revocation_pubkey>
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t rev_len = static_cast<uint16_t>(revocation_pubkey.size());
    script.bytes.push_back(rev_len & 0xFF);
    script.bytes.push_back((rev_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), revocation_pubkey.begin(), revocation_pubkey.end());

    // OP_ELSE
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ELSE));

    // Delayed path: <to_self_delay> OP_CHECKSEQUENCEVERIFY OP_DROP <local_delayed_pubkey>
    // Push to_self_delay (2 bytes for values up to 65535)
    script.bytes.push_back(2);  // Push 2 bytes
    script.bytes.push_back(to_self_delay & 0xFF);
    script.bytes.push_back((to_self_delay >> 8) & 0xFF);

    // OP_CHECKSEQUENCEVERIFY
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSEQUENCEVERIFY));

    // OP_DROP (remove CSV delay from stack)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_DROP));

    // Push local_delayed_pubkey
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t local_len = static_cast<uint16_t>(local_delayed_pubkey.size());
    script.bytes.push_back(local_len & 0xFF);
    script.bytes.push_back((local_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), local_delayed_pubkey.begin(), local_delayed_pubkey.end());

    // OP_ENDIF
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ENDIF));

    // OP_CHECKSIG
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));

    return script;
}

Script Script::CreateToRemoteScript(const PublicKey& remote_pubkey) {
    // BOLT #3 to_remote script (simple P2PK for remote party)
    // This is just: <remote_pubkey> OP_CHECKSIG
    return CreateP2PK(remote_pubkey);
}

Script Script::CreateOfferedHTLCScript(const PublicKey& revocation_pubkey,
                                        const PublicKey& local_htlcpubkey,
                                        const PublicKey& remote_htlcpubkey,
                                        const uint256& payment_hash,
                                        uint32_t cltv_expiry) {
    // BOLT #3 offered HTLC script (simplified for INTcoin with Dilithium3)
    // Structure:
    // OP_IF
    //     <revocation_pubkey>  # Revocation path (remote can penalize us)
    // OP_ELSE
    //     OP_IF
    //         OP_HASH <payment_hash> OP_EQUALVERIFY  # Success path (remote with preimage)
    //         <remote_htlcpubkey>
    //     OP_ELSE
    //         <cltv_expiry> OP_CHECKLOCKTIMEVERIFY OP_DROP  # Timeout path (we reclaim)
    //         <local_htlcpubkey>
    //     OP_ENDIF
    // OP_ENDIF
    // OP_CHECKSIG

    Script script;

    // OP_IF (revocation check)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_IF));

    // Revocation path: <revocation_pubkey>
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t rev_len = static_cast<uint16_t>(revocation_pubkey.size());
    script.bytes.push_back(rev_len & 0xFF);
    script.bytes.push_back((rev_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), revocation_pubkey.begin(), revocation_pubkey.end());

    // OP_ELSE
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ELSE));

    // OP_IF (success vs timeout check)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_IF));

    // Success path: OP_HASH <payment_hash> OP_EQUALVERIFY <remote_htlcpubkey>
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_HASH));

    // Push payment hash (32 bytes)
    script.bytes.push_back(32);
    script.bytes.insert(script.bytes.end(), payment_hash.begin(), payment_hash.end());

    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_EQUALVERIFY));

    // Push remote_htlcpubkey
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t remote_len = static_cast<uint16_t>(remote_htlcpubkey.size());
    script.bytes.push_back(remote_len & 0xFF);
    script.bytes.push_back((remote_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), remote_htlcpubkey.begin(), remote_htlcpubkey.end());

    // OP_ELSE (timeout path)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ELSE));

    // Timeout path: <cltv_expiry> OP_CHECKLOCKTIMEVERIFY OP_DROP <local_htlcpubkey>
    script.bytes.push_back(4);  // Push 4 bytes for uint32_t
    script.bytes.push_back(cltv_expiry & 0xFF);
    script.bytes.push_back((cltv_expiry >> 8) & 0xFF);
    script.bytes.push_back((cltv_expiry >> 16) & 0xFF);
    script.bytes.push_back((cltv_expiry >> 24) & 0xFF);

    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKLOCKTIMEVERIFY));
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_DROP));

    // Push local_htlcpubkey
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t local_len = static_cast<uint16_t>(local_htlcpubkey.size());
    script.bytes.push_back(local_len & 0xFF);
    script.bytes.push_back((local_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), local_htlcpubkey.begin(), local_htlcpubkey.end());

    // OP_ENDIF (close success/timeout IF)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ENDIF));

    // OP_ENDIF (close revocation IF)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ENDIF));

    // OP_CHECKSIG (verify signature)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));

    return script;
}

Script Script::CreateReceivedHTLCScript(const PublicKey& revocation_pubkey,
                                         const PublicKey& local_htlcpubkey,
                                         const PublicKey& remote_htlcpubkey,
                                         const uint256& payment_hash,
                                         uint32_t cltv_expiry) {
    // BOLT #3 received HTLC script (simplified for INTcoin with Dilithium3)
    // Structure:
    // OP_IF
    //     <revocation_pubkey>  # Revocation path (remote can penalize us)
    // OP_ELSE
    //     OP_IF
    //         OP_HASH <payment_hash> OP_EQUALVERIFY  # Success path (we claim with preimage)
    //         <local_htlcpubkey>
    //     OP_ELSE
    //         <cltv_expiry> OP_CHECKLOCKTIMEVERIFY OP_DROP  # Timeout path (remote reclaims)
    //         <remote_htlcpubkey>
    //     OP_ENDIF
    // OP_ENDIF
    // OP_CHECKSIG

    Script script;

    // OP_IF (revocation check)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_IF));

    // Revocation path: <revocation_pubkey>
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t rev_len = static_cast<uint16_t>(revocation_pubkey.size());
    script.bytes.push_back(rev_len & 0xFF);
    script.bytes.push_back((rev_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), revocation_pubkey.begin(), revocation_pubkey.end());

    // OP_ELSE
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ELSE));

    // OP_IF (success vs timeout check)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_IF));

    // Success path: OP_HASH <payment_hash> OP_EQUALVERIFY <local_htlcpubkey>
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_HASH));

    // Push payment hash (32 bytes)
    script.bytes.push_back(32);
    script.bytes.insert(script.bytes.end(), payment_hash.begin(), payment_hash.end());

    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_EQUALVERIFY));

    // Push local_htlcpubkey (we claim with preimage)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t local_len = static_cast<uint16_t>(local_htlcpubkey.size());
    script.bytes.push_back(local_len & 0xFF);
    script.bytes.push_back((local_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), local_htlcpubkey.begin(), local_htlcpubkey.end());

    // OP_ELSE (timeout path)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ELSE));

    // Timeout path: <cltv_expiry> OP_CHECKLOCKTIMEVERIFY OP_DROP <remote_htlcpubkey>
    script.bytes.push_back(4);  // Push 4 bytes for uint32_t
    script.bytes.push_back(cltv_expiry & 0xFF);
    script.bytes.push_back((cltv_expiry >> 8) & 0xFF);
    script.bytes.push_back((cltv_expiry >> 16) & 0xFF);
    script.bytes.push_back((cltv_expiry >> 24) & 0xFF);

    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKLOCKTIMEVERIFY));
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_DROP));

    // Push remote_htlcpubkey (they reclaim after timeout)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t remote_len = static_cast<uint16_t>(remote_htlcpubkey.size());
    script.bytes.push_back(remote_len & 0xFF);
    script.bytes.push_back((remote_len >> 8) & 0xFF);
    script.bytes.insert(script.bytes.end(), remote_htlcpubkey.begin(), remote_htlcpubkey.end());

    // OP_ENDIF (close success/timeout IF)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ENDIF));

    // OP_ENDIF (close revocation IF)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ENDIF));

    // OP_CHECKSIG (verify signature)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));

    return script;
}

Script Script::CreateMultisig(uint8_t m, const std::vector<PublicKey>& pubkeys) {
    // Create M-of-N multisig script
    // Format: <M> <pubkey1> <pubkey2> ... <pubkeyN> <N> OP_CHECKMULTISIG
    // For Lightning funding: 2-of-2 multisig

    if (m == 0 || m > pubkeys.size()) {
        // Invalid M value - return empty script
        return Script();
    }

    if (pubkeys.size() > 20) {
        // Too many keys (Bitcoin limit is 20)
        return Script();
    }

    Script script;

    // Push M (required signatures)
    if (m == 1) {
        script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_1));
    } else if (m == 2) {
        script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_2));
    } else {
        // For M > 2, push as small integer (OP_1 = 0x51, so OP_3 = 0x53, etc.)
        script.bytes.push_back(0x50 + m);  // OP_1 through OP_16
    }

    // Push all public keys
    for (const auto& pubkey : pubkeys) {
        script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
        uint16_t len = static_cast<uint16_t>(pubkey.size());
        script.bytes.push_back(len & 0xFF);
        script.bytes.push_back((len >> 8) & 0xFF);
        script.bytes.insert(script.bytes.end(), pubkey.begin(), pubkey.end());
    }

    // Push N (total public keys)
    uint8_t n = static_cast<uint8_t>(pubkeys.size());
    if (n == 1) {
        script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_1));
    } else if (n == 2) {
        script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_2));
    } else {
        script.bytes.push_back(0x50 + n);  // OP_1 through OP_16
    }

    // OP_CHECKMULTISIG
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKMULTISIG));

    return script;
}

Script Script::CreateMultisigScriptSig(const std::vector<Signature>& signatures) {
    // Create multisig unlocking script (script_sig)
    // Format: OP_0 <sig1> <sig2> ... <sigM>
    // OP_0 is required due to Bitcoin off-by-one bug (kept for compatibility)

    Script script;

    // Push OP_0 (dummy element for Bitcoin bug compatibility)
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_0));

    // Push all signatures
    for (const auto& signature : signatures) {
        script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
        uint16_t len = static_cast<uint16_t>(signature.size());
        script.bytes.push_back(len & 0xFF);
        script.bytes.push_back((len >> 8) & 0xFF);
        script.bytes.insert(script.bytes.end(), signature.begin(), signature.end());
    }

    return script;
}

bool Script::IsP2PKH() const {
    // P2PKH pattern: OP_DUP OP_HASH <32> <32-byte hash> OP_EQUALVERIFY OP_CHECKSIG
    // Total: 1 + 1 + 1 + 32 + 1 + 1 = 37 bytes
    if (bytes.size() != 37) return false;

    return bytes[0] == static_cast<uint8_t>(OpCode::OP_DUP) &&
           bytes[1] == static_cast<uint8_t>(OpCode::OP_HASH) &&
           bytes[2] == 32 &&
           bytes[35] == static_cast<uint8_t>(OpCode::OP_EQUALVERIFY) &&
           bytes[36] == static_cast<uint8_t>(OpCode::OP_CHECKSIG);
}

bool Script::IsP2PK() const {
    // P2PK pattern: OP_PUSHDATA <2-byte length> <pubkey bytes> OP_CHECKSIG
    // For Dilithium3: 1 + 2 + 1952 + 1 = 1956 bytes
    if (bytes.size() != 1956) return false;

    if (bytes[0] != static_cast<uint8_t>(OpCode::OP_PUSHDATA)) return false;

    // Check length bytes (1952 in little-endian)
    uint16_t len = bytes[1] | (bytes[2] << 8);
    if (len != 1952) return false;

    // Check final OP_CHECKSIG
    return bytes[1955] == static_cast<uint8_t>(OpCode::OP_CHECKSIG);
}

bool Script::IsOpReturn() const {
    if (bytes.empty()) return false;
    return bytes[0] == static_cast<uint8_t>(OpCode::OP_RETURN);
}

std::optional<uint256> Script::GetP2PKHHash() const {
    if (!IsP2PKH()) return std::nullopt;

    // Extract 32-byte hash at bytes[3..35]
    uint256 hash;
    std::copy(bytes.begin() + 3, bytes.begin() + 35, hash.begin());
    return hash;
}

std::optional<PublicKey> Script::GetP2PKPublicKey() const {
    if (!IsP2PK()) return std::nullopt;

    // Extract 1952-byte pubkey at bytes[3..1955]
    PublicKey pubkey;
    std::copy(bytes.begin() + 3, bytes.begin() + 1955, pubkey.begin());
    return pubkey;
}

std::string Script::ToString() const {
    if (bytes.empty()) return "(empty)";

    std::string result;
    size_t i = 0;

    while (i < bytes.size()) {
        if (!result.empty()) result += " ";

        uint8_t opcode = bytes[i];

        // Decode opcode
        if (opcode == static_cast<uint8_t>(OpCode::OP_PUSHDATA)) {
            result += "OP_PUSHDATA";
            i++;
            if (i + 1 < bytes.size()) {
                uint16_t len = bytes[i] | (bytes[i+1] << 8);
                result += "[" + std::to_string(len) + "]";
                i += 2 + len;
            }
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_DUP)) {
            result += "OP_DUP";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_HASH)) {
            result += "OP_HASH";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_CHECKSIG)) {
            result += "OP_CHECKSIG";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_DROP)) {
            result += "OP_DROP";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_SWAP)) {
            result += "OP_SWAP";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_EQUAL)) {
            result += "OP_EQUAL";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_VERIFY)) {
            result += "OP_VERIFY";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_EQUALVERIFY)) {
            result += "OP_EQUALVERIFY";
            i++;
        } else if (opcode == static_cast<uint8_t>(OpCode::OP_RETURN)) {
            result += "OP_RETURN";
            i++;
        } else if (opcode <= 75) {
            // Direct push (length byte followed by data)
            size_t len = opcode;
            result += "PUSH[" + std::to_string(len) + "]";
            i += 1 + len;
        } else {
            result += "OP_UNKNOWN[0x" + BytesToHex(std::vector<uint8_t>{opcode}) + "]";
            i++;
        }
    }

    return result;
}

// ============================================================================
// Script Execution
// ============================================================================

/// Stack-based virtual machine for script execution
class ScriptVM {
private:
    std::vector<std::vector<uint8_t>> stack;
    const Transaction* tx;
    size_t input_index;
    const Script* script_pubkey;  // Previous output's script_pubkey (for signature verification)

public:
    ScriptVM(const Transaction* transaction, size_t input_idx, const Script* prev_script_pubkey)
        : tx(transaction), input_index(input_idx), script_pubkey(prev_script_pubkey) {
    }

    /// Execute a script on this VM
    ScriptExecutionResult Execute(const Script& script) {
        const auto& bytes = script.bytes;
        size_t pc = 0;  // Program counter

        while (pc < bytes.size()) {
            uint8_t opcode = bytes[pc];

            // OP_PUSHDATA: Push large data onto stack
            if (opcode == static_cast<uint8_t>(OpCode::OP_PUSHDATA)) {
                pc++;
                if (pc + 1 >= bytes.size()) {
                    return ScriptExecutionResult::Error("OP_PUSHDATA: truncated length");
                }
                uint16_t len = bytes[pc] | (bytes[pc+1] << 8);
                pc += 2;
                if (pc + len > bytes.size()) {
                    return ScriptExecutionResult::Error("OP_PUSHDATA: truncated data");
                }
                std::vector<uint8_t> data(bytes.begin() + pc, bytes.begin() + pc + len);
                stack.push_back(data);
                pc += len;
            }
            // OP_DUP: Duplicate top stack item
            else if (opcode == static_cast<uint8_t>(OpCode::OP_DUP)) {
                if (stack.empty()) {
                    return ScriptExecutionResult::Error("OP_DUP: stack underflow");
                }
                stack.push_back(stack.back());
                pc++;
            }
            // OP_HASH: SHA3-256 hash of top stack item
            else if (opcode == static_cast<uint8_t>(OpCode::OP_HASH)) {
                if (stack.empty()) {
                    return ScriptExecutionResult::Error("OP_HASH: stack underflow");
                }
                auto data = stack.back();
                stack.pop_back();
                uint256 hash = SHA3::Hash(data);
                stack.push_back(std::vector<uint8_t>(hash.begin(), hash.end()));
                pc++;
            }
            // OP_CHECKSIG: Verify Dilithium signature
            // Stack before: [signature, pubkey]
            // Stack after: [1] or [0] (verification result)
            else if (opcode == static_cast<uint8_t>(OpCode::OP_CHECKSIG)) {
                if (stack.size() < 2) {
                    return ScriptExecutionResult::Error("OP_CHECKSIG: stack underflow");
                }
                // Pop in correct order: pubkey is on top, signature below
                auto pubkey_bytes = stack.back();
                stack.pop_back();
                auto signature_bytes = stack.back();
                stack.pop_back();

                // Verify Dilithium signature
                if (pubkey_bytes.size() != 1952) {
                    stack.push_back({0});  // Push false
                } else if (signature_bytes.size() != 3309) {  // Dilithium3 signature size
                    stack.push_back({0});  // Push false
                } else if (!script_pubkey) {
                    return ScriptExecutionResult::Error("OP_CHECKSIG: no script_pubkey provided");
                } else {
                    // Copy vectors to arrays
                    PublicKey pubkey;
                    Signature signature;
                    std::copy(pubkey_bytes.begin(), pubkey_bytes.end(), pubkey.begin());
                    std::copy(signature_bytes.begin(), signature_bytes.end(), signature.begin());

                    // Get transaction hash for signing with the previous output's script_pubkey
                    // This ensures the same hash is used during both signing and verification
                    uint256 tx_hash = tx->GetHashForSigning(SIGHASH_ALL, input_index, *script_pubkey);

                    auto result = DilithiumCrypto::VerifyHash(tx_hash, signature, pubkey);
                    stack.push_back(result.IsOk() ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0});
                }
                pc++;
            }
            // OP_CHECKMULTISIG: Verify M-of-N multisig
            // Stack before: [0 sig1 sig2 ... sigM M pubkey1 ... pubkeyN N]
            // Stack after: [1] or [0] (verification result)
            // Note: Extra OP_0 is required due to Bitcoin bug kept for compatibility
            else if (opcode == static_cast<uint8_t>(OpCode::OP_CHECKMULTISIG)) {
                if (stack.empty()) {
                    return ScriptExecutionResult::Error("OP_CHECKMULTISIG: stack underflow (N)");
                }

                // Pop N (number of public keys)
                auto n_bytes = stack.back();
                stack.pop_back();
                if (n_bytes.empty() || n_bytes.size() > 1) {
                    return ScriptExecutionResult::Error("OP_CHECKMULTISIG: invalid N");
                }
                uint8_t n = n_bytes[0];
                if (n > 0x51) n -= 0x50;  // Convert OP_1..OP_16 to 1..16

                // Pop N public keys
                if (stack.size() < n) {
                    return ScriptExecutionResult::Error("OP_CHECKMULTISIG: not enough pubkeys on stack");
                }
                std::vector<std::vector<uint8_t>> pubkeys;
                for (uint8_t i = 0; i < n; i++) {
                    pubkeys.push_back(stack.back());
                    stack.pop_back();
                }
                std::reverse(pubkeys.begin(), pubkeys.end());  // Stack is LIFO

                // Pop M (required signatures)
                if (stack.empty()) {
                    return ScriptExecutionResult::Error("OP_CHECKMULTISIG: stack underflow (M)");
                }
                auto m_bytes = stack.back();
                stack.pop_back();
                if (m_bytes.empty() || m_bytes.size() > 1) {
                    return ScriptExecutionResult::Error("OP_CHECKMULTISIG: invalid M");
                }
                uint8_t m = m_bytes[0];
                if (m > 0x51) m -= 0x50;  // Convert OP_1..OP_16 to 1..16

                if (m > n) {
                    stack.push_back({0});  // Invalid: M > N
                    pc++;
                    continue;
                }

                // Pop M signatures
                if (stack.size() < m) {
                    return ScriptExecutionResult::Error("OP_CHECKMULTISIG: not enough sigs on stack");
                }
                std::vector<std::vector<uint8_t>> sigs;
                for (uint8_t i = 0; i < m; i++) {
                    sigs.push_back(stack.back());
                    stack.pop_back();
                }
                std::reverse(sigs.begin(), sigs.end());  // Stack is LIFO

                // Pop dummy element (Bitcoin bug compatibility)
                if (stack.empty()) {
                    return ScriptExecutionResult::Error("OP_CHECKMULTISIG: missing dummy element");
                }
                stack.pop_back();  // Remove OP_0 dummy

                // Verify signatures
                // Each signature must match one of the public keys, in order
                size_t sig_idx = 0;
                size_t pubkey_idx = 0;

                while (sig_idx < sigs.size() && pubkey_idx < pubkeys.size()) {
                    if (sigs[sig_idx].size() != 3309 || pubkeys[pubkey_idx].size() != 1952) {
                        pubkey_idx++;
                        continue;
                    }

                    // Try to verify signature with current pubkey
                    PublicKey pubkey;
                    Signature signature;
                    std::copy(pubkeys[pubkey_idx].begin(), pubkeys[pubkey_idx].end(), pubkey.begin());
                    std::copy(sigs[sig_idx].begin(), sigs[sig_idx].end(), signature.begin());

                    uint256 tx_hash = tx->GetHashForSigning(SIGHASH_ALL, input_index, *script_pubkey);
                    auto result = DilithiumCrypto::VerifyHash(tx_hash, signature, pubkey);

                    if (result.IsOk()) {
                        // Signature verified with this pubkey, move to next sig
                        sig_idx++;
                    }
                    // Move to next pubkey regardless
                    pubkey_idx++;
                }

                // All signatures must have been verified
                if (sig_idx == sigs.size()) {
                    stack.push_back({1});  // All signatures valid
                } else {
                    stack.push_back({0});  // Not all signatures verified
                }
                pc++;
            }
            // OP_DROP: Remove top stack item
            else if (opcode == static_cast<uint8_t>(OpCode::OP_DROP)) {
                if (stack.empty()) {
                    return ScriptExecutionResult::Error("OP_DROP: stack underflow");
                }
                stack.pop_back();
                pc++;
            }
            // OP_SWAP: Swap top two stack items
            else if (opcode == static_cast<uint8_t>(OpCode::OP_SWAP)) {
                if (stack.size() < 2) {
                    return ScriptExecutionResult::Error("OP_SWAP: stack underflow");
                }
                std::swap(stack[stack.size()-1], stack[stack.size()-2]);
                pc++;
            }
            // OP_EQUAL: Check if top two items are equal
            else if (opcode == static_cast<uint8_t>(OpCode::OP_EQUAL)) {
                if (stack.size() < 2) {
                    return ScriptExecutionResult::Error("OP_EQUAL: stack underflow");
                }
                auto a = stack.back();
                stack.pop_back();
                auto b = stack.back();
                stack.pop_back();
                stack.push_back((a == b) ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0});
                pc++;
            }
            // OP_VERIFY: Verify top item is true, fail otherwise
            else if (opcode == static_cast<uint8_t>(OpCode::OP_VERIFY)) {
                if (stack.empty()) {
                    return ScriptExecutionResult::Error("OP_VERIFY: stack underflow");
                }
                auto value = stack.back();
                stack.pop_back();
                if (value.empty() || value[0] == 0) {
                    return ScriptExecutionResult::Error("OP_VERIFY: failed");
                }
                pc++;
            }
            // OP_EQUALVERIFY: OP_EQUAL + OP_VERIFY
            else if (opcode == static_cast<uint8_t>(OpCode::OP_EQUALVERIFY)) {
                if (stack.size() < 2) {
                    return ScriptExecutionResult::Error("OP_EQUALVERIFY: stack underflow");
                }
                auto a = stack.back();
                stack.pop_back();
                auto b = stack.back();
                stack.pop_back();
                if (a != b) {
                    return ScriptExecutionResult::Error("OP_EQUALVERIFY: not equal");
                }
                pc++;
            }
            // OP_RETURN: Always fails (unspendable)
            else if (opcode == static_cast<uint8_t>(OpCode::OP_RETURN)) {
                return ScriptExecutionResult::Error("OP_RETURN: unspendable output");
            }
            // Direct push (opcode <= 75 means push that many bytes)
            else if (opcode > 0 && opcode <= 75) {
                size_t len = opcode;
                pc++;
                if (pc + len > bytes.size()) {
                    return ScriptExecutionResult::Error("Direct push: truncated data");
                }
                std::vector<uint8_t> data(bytes.begin() + pc, bytes.begin() + pc + len);
                stack.push_back(data);
                pc += len;
            }
            else {
                return ScriptExecutionResult::Error("Unknown opcode: 0x" +
                    BytesToHex(std::vector<uint8_t>{opcode}));
            }
        }

        return ScriptExecutionResult::Ok();
    }

    /// Check if execution succeeded (non-empty stack with true value on top)
    bool IsSuccess() const {
        if (stack.empty()) return false;
        const auto& top = stack.back();
        if (top.empty()) return false;
        return top[0] != 0;
    }
};

ScriptExecutionResult ExecuteScript(const Script& script_sig,
                                   const Script& script_pubkey,
                                   const class Transaction& tx,
                                   size_t input_index) {
    // Create VM with transaction context and prev_scriptpubkey for signature verification
    ScriptVM vm(&tx, input_index, &script_pubkey);

    // Phase 1: Execute script_sig (unlocking script)
    auto result = vm.Execute(script_sig);
    if (!result.success) {
        return ScriptExecutionResult::Error("script_sig execution failed: " + result.error);
    }

    // Phase 2: Execute script_pubkey (locking script)
    result = vm.Execute(script_pubkey);
    if (!result.success) {
        return ScriptExecutionResult::Error("script_pubkey execution failed: " + result.error);
    }

    // Phase 3: Verify final stack state
    if (!vm.IsSuccess()) {
        return ScriptExecutionResult::Error("Script failed: stack is empty or top is false");
    }

    return ScriptExecutionResult::Ok();
}

} // namespace intcoin
