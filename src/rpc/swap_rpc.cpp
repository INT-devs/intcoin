// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/atomic_swap.h>
#include <intcoin/rpc.h>
#include <intcoin/util.h>

#include <sstream>
#include <iomanip>

namespace intcoin {
namespace rpc {

using namespace atomic_swap;

// Global atomic swap coordinator instance
static AtomicSwapCoordinator g_swap_coordinator;

// Helper: Convert bytes to hex string
static std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Helper: Convert uint256 to hex string
static std::string Uint256ToHex(const uint256& hash) {
    return BytesToHex(std::vector<uint8_t>(hash.begin(), hash.end()));
}

// Helper: Parse swap chain from string
static Result<SwapChain> ParseSwapChain(const std::string& chain_str) {
    if (chain_str == "intcoin" || chain_str == "INT") {
        return Result<SwapChain>::Ok(SwapChain::INTCOIN);
    } else if (chain_str == "bitcoin" || chain_str == "BTC") {
        return Result<SwapChain>::Ok(SwapChain::BITCOIN);
    } else if (chain_str == "litecoin" || chain_str == "LTC") {
        return Result<SwapChain>::Ok(SwapChain::LITECOIN);
    } else if (chain_str == "testnet_int") {
        return Result<SwapChain>::Ok(SwapChain::TESTNET_INT);
    } else if (chain_str == "testnet_btc") {
        return Result<SwapChain>::Ok(SwapChain::TESTNET_BTC);
    } else if (chain_str == "testnet_ltc") {
        return Result<SwapChain>::Ok(SwapChain::TESTNET_LTC);
    }
    return Result<SwapChain>::Error("Invalid chain: " + chain_str);
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

// Atomic Swap RPC Methods
class AtomicSwapRPC {
public:
    static void RegisterMethods(RPCServer& server) {
        server.RegisterMethod({
            "createswap",
            "Create a new atomic swap offer\n"
            "Arguments: [initiator_chain, participant_chain, initiator_amount, participant_amount, initiator_pubkey, locktime_hours]\n"
            "Returns: {swap_id, payment_hash, initiator_locktime, participant_locktime}",
            {"initiator_chain", "participant_chain", "initiator_amount", "participant_amount", "initiator_pubkey", "locktime_hours"},
            false,
            [](const JSONValue& params) { return createswap(params); }
        });

        server.RegisterMethod({
            "listswaps",
            "List all atomic swaps\n"
            "Arguments: none\n"
            "Returns: Array of swap summaries",
            {},
            false,
            [](const JSONValue& params) { return listswaps(params); }
        });

        LogF(LogLevel::INFO, "Registered Atomic Swap RPC methods");
    }

    static JSONValue createswap(const JSONValue& params) {
        if (!params.IsArray() || params.Size() < 5) {
            throw std::runtime_error("Usage: createswap <initiator_chain> <participant_chain> <initiator_amount> <participant_amount> <initiator_pubkey> [locktime_hours]");
        }

        auto initiator_chain_result = ParseSwapChain(params[0].GetString());
        if (initiator_chain_result.IsError()) {
            throw std::runtime_error(initiator_chain_result.error);
        }

        auto participant_chain_result = ParseSwapChain(params[1].GetString());
        if (participant_chain_result.IsError()) {
            throw std::runtime_error(participant_chain_result.error);
        }

        uint64_t initiator_amount = params[2].GetInt();
        uint64_t participant_amount = params[3].GetInt();

        auto initiator_pubkey_result = HexToBytes(params[4].GetString());
        if (initiator_pubkey_result.IsError()) {
            throw std::runtime_error("Invalid initiator_pubkey");
        }

        uint32_t locktime_hours = 48;  // Default
        if (params.Size() > 5) {
            locktime_hours = params[5].GetInt();
        }

        auto offer_result = g_swap_coordinator.CreateSwapOffer(
            initiator_chain_result.GetValue(),
            participant_chain_result.GetValue(),
            initiator_amount,
            participant_amount,
            initiator_pubkey_result.GetValue(),
            locktime_hours
        );

        if (offer_result.IsError()) {
            throw std::runtime_error(offer_result.error);
        }

        SwapOffer offer = offer_result.GetValue();

        std::map<std::string, JSONValue> result;
        result["swap_id"] = JSONValue(Uint256ToHex(offer.swap_id));
        result["initiator_chain"] = JSONValue(AtomicSwapCoordinator::GetChainName(offer.initiator_chain));
        result["participant_chain"] = JSONValue(AtomicSwapCoordinator::GetChainName(offer.participant_chain));
        result["initiator_amount"] = JSONValue(static_cast<int64_t>(offer.initiator_amount));
        result["participant_amount"] = JSONValue(static_cast<int64_t>(offer.participant_amount));
        result["payment_hash"] = JSONValue(BytesToHex(offer.payment_hash));
        result["initiator_locktime"] = JSONValue(static_cast<int64_t>(offer.initiator_locktime));
        result["participant_locktime"] = JSONValue(static_cast<int64_t>(offer.participant_locktime));

        return JSONValue(result);
    }

    static JSONValue listswaps(const JSONValue& params) {
        std::vector<SwapInfo> swaps = g_swap_coordinator.GetAllSwaps();

        std::vector<JSONValue> swap_list;
        for (const auto& swap : swaps) {
            std::map<std::string, JSONValue> swap_obj;
            swap_obj["swap_id"] = JSONValue(Uint256ToHex(swap.offer.swap_id));
            swap_obj["state"] = JSONValue(AtomicSwapCoordinator::GetStateName(swap.state));
            swap_obj["role"] = JSONValue(swap.role == SwapRole::INITIATOR ? "INITIATOR" : "PARTICIPANT");
            swap_obj["initiator_chain"] = JSONValue(AtomicSwapCoordinator::GetChainName(swap.offer.initiator_chain));
            swap_obj["participant_chain"] = JSONValue(AtomicSwapCoordinator::GetChainName(swap.offer.participant_chain));
            swap_obj["initiator_amount"] = JSONValue(static_cast<int64_t>(swap.offer.initiator_amount));
            swap_obj["participant_amount"] = JSONValue(static_cast<int64_t>(swap.offer.participant_amount));

            swap_list.push_back(JSONValue(swap_obj));
        }

        return JSONValue(swap_list);
    }
};

// Export registration function
void RegisterAtomicSwapRPCCommands(RPCServer& server) {
    AtomicSwapRPC::RegisterMethods(server);
}

}  // namespace rpc
}  // namespace intcoin
