// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/atomic_swap.h>
#include <intcoin/crypto.h>
#include <intcoin/util.h>

#include <algorithm>
#include <random>

namespace intcoin {
namespace atomic_swap {

// ============================================================================
// AtomicSwapCoordinator Implementation
// ============================================================================

AtomicSwapCoordinator::AtomicSwapCoordinator()
    : htlc_manager_(std::make_shared<htlc::HTLCManager>()) {
    LogF(LogLevel::INFO, "Atomic Swap: Coordinator initialized");
}

AtomicSwapCoordinator::~AtomicSwapCoordinator() {
    LogF(LogLevel::INFO, "Atomic Swap: Coordinator shutting down");
}

// ========================================
// Swap Creation & Negotiation
// ========================================

Result<SwapOffer> AtomicSwapCoordinator::CreateSwapOffer(
    SwapChain initiator_chain,
    SwapChain participant_chain,
    uint64_t initiator_amount,
    uint64_t participant_amount,
    const std::vector<uint8_t>& initiator_pubkey,
    uint32_t locktime_hours) {

    // Generate secret preimage
    std::vector<uint8_t> preimage = GeneratePreimage();

    // Compute payment hash based on participant's chain
    std::vector<uint8_t> payment_hash = ComputePaymentHash(preimage, participant_chain);

    // Calculate locktimes
    uint64_t now = std::time(nullptr);
    uint64_t participant_locktime = now + (locktime_hours * 3600);
    uint64_t initiator_locktime = participant_locktime + (24 * 3600);  // +24 hours safety buffer

    // Create swap offer
    SwapOffer offer;
    offer.initiator_chain = initiator_chain;
    offer.participant_chain = participant_chain;
    offer.initiator_amount = initiator_amount;
    offer.participant_amount = participant_amount;
    offer.initiator_pubkey = initiator_pubkey;
    offer.payment_hash = payment_hash;
    offer.initiator_locktime = initiator_locktime;
    offer.participant_locktime = participant_locktime;
    offer.offer_expires_at = now + (7 * 24 * 3600);  // 7 days

    // Calculate swap ID
    offer.swap_id = CalculateSwapID(offer);

    // TODO: Sign offer with initiator's private key
    // offer.signature = Sign(offer_hash, initiator_privkey);

    // Create swap info
    SwapInfo swap_info;
    swap_info.offer = offer;
    swap_info.state = SwapState::OFFER_CREATED;
    swap_info.role = SwapRole::INITIATOR;
    swap_info.preimage = preimage;  // Save preimage for initiator
    swap_info.created_at = now;
    swap_info.updated_at = now;

    // Store swap
    swaps_[offer.swap_id] = swap_info;

    LogF(LogLevel::INFO, "Atomic Swap: Created offer %s (%llu %s for %llu %s)",
         BytesToHex(std::vector<uint8_t>(offer.swap_id.begin(), offer.swap_id.end())).substr(0, 16).c_str(),
         initiator_amount,
         GetChainName(initiator_chain).c_str(),
         participant_amount,
         GetChainName(participant_chain).c_str());

    TriggerEvent(SwapEventType::OFFER_RECEIVED, offer.swap_id,
                SwapState::OFFER_CREATED, "Swap offer created");

    return Result<SwapOffer>::Ok(offer);
}

Result<SwapOffer> AtomicSwapCoordinator::AcceptSwapOffer(
    const SwapOffer& offer,
    const std::vector<uint8_t>& participant_pubkey) {

    // Validate offer
    auto validate_result = ValidateSwapOffer(offer);
    if (validate_result.IsError()) {
        return Result<SwapOffer>::Error(validate_result.error);
    }

    // Check if offer has expired
    uint64_t now = std::time(nullptr);
    if (now >= offer.offer_expires_at) {
        return Result<SwapOffer>::Error("Swap offer has expired");
    }

    // Update offer with participant info
    SwapOffer accepted_offer = offer;
    accepted_offer.participant_pubkey = participant_pubkey;

    // TODO: Sign acceptance with participant's private key
    // accepted_offer.signature = Sign(offer_hash, participant_privkey);

    // Create swap info for participant
    SwapInfo swap_info;
    swap_info.offer = accepted_offer;
    swap_info.state = SwapState::OFFER_ACCEPTED;
    swap_info.role = SwapRole::PARTICIPANT;
    swap_info.created_at = now;
    swap_info.updated_at = now;

    // Store swap
    swaps_[offer.swap_id] = swap_info;

    LogF(LogLevel::INFO, "Atomic Swap: Accepted offer %s",
         BytesToHex(std::vector<uint8_t>(offer.swap_id.begin(), offer.swap_id.end())).substr(0, 16).c_str());

    TriggerEvent(SwapEventType::OFFER_ACCEPTED, offer.swap_id,
                SwapState::OFFER_ACCEPTED, "Swap offer accepted");

    return Result<SwapOffer>::Ok(accepted_offer);
}

Result<void> AtomicSwapCoordinator::CancelSwap(const uint256& swap_id) {
    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return Result<void>::Error("Swap not found");
    }

    SwapInfo& swap = it->second;

    // Can only cancel in early states
    if (swap.state != SwapState::OFFER_CREATED &&
        swap.state != SwapState::OFFER_SENT &&
        swap.state != SwapState::OFFER_RECEIVED) {
        return Result<void>::Error("Cannot cancel swap in current state");
    }

    UpdateSwapState(swap_id, SwapState::CANCELLED);

    LogF(LogLevel::INFO, "Atomic Swap: Cancelled swap %s",
         BytesToHex(std::vector<uint8_t>(swap_id.begin(), swap_id.end())).substr(0, 16).c_str());

    return Result<void>::Ok();
}

// ========================================
// Swap Execution
// ========================================

Result<void> AtomicSwapCoordinator::StartSwapExecution(const uint256& swap_id) {
    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return Result<void>::Error("Swap not found");
    }

    SwapInfo& swap = it->second;

    // Must be in OFFER_ACCEPTED state
    if (swap.state != SwapState::OFFER_ACCEPTED) {
        return Result<void>::Error("Swap must be accepted before execution");
    }

    // Initiator creates HTLC first
    if (swap.role == SwapRole::INITIATOR) {
        UpdateSwapState(swap_id, SwapState::INITIATOR_HTLC_PENDING);
    } else {
        // Participant waits for initiator's HTLC
        UpdateSwapState(swap_id, SwapState::INITIATOR_HTLC_PENDING);
    }

    LogF(LogLevel::INFO, "Atomic Swap: Started execution for swap %s",
         BytesToHex(std::vector<uint8_t>(swap_id.begin(), swap_id.end())).substr(0, 16).c_str());

    return Result<void>::Ok();
}

Result<Transaction> AtomicSwapCoordinator::CreateInitiatorHTLC(
    const uint256& swap_id,
    const std::vector<TxIn>& funding_inputs) {

    auto swap_result = GetSwapInfo(swap_id);
    if (swap_result.IsError()) {
        return Result<Transaction>::Error(swap_result.error);
    }

    SwapInfo swap = swap_result.GetValue();

    // Build HTLC parameters
    htlc::HTLCParameters htlc_params;
    htlc_params.recipient_pubkey = swap.offer.participant_pubkey;
    htlc_params.refund_pubkey = swap.offer.initiator_pubkey;
    htlc_params.hash_lock = swap.offer.payment_hash;
    htlc_params.locktime = swap.offer.initiator_locktime;
    htlc_params.hash_algorithm = htlc::HTLCHashAlgorithm::SHA3_256;  // TODO: Set based on chain
    htlc_params.is_block_height = false;  // Use timestamp

    // Create HTLC transaction
    htlc::HTLCTransactionBuilder builder;
    auto tx_result = builder.CreateFundingTransaction(
        funding_inputs,
        htlc_params,
        swap.offer.initiator_amount,
        "",  // TODO: Change address
        2000  // Fee rate
    );

    if (tx_result.IsError()) {
        return tx_result;
    }

    Transaction tx = tx_result.GetValue();

    // Update swap with HTLC details
    swap.initiator_contract.htlc_tx_hash = tx.GetHash();
    swap.initiator_contract.htlc_output_index = 0;
    swap.initiator_contract.htlc_script = tx.outputs[0].script_pubkey.bytes;
    swap.initiator_contract.amount = swap.offer.initiator_amount;
    swap.initiator_contract.locktime = swap.offer.initiator_locktime;
    swap.initiator_contract.required_confirmations = 3;

    swaps_[swap_id] = swap;
    UpdateSwapState(swap_id, SwapState::INITIATOR_HTLC_FUNDED);

    LogF(LogLevel::INFO, "Atomic Swap: Created initiator HTLC for swap %s",
         BytesToHex(std::vector<uint8_t>(swap_id.begin(), swap_id.end())).substr(0, 16).c_str());

    return Result<Transaction>::Ok(tx);
}

Result<Transaction> AtomicSwapCoordinator::CreateParticipantHTLC(
    const uint256& swap_id,
    const std::vector<TxIn>& funding_inputs) {

    auto swap_result = GetSwapInfo(swap_id);
    if (swap_result.IsError()) {
        return Result<Transaction>::Error(swap_result.error);
    }

    SwapInfo swap = swap_result.GetValue();

    // Verify initiator's HTLC is funded
    if (swap.state != SwapState::INITIATOR_HTLC_FUNDED) {
        return Result<Transaction>::Error("Initiator HTLC must be funded first");
    }

    // Build HTLC parameters
    htlc::HTLCParameters htlc_params;
    htlc_params.recipient_pubkey = swap.offer.initiator_pubkey;
    htlc_params.refund_pubkey = swap.offer.participant_pubkey;
    htlc_params.hash_lock = swap.offer.payment_hash;
    htlc_params.locktime = swap.offer.participant_locktime;
    htlc_params.hash_algorithm = htlc::HTLCHashAlgorithm::SHA3_256;  // TODO: Set based on chain
    htlc_params.is_block_height = false;  // Use timestamp

    // Create HTLC transaction
    htlc::HTLCTransactionBuilder builder;
    auto tx_result = builder.CreateFundingTransaction(
        funding_inputs,
        htlc_params,
        swap.offer.participant_amount,
        "",  // TODO: Change address
        2000  // Fee rate
    );

    if (tx_result.IsError()) {
        return tx_result;
    }

    Transaction tx = tx_result.GetValue();

    // Update swap with HTLC details
    swap.participant_contract.htlc_tx_hash = tx.GetHash();
    swap.participant_contract.htlc_output_index = 0;
    swap.participant_contract.htlc_script = tx.outputs[0].script_pubkey.bytes;
    swap.participant_contract.amount = swap.offer.participant_amount;
    swap.participant_contract.locktime = swap.offer.participant_locktime;
    swap.participant_contract.required_confirmations = 3;

    swaps_[swap_id] = swap;
    UpdateSwapState(swap_id, SwapState::PARTICIPANT_HTLC_FUNDED);

    LogF(LogLevel::INFO, "Atomic Swap: Created participant HTLC for swap %s",
         BytesToHex(std::vector<uint8_t>(swap_id.begin(), swap_id.end())).substr(0, 16).c_str());

    return Result<Transaction>::Ok(tx);
}

Result<Transaction> AtomicSwapCoordinator::ClaimHTLC(
    const uint256& swap_id,
    bool is_initiator) {

    auto swap_result = GetSwapInfo(swap_id);
    if (swap_result.IsError()) {
        return Result<Transaction>::Error(swap_result.error);
    }

    SwapInfo swap = swap_result.GetValue();

    // Get the contract to claim
    const SwapContract& contract = is_initiator ?
        swap.participant_contract : swap.initiator_contract;

    // Verify contract is funded
    if (contract.htlc_tx_hash == uint256{}) {
        return Result<Transaction>::Error("HTLC not funded");
    }

    // Get preimage
    std::vector<uint8_t> preimage;
    if (is_initiator) {
        // Initiator has the preimage
        preimage = swap.preimage;
    } else {
        // Participant must extract from initiator's claim
        preimage = WatchForPreimage(swap_id);
        if (preimage.empty()) {
            return Result<Transaction>::Error("Preimage not yet revealed");
        }
    }

    // Create HTLC outpoint
    OutPoint htlc_outpoint;
    htlc_outpoint.tx_hash = contract.htlc_tx_hash;
    htlc_outpoint.index = contract.htlc_output_index;

    // Create HTLC script
    Script htlc_script;
    htlc_script.bytes = contract.htlc_script;

    // Create claim transaction
    htlc::HTLCTransactionBuilder builder;
    auto tx_result = builder.CreateClaimTransaction(
        htlc_outpoint,
        contract.amount,
        htlc_script,
        preimage,
        "",  // TODO: Recipient address
        2000  // Fee rate
    );

    if (tx_result.IsError()) {
        return tx_result;
    }

    Transaction tx = tx_result.GetValue();

    // Update swap state
    if (is_initiator) {
        UpdateSwapState(swap_id, SwapState::INITIATOR_CLAIMED);
        if (swap.state == SwapState::PARTICIPANT_CLAIMED) {
            UpdateSwapState(swap_id, SwapState::COMPLETED);
        }
    } else {
        UpdateSwapState(swap_id, SwapState::PARTICIPANT_CLAIMED);
    }

    LogF(LogLevel::INFO, "Atomic Swap: %s claimed HTLC for swap %s",
         is_initiator ? "Initiator" : "Participant",
         BytesToHex(std::vector<uint8_t>(swap_id.begin(), swap_id.end())).substr(0, 16).c_str());

    return Result<Transaction>::Ok(tx);
}

Result<Transaction> AtomicSwapCoordinator::RefundHTLC(
    const uint256& swap_id,
    bool is_initiator) {

    auto swap_result = GetSwapInfo(swap_id);
    if (swap_result.IsError()) {
        return Result<Transaction>::Error(swap_result.error);
    }

    SwapInfo swap = swap_result.GetValue();

    // Get the contract to refund
    const SwapContract& contract = is_initiator ?
        swap.initiator_contract : swap.participant_contract;

    // Verify locktime has passed
    if (!IsSwapExpired(swap_id)) {
        return Result<Transaction>::Error("Locktime has not passed yet");
    }

    // Create HTLC outpoint
    OutPoint htlc_outpoint;
    htlc_outpoint.tx_hash = contract.htlc_tx_hash;
    htlc_outpoint.index = contract.htlc_output_index;

    // Create HTLC script
    Script htlc_script;
    htlc_script.bytes = contract.htlc_script;

    // Create refund transaction
    htlc::HTLCTransactionBuilder builder;
    auto tx_result = builder.CreateRefundTransaction(
        htlc_outpoint,
        contract.amount,
        htlc_script,
        "",  // TODO: Refund address
        contract.locktime,
        2000  // Fee rate
    );

    if (tx_result.IsError()) {
        return tx_result;
    }

    Transaction tx = tx_result.GetValue();

    // Update swap state
    UpdateSwapState(swap_id, SwapState::REFUNDED);

    LogF(LogLevel::INFO, "Atomic Swap: Refunded HTLC for swap %s",
         BytesToHex(std::vector<uint8_t>(swap_id.begin(), swap_id.end())).substr(0, 16).c_str());

    TriggerEvent(SwapEventType::SWAP_REFUNDED, swap_id,
                SwapState::REFUNDED, "Swap refunded after timeout");

    return Result<Transaction>::Ok(tx);
}

// ========================================
// Swap Monitoring
// ========================================

Result<SwapState> AtomicSwapCoordinator::MonitorSwap(const uint256& swap_id) {
    auto swap_result = GetSwapInfo(swap_id);
    if (swap_result.IsError()) {
        return Result<SwapState>::Error(swap_result.error);
    }

    SwapInfo& swap = swap_result.GetValue();
    SwapState current_state = swap.state;

    // State machine transitions
    switch (current_state) {
        case SwapState::INITIATOR_HTLC_PENDING:
        case SwapState::INITIATOR_HTLC_FUNDED: {
            // Check initiator HTLC confirmations
            uint32_t confirmations = CheckHTLCConfirmations(swap.initiator_contract);
            if (confirmations >= swap.initiator_contract.required_confirmations) {
                if (current_state == SwapState::INITIATOR_HTLC_PENDING) {
                    UpdateSwapState(swap_id, SwapState::INITIATOR_HTLC_FUNDED);
                    TriggerEvent(SwapEventType::INITIATOR_HTLC_DETECTED, swap_id,
                               SwapState::INITIATOR_HTLC_FUNDED, "Initiator HTLC confirmed");
                }
            }
            break;
        }

        case SwapState::PARTICIPANT_HTLC_PENDING:
        case SwapState::PARTICIPANT_HTLC_FUNDED: {
            // Check participant HTLC confirmations
            uint32_t confirmations = CheckHTLCConfirmations(swap.participant_contract);
            if (confirmations >= swap.participant_contract.required_confirmations) {
                if (current_state == SwapState::PARTICIPANT_HTLC_PENDING) {
                    UpdateSwapState(swap_id, SwapState::PARTICIPANT_HTLC_FUNDED);
                    TriggerEvent(SwapEventType::PARTICIPANT_HTLC_DETECTED, swap_id,
                               SwapState::PARTICIPANT_HTLC_FUNDED, "Participant HTLC confirmed");
                }

                // Watch for preimage revelation
                std::vector<uint8_t> preimage = WatchForPreimage(swap_id);
                if (!preimage.empty() && swap.preimage.empty()) {
                    swap.preimage = preimage;
                    swaps_[swap_id] = swap;
                    TriggerEvent(SwapEventType::PREIMAGE_REVEALED, swap_id,
                               current_state, "Secret preimage revealed");
                }
            }
            break;
        }

        case SwapState::PARTICIPANT_CLAIMED:
        case SwapState::INITIATOR_CLAIMED: {
            // Check if both parties have claimed
            if (current_state == SwapState::PARTICIPANT_CLAIMED) {
                // Initiator can now claim with revealed preimage
                // This would be handled by the initiator calling ClaimHTLC()
            } else if (current_state == SwapState::INITIATOR_CLAIMED) {
                // Swap completed
                UpdateSwapState(swap_id, SwapState::COMPLETED);
                TriggerEvent(SwapEventType::SWAP_COMPLETED, swap_id,
                           SwapState::COMPLETED, "Swap completed successfully");
            }
            break;
        }

        default:
            break;
    }

    // Check for expiration
    if (IsSwapExpired(swap_id) &&
        current_state != SwapState::COMPLETED &&
        current_state != SwapState::REFUNDED &&
        current_state != SwapState::CANCELLED) {
        UpdateSwapState(swap_id, SwapState::EXPIRED);
        TriggerEvent(SwapEventType::SWAP_FAILED, swap_id,
                   SwapState::EXPIRED, "Swap expired");
    }

    return Result<SwapState>::Ok(swaps_[swap_id].state);
}

uint32_t AtomicSwapCoordinator::CheckHTLCConfirmations(const SwapContract& contract) {
    // TODO: Query blockchain for HTLC transaction confirmations
    // For now, return 0
    return 0;
}

std::vector<uint8_t> AtomicSwapCoordinator::WatchForPreimage(const uint256& swap_id) {
    // TODO: Monitor blockchain for claim transaction and extract preimage
    // For now, return empty
    return {};
}

bool AtomicSwapCoordinator::IsSwapExpired(const uint256& swap_id) {
    auto swap_result = GetSwapInfo(swap_id);
    if (swap_result.IsError()) {
        return false;
    }

    SwapInfo swap = swap_result.GetValue();
    uint64_t now = std::time(nullptr);

    // Check participant locktime (happens first)
    return now >= swap.offer.participant_locktime;
}

// ========================================
// Swap Query
// ========================================

Result<SwapInfo> AtomicSwapCoordinator::GetSwapInfo(const uint256& swap_id) const {
    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return Result<SwapInfo>::Error("Swap not found");
    }

    return Result<SwapInfo>::Ok(it->second);
}

std::vector<SwapInfo> AtomicSwapCoordinator::GetAllSwaps() const {
    std::vector<SwapInfo> result;
    result.reserve(swaps_.size());

    for (const auto& [swap_id, info] : swaps_) {
        result.push_back(info);
    }

    return result;
}

std::vector<SwapInfo> AtomicSwapCoordinator::GetSwapsByState(SwapState state) const {
    std::vector<SwapInfo> result;

    for (const auto& [swap_id, info] : swaps_) {
        if (info.state == state) {
            result.push_back(info);
        }
    }

    return result;
}

size_t AtomicSwapCoordinator::GetSwapCount() const {
    return swaps_.size();
}

// ========================================
// Callbacks
// ========================================

void AtomicSwapCoordinator::SetSwapEventCallback(
    std::function<void(const SwapEvent&)> callback) {
    event_callback_ = callback;
}

// ========================================
// Utilities
// ========================================

std::vector<uint8_t> AtomicSwapCoordinator::GeneratePreimage() {
    std::vector<uint8_t> preimage(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (size_t i = 0; i < 32; i++) {
        preimage[i] = static_cast<uint8_t>(dis(gen));
    }

    return preimage;
}

std::vector<uint8_t> AtomicSwapCoordinator::ComputePaymentHash(
    const std::vector<uint8_t>& preimage,
    SwapChain chain) {

    // Use SHA3-256 for INTcoin, SHA-256 for Bitcoin
    uint256 hash = SHA3_256(preimage);
    return std::vector<uint8_t>(hash.begin(), hash.end());
}

std::string AtomicSwapCoordinator::GetChainName(SwapChain chain) {
    switch (chain) {
        case SwapChain::INTCOIN:     return "INTcoin";
        case SwapChain::BITCOIN:     return "Bitcoin";
        case SwapChain::LITECOIN:    return "Litecoin";
        case SwapChain::TESTNET_INT: return "INTcoin Testnet";
        case SwapChain::TESTNET_BTC: return "Bitcoin Testnet";
        case SwapChain::TESTNET_LTC: return "Litecoin Testnet";
    }
    return "Unknown";
}

std::string AtomicSwapCoordinator::GetStateName(SwapState state) {
    switch (state) {
        case SwapState::OFFER_CREATED:            return "Offer Created";
        case SwapState::OFFER_SENT:               return "Offer Sent";
        case SwapState::OFFER_RECEIVED:           return "Offer Received";
        case SwapState::OFFER_ACCEPTED:           return "Offer Accepted";
        case SwapState::INITIATOR_HTLC_PENDING:   return "Initiator HTLC Pending";
        case SwapState::INITIATOR_HTLC_FUNDED:    return "Initiator HTLC Funded";
        case SwapState::PARTICIPANT_HTLC_PENDING: return "Participant HTLC Pending";
        case SwapState::PARTICIPANT_HTLC_FUNDED:  return "Participant HTLC Funded";
        case SwapState::PARTICIPANT_CLAIMED:      return "Participant Claimed";
        case SwapState::INITIATOR_CLAIMED:        return "Initiator Claimed";
        case SwapState::COMPLETED:                return "Completed";
        case SwapState::CANCELLED:                return "Cancelled";
        case SwapState::EXPIRED:                  return "Expired";
        case SwapState::REFUNDED:                 return "Refunded";
        case SwapState::FAILED:                   return "Failed";
    }
    return "Unknown";
}

// ========================================
// Private Methods
// ========================================

void AtomicSwapCoordinator::UpdateSwapState(const uint256& swap_id, SwapState new_state) {
    auto it = swaps_.find(swap_id);
    if (it != swaps_.end()) {
        it->second.state = new_state;
        it->second.updated_at = std::time(nullptr);

        LogF(LogLevel::INFO, "Atomic Swap: Updated state for swap %s to %s",
             BytesToHex(std::vector<uint8_t>(swap_id.begin(), swap_id.end())).substr(0, 16).c_str(),
             GetStateName(new_state).c_str());
    }
}

void AtomicSwapCoordinator::TriggerEvent(SwapEventType type, const uint256& swap_id,
                                        SwapState new_state, const std::string& message) {
    if (event_callback_) {
        SwapEvent event;
        event.type = type;
        event.swap_id = swap_id;
        event.new_state = new_state;
        event.message = message;
        event_callback_(event);
    }
}

Result<void> AtomicSwapCoordinator::ValidateSwapOffer(const SwapOffer& offer) const {
    // Validate amounts
    if (offer.initiator_amount == 0) {
        return Result<void>::Error("Initiator amount cannot be zero");
    }

    if (offer.participant_amount == 0) {
        return Result<void>::Error("Participant amount cannot be zero");
    }

    // Validate public keys
    if (offer.initiator_pubkey.empty()) {
        return Result<void>::Error("Initiator public key missing");
    }

    // Validate payment hash
    if (offer.payment_hash.size() != 32) {
        return Result<void>::Error("Invalid payment hash size");
    }

    // Validate locktimes
    if (offer.participant_locktime >= offer.initiator_locktime) {
        return Result<void>::Error("Participant locktime must be before initiator locktime");
    }

    uint64_t now = std::time(nullptr);
    if (offer.participant_locktime <= now) {
        return Result<void>::Error("Participant locktime must be in the future");
    }

    return Result<void>::Ok();
}

uint256 AtomicSwapCoordinator::CalculateSwapID(const SwapOffer& offer) const {
    // Hash all offer parameters to create unique swap ID
    std::vector<uint8_t> data;

    // Add amounts
    for (int i = 0; i < 8; i++) {
        data.push_back((offer.initiator_amount >> (i * 8)) & 0xFF);
    }
    for (int i = 0; i < 8; i++) {
        data.push_back((offer.participant_amount >> (i * 8)) & 0xFF);
    }

    // Add chains
    data.push_back(static_cast<uint8_t>(offer.initiator_chain));
    data.push_back(static_cast<uint8_t>(offer.participant_chain));

    // Add public keys
    data.insert(data.end(), offer.initiator_pubkey.begin(), offer.initiator_pubkey.end());

    // Add payment hash
    data.insert(data.end(), offer.payment_hash.begin(), offer.payment_hash.end());

    // Add locktimes
    for (int i = 0; i < 8; i++) {
        data.push_back((offer.initiator_locktime >> (i * 8)) & 0xFF);
    }
    for (int i = 0; i < 8; i++) {
        data.push_back((offer.participant_locktime >> (i * 8)) & 0xFF);
    }

    return SHA3_256(data);
}

}  // namespace atomic_swap
}  // namespace intcoin
