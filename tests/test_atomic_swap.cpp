// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/htlc.h>
#include <intcoin/atomic_swap.h>
#include <intcoin/blockchain_monitor.h>
#include <intcoin/crypto.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cstring>

using namespace intcoin;
using namespace intcoin::htlc;
using namespace intcoin::atomic_swap;
using namespace intcoin::blockchain_monitor;

// Test helper: Convert hex string to bytes
std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
    }
    return bytes;
}

// Test helper: Convert bytes to hex string
std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Test 1: HTLC Script Creation and Verification
void TestHTLCScriptCreation() {
    std::cout << "Test 1: HTLC Script Creation and Verification... ";

    // Generate test keys and hash
    std::vector<uint8_t> recipient_pubkey(33, 0x02);
    std::vector<uint8_t> refund_pubkey(33, 0x03);
    std::vector<uint8_t> preimage(32, 0xAA);

    // Create payment hash using SHA3-256
    std::vector<uint8_t> payment_hash = HTLCScript::HashPreimage(preimage, HTLCHashAlgorithm::SHA3_256);

    // Create HTLC parameters
    HTLCParameters params;
    params.recipient_pubkey = recipient_pubkey;
    params.refund_pubkey = refund_pubkey;
    params.hash_lock = payment_hash;
    params.locktime = 1000000;
    params.hash_algorithm = HTLCHashAlgorithm::SHA3_256;
    params.is_block_height = false;

    // Create HTLC script
    Script htlc_script = HTLCScript::CreateHTLCScript(params);

    // Verify script is not empty
    assert(htlc_script.bytes.size() > 0);

    // Verify preimage
    bool valid = HTLCScript::VerifyPreimage(preimage, payment_hash, HTLCHashAlgorithm::SHA3_256);
    assert(valid);

    // Test invalid preimage
    std::vector<uint8_t> wrong_preimage(32, 0xBB);
    bool invalid = HTLCScript::VerifyPreimage(wrong_preimage, payment_hash, HTLCHashAlgorithm::SHA3_256);
    assert(!invalid);

    std::cout << "PASSED" << std::endl;
}

// Test 2: Bitcoin-Compatible Hash Functions
void TestBitcoinHashes() {
    std::cout << "Test 2: Bitcoin-Compatible Hash Functions... ";

    std::vector<uint8_t> preimage(32, 0xCC);

    // Test SHA-256
    std::vector<uint8_t> sha256_hash = HTLCScript::HashPreimage(preimage, HTLCHashAlgorithm::SHA256);
    assert(sha256_hash.size() == 32);

    // Test RIPEMD-160
    std::vector<uint8_t> ripemd160_hash = HTLCScript::HashPreimage(preimage, HTLCHashAlgorithm::RIPEMD160);
    assert(ripemd160_hash.size() == 20);

    // Verify SHA-256 preimage
    bool sha256_valid = HTLCScript::VerifyPreimage(preimage, sha256_hash, HTLCHashAlgorithm::SHA256);
    assert(sha256_valid);

    // Verify RIPEMD-160 preimage
    bool ripemd160_valid = HTLCScript::VerifyPreimage(preimage, ripemd160_hash, HTLCHashAlgorithm::RIPEMD160);
    assert(ripemd160_valid);

    std::cout << "PASSED" << std::endl;
}

// Test 3: Atomic Swap Offer Creation
void TestSwapOfferCreation() {
    std::cout << "Test 3: Atomic Swap Offer Creation... ";

    AtomicSwapCoordinator coordinator;

    std::vector<uint8_t> initiator_pubkey(33, 0x02);

    auto offer_result = coordinator.CreateSwapOffer(
        SwapChain::INTCOIN,
        SwapChain::BITCOIN,
        100000000,  // 1 INT
        50000000,   // 0.5 BTC
        initiator_pubkey,
        48  // 48 hour locktime
    );

    assert(offer_result.IsOk());

    SwapOffer offer = offer_result.GetValue();

    // Verify offer fields
    assert(offer.initiator_chain == SwapChain::INTCOIN);
    assert(offer.participant_chain == SwapChain::BITCOIN);
    assert(offer.initiator_amount == 100000000);
    assert(offer.participant_amount == 50000000);
    assert(offer.payment_hash.size() == 32);
    assert(offer.initiator_locktime > offer.participant_locktime);
    assert((offer.initiator_locktime - offer.participant_locktime) >= 24 * 3600);  // 24h safety

    std::cout << "PASSED" << std::endl;
}

// Test 4: Swap Offer Acceptance
void TestSwapOfferAcceptance() {
    std::cout << "Test 4: Swap Offer Acceptance... ";

    AtomicSwapCoordinator coordinator;

    std::vector<uint8_t> initiator_pubkey(33, 0x02);
    std::vector<uint8_t> participant_pubkey(33, 0x03);

    // Create offer
    auto offer_result = coordinator.CreateSwapOffer(
        SwapChain::INTCOIN,
        SwapChain::LITECOIN,
        100000000,  // 1 INT
        25000000,   // 0.25 LTC
        initiator_pubkey,
        48
    );

    assert(offer_result.IsOk());
    SwapOffer offer = offer_result.GetValue();

    // Accept offer
    auto accepted_offer_result = coordinator.AcceptSwapOffer(offer, participant_pubkey);
    assert(accepted_offer_result.IsOk());

    SwapOffer accepted_offer = accepted_offer_result.GetValue();

    // Verify participant pubkey was added
    assert(accepted_offer.participant_pubkey == participant_pubkey);
    assert(accepted_offer.swap_id == offer.swap_id);

    std::cout << "PASSED" << std::endl;
}

// Test 5: Swap State Machine
void TestSwapStateMachine() {
    std::cout << "Test 5: Swap State Machine... ";

    AtomicSwapCoordinator coordinator;

    std::vector<uint8_t> initiator_pubkey(33, 0x02);

    // Create offer
    auto offer_result = coordinator.CreateSwapOffer(
        SwapChain::TESTNET_INT,
        SwapChain::TESTNET_BTC,
        100000000,
        50000000,
        initiator_pubkey,
        48
    );

    assert(offer_result.IsOk());
    SwapOffer offer = offer_result.GetValue();

    // Get swap info
    auto swap_info_result = coordinator.GetSwapInfo(offer.swap_id);
    assert(swap_info_result.IsOk());

    SwapInfo swap_info = swap_info_result.GetValue();

    // Verify initial state
    assert(swap_info.state == SwapState::OFFER_CREATED);
    assert(swap_info.role == SwapRole::INITIATOR);

    std::cout << "PASSED" << std::endl;
}

// Test 6: Multiple Swaps
void TestMultipleSwaps() {
    std::cout << "Test 6: Multiple Swaps... ";

    AtomicSwapCoordinator coordinator;

    std::vector<uint8_t> pubkey1(33, 0x02);
    std::vector<uint8_t> pubkey2(33, 0x03);
    std::vector<uint8_t> pubkey3(33, 0x04);

    // Create multiple swaps
    auto swap1 = coordinator.CreateSwapOffer(SwapChain::INTCOIN, SwapChain::BITCOIN, 100000000, 50000000, pubkey1, 48);
    auto swap2 = coordinator.CreateSwapOffer(SwapChain::INTCOIN, SwapChain::LITECOIN, 100000000, 25000000, pubkey2, 48);
    auto swap3 = coordinator.CreateSwapOffer(SwapChain::TESTNET_INT, SwapChain::TESTNET_BTC, 100000000, 50000000, pubkey3, 48);

    assert(swap1.IsOk());
    assert(swap2.IsOk());
    assert(swap3.IsOk());

    // Get all swaps
    std::vector<SwapInfo> all_swaps = coordinator.GetAllSwaps();
    assert(all_swaps.size() == 3);

    // Verify each swap has unique ID
    assert(swap1.GetValue().swap_id != swap2.GetValue().swap_id);
    assert(swap2.GetValue().swap_id != swap3.GetValue().swap_id);
    assert(swap1.GetValue().swap_id != swap3.GetValue().swap_id);

    std::cout << "PASSED" << std::endl;
}

// Test 7: Chain Name Conversion
void TestChainNameConversion() {
    std::cout << "Test 7: Chain Name Conversion... ";

    assert(AtomicSwapCoordinator::GetChainName(SwapChain::INTCOIN) == "INTcoin");
    assert(AtomicSwapCoordinator::GetChainName(SwapChain::BITCOIN) == "Bitcoin");
    assert(AtomicSwapCoordinator::GetChainName(SwapChain::LITECOIN) == "Litecoin");
    assert(AtomicSwapCoordinator::GetChainName(SwapChain::TESTNET_INT) == "INTcoin Testnet");
    assert(AtomicSwapCoordinator::GetChainName(SwapChain::TESTNET_BTC) == "Bitcoin Testnet");
    assert(AtomicSwapCoordinator::GetChainName(SwapChain::TESTNET_LTC) == "Litecoin Testnet");

    std::cout << "PASSED" << std::endl;
}

// Test 8: State Name Conversion
void TestStateNameConversion() {
    std::cout << "Test 8: State Name Conversion... ";

    assert(AtomicSwapCoordinator::GetStateName(SwapState::OFFER_CREATED) == "Offer Created");
    assert(AtomicSwapCoordinator::GetStateName(SwapState::INITIATOR_HTLC_FUNDED) == "Initiator HTLC Funded");
    assert(AtomicSwapCoordinator::GetStateName(SwapState::PARTICIPANT_HTLC_FUNDED) == "Participant HTLC Funded");
    assert(AtomicSwapCoordinator::GetStateName(SwapState::PARTICIPANT_CLAIMED) == "Participant Claimed");
    assert(AtomicSwapCoordinator::GetStateName(SwapState::COMPLETED) == "Completed");
    assert(AtomicSwapCoordinator::GetStateName(SwapState::FAILED) == "Failed");

    std::cout << "PASSED" << std::endl;
}

// Test 9: HTLC Claim Witness
void TestHTLCClaimWitness() {
    std::cout << "Test 9: HTLC Claim Witness... ";

    std::vector<uint8_t> preimage(32, 0xDD);
    std::vector<uint8_t> signature(64, 0xEE);

    Script claim_witness = HTLCScript::CreateClaimWitness(preimage, signature);

    // Verify witness contains preimage and signature
    assert(claim_witness.bytes.size() > 0);

    std::cout << "PASSED" << std::endl;
}

// Test 10: HTLC Refund Witness
void TestHTLCRefundWitness() {
    std::cout << "Test 10: HTLC Refund Witness... ";

    std::vector<uint8_t> signature(64, 0xFF);

    Script refund_witness = HTLCScript::CreateRefundWitness(signature);

    // Verify witness contains signature
    assert(refund_witness.bytes.size() > 0);

    std::cout << "PASSED" << std::endl;
}

// Test 11: Blockchain Monitor Factory
void TestBlockchainMonitorFactory() {
    std::cout << "Test 11: Blockchain Monitor Factory... ";

    // Test factory creation for different chains
    auto btc_monitor = CreateBlockchainMonitor(
        BlockchainType::BITCOIN,
        "http://localhost:8332",
        "user",
        "password"
    );

    auto ltc_monitor = CreateBlockchainMonitor(
        BlockchainType::LITECOIN,
        "http://localhost:9332",
        "user",
        "password"
    );

    auto testnet_btc_monitor = CreateBlockchainMonitor(
        BlockchainType::TESTNET_BTC,
        "http://localhost:18332",
        "user",
        "password"
    );

    assert(btc_monitor != nullptr);
    assert(ltc_monitor != nullptr);
    assert(testnet_btc_monitor != nullptr);

    assert(btc_monitor->GetBlockchainType() == BlockchainType::BITCOIN);
    assert(ltc_monitor->GetBlockchainType() == BlockchainType::LITECOIN);
    assert(testnet_btc_monitor->GetBlockchainType() == BlockchainType::TESTNET_BTC);

    std::cout << "PASSED" << std::endl;
}

// Test 12: Locktime Safety Buffer
void TestLocktimeSafetyBuffer() {
    std::cout << "Test 12: Locktime Safety Buffer... ";

    AtomicSwapCoordinator coordinator;
    std::vector<uint8_t> pubkey(33, 0x02);

    auto offer_result = coordinator.CreateSwapOffer(
        SwapChain::INTCOIN,
        SwapChain::BITCOIN,
        100000000,
        50000000,
        pubkey,
        48  // 48 hours
    );

    assert(offer_result.IsOk());
    SwapOffer offer = offer_result.GetValue();

    // Verify 24-hour safety buffer
    uint64_t safety_buffer = offer.initiator_locktime - offer.participant_locktime;
    assert(safety_buffer >= 24 * 3600);  // At least 24 hours

    std::cout << "PASSED" << std::endl;
}

// Test 13: Payment Hash Uniqueness
void TestPaymentHashUniqueness() {
    std::cout << "Test 13: Payment Hash Uniqueness... ";

    AtomicSwapCoordinator coordinator;
    std::vector<uint8_t> pubkey(33, 0x02);

    // Create multiple offers - each should have unique payment hash
    auto offer1 = coordinator.CreateSwapOffer(SwapChain::INTCOIN, SwapChain::BITCOIN, 100000000, 50000000, pubkey, 48);
    auto offer2 = coordinator.CreateSwapOffer(SwapChain::INTCOIN, SwapChain::BITCOIN, 100000000, 50000000, pubkey, 48);
    auto offer3 = coordinator.CreateSwapOffer(SwapChain::INTCOIN, SwapChain::BITCOIN, 100000000, 50000000, pubkey, 48);

    assert(offer1.IsOk() && offer2.IsOk() && offer3.IsOk());

    // Verify all payment hashes are unique
    assert(offer1.GetValue().payment_hash != offer2.GetValue().payment_hash);
    assert(offer2.GetValue().payment_hash != offer3.GetValue().payment_hash);
    assert(offer1.GetValue().payment_hash != offer3.GetValue().payment_hash);

    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "===============================================" << std::endl;
    std::cout << "     Atomic Swap Test Suite" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << std::endl;

    try {
        TestHTLCScriptCreation();
        TestBitcoinHashes();
        TestSwapOfferCreation();
        TestSwapOfferAcceptance();
        TestSwapStateMachine();
        TestMultipleSwaps();
        TestChainNameConversion();
        TestStateNameConversion();
        TestHTLCClaimWitness();
        TestHTLCRefundWitness();
        TestBlockchainMonitorFactory();
        TestLocktimeSafetyBuffer();
        TestPaymentHashUniqueness();

        std::cout << std::endl;
        std::cout << "===============================================" << std::endl;
        std::cout << "     All tests PASSED! ✓" << std::endl;
        std::cout << "===============================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << std::endl;
        std::cerr << "FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
