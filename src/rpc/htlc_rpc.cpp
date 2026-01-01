// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/htlc.h>
#include <intcoin/rpc.h>
#include <intcoin/util.h>

#include <sstream>
#include <iomanip>

namespace intcoin {
namespace rpc {

using namespace htlc;

// Global HTLC manager (should be part of blockchain state in production)
static HTLCManager g_htlc_manager;

// Helper: Convert bytes to hex string
static std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Helper: Convert hex string to bytes
static Result<std::vector<uint8_t>> HexToBytes(const std::string& hex) {
    if (hex.length() % 2 != 0) {
        return Result<std::vector<uint8_t>>::Error("Hex string must have even length");
    }

    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
            bytes.push_back(byte);
        } catch (...) {
            return Result<std::vector<uint8_t>>::Error("Invalid hex string");
        }
    }
    return Result<std::vector<uint8_t>>::Ok(bytes);
}

// HTLC RPC Methods
class HTLCRPC {
public:
    static void RegisterMethods(RPCServer& server) {
        server.RegisterMethod({
            "createhtlc",
            "Create a new Hash Time-Locked Contract\n"
            "Arguments: [recipient_pubkey, refund_pubkey, hash_lock, locktime, amount]\n"
            "Returns: {htlc_script, locktime, hash_lock}",
            {"recipient_pubkey", "refund_pubkey", "hash_lock", "locktime", "amount"},
            false,
            [](const JSONValue& params) { return createhtlc(params); }
        });

        server.RegisterMethod({
            "verifypreimage",
            "Verify a preimage matches a hash\n"
            "Arguments: [preimage, hash]\n"
            "Returns: {valid: bool}",
            {"preimage", "hash"},
            false,
            [](const JSONValue& params) { return verifypreimage(params); }
        });

        LogF(LogLevel::INFO, "Registered HTLC RPC methods");
    }

    static JSONValue createhtlc(const JSONValue& params) {
        if (!params.IsArray() || params.Size() < 5) {
            throw std::runtime_error("Usage: createhtlc <recipient_pubkey> <refund_pubkey> <hash_lock> <locktime> <amount>");
        }

        auto recipient_pubkey_result = HexToBytes(params[0].GetString());
        if (recipient_pubkey_result.IsError()) {
            throw std::runtime_error("Invalid recipient_pubkey");
        }

        auto refund_pubkey_result = HexToBytes(params[1].GetString());
        if (refund_pubkey_result.IsError()) {
            throw std::runtime_error("Invalid refund_pubkey");
        }

        auto hash_lock_result = HexToBytes(params[2].GetString());
        if (hash_lock_result.IsError() || hash_lock_result.GetValue().size() != 32) {
            throw std::runtime_error("Invalid hash_lock (must be 32 bytes)");
        }

        uint64_t locktime = params[3].GetInt();
        uint64_t amount = params[4].GetInt();

        HTLCParameters htlc_params;
        htlc_params.recipient_pubkey = recipient_pubkey_result.GetValue();
        htlc_params.refund_pubkey = refund_pubkey_result.GetValue();
        htlc_params.hash_lock = hash_lock_result.GetValue();
        htlc_params.locktime = locktime;
        htlc_params.hash_algorithm = HTLCHashAlgorithm::SHA3_256;
        htlc_params.is_block_height = locktime < 500000000;

        Script htlc_script = HTLCScript::CreateHTLCScript(htlc_params);

        std::map<std::string, JSONValue> result;
        result["htlc_script"] = JSONValue(BytesToHex(htlc_script.bytes));
        result["amount"] = JSONValue(static_cast<int64_t>(amount));
        result["locktime"] = JSONValue(static_cast<int64_t>(locktime));
        result["hash_lock"] = JSONValue(params[2].GetString());

        return JSONValue(result);
    }

    static JSONValue verifypreimage(const JSONValue& params) {
        if (!params.IsArray() || params.Size() < 2) {
            throw std::runtime_error("Usage: verifypreimage <preimage> <hash>");
        }

        auto preimage_result = HexToBytes(params[0].GetString());
        if (preimage_result.IsError()) {
            throw std::runtime_error("Invalid preimage");
        }

        auto hash_result = HexToBytes(params[1].GetString());
        if (hash_result.IsError()) {
            throw std::runtime_error("Invalid hash");
        }

        bool valid = HTLCScript::VerifyPreimage(
            preimage_result.GetValue(),
            hash_result.GetValue(),
            HTLCHashAlgorithm::SHA3_256
        );

        std::map<std::string, JSONValue> result;
        result["valid"] = JSONValue(valid);

        return JSONValue(result);
    }
};

// Export registration function
void RegisterHTLCRPCCommands(RPCServer& server) {
    HTLCRPC::RegisterMethods(server);
}

}  // namespace rpc
}  // namespace intcoin
