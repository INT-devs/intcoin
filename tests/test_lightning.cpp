// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/lightning.h"
#include "intcoin/crypto.h"
#include <iostream>
#include <cassert>

using namespace intcoin;
using namespace intcoin::lightning;

// Test utilities
int tests_passed = 0;
int tests_failed = 0;

void test_assert(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "[PASS] " << test_name << std::endl;
        tests_passed++;
    } else {
        std::cout << "[FAIL] " << test_name << std::endl;
        tests_failed++;
    }
}

// Test HTLC creation
void test_htlc_creation() {
    std::cout << "\n=== Testing HTLC Creation ===" << std::endl;

    HTLC htlc;
    htlc.id = 1;
    htlc.amount_sat = 1000;
    std::fill(htlc.payment_hash.begin(), htlc.payment_hash.end(), 0xAA);
    htlc.cltv_expiry = 144;
    htlc.direction = HTLCDirection::OFFERED;

    test_assert(htlc.id == 1, "HTLC ID set correctly");
    test_assert(htlc.amount_sat == 1000, "HTLC amount set correctly");
    test_assert(htlc.direction == HTLCDirection::OFFERED, "HTLC direction is OFFERED");
}

// Test HTLC serialization
void test_htlc_serialization() {
    std::cout << "\n=== Testing HTLC Serialization ===" << std::endl;

    HTLC htlc;
    htlc.id = 123;
    htlc.amount_sat = 5000;
    std::fill(htlc.payment_hash.begin(), htlc.payment_hash.end(), 0xBB);
    htlc.cltv_expiry = 288;
    htlc.direction = HTLCDirection::RECEIVED;
    htlc.onion_routing = {1, 2, 3, 4, 5};

    // Serialize
    std::vector<uint8_t> serialized = htlc.serialize();
    test_assert(!serialized.empty(), "HTLC serialization produces data");

    // Deserialize
    HTLC deserialized = HTLC::deserialize(serialized);
    test_assert(deserialized.id == htlc.id, "Deserialized ID matches");
    test_assert(deserialized.amount_sat == htlc.amount_sat, "Deserialized amount matches");
    test_assert(deserialized.cltv_expiry == htlc.cltv_expiry, "Deserialized CLTV matches");
}

// Test channel creation
void test_channel_creation() {
    std::cout << "\n=== Testing Channel Creation ===" << std::endl;

    Channel channel;

    test_assert(channel.state == ChannelState::OPENING, "New channel is OPENING");
    test_assert(channel.capacity_sat == 0, "New channel has zero capacity");
    test_assert(channel.pending_htlcs.empty(), "New channel has no HTLCs");
    test_assert(channel.to_self_delay == 144, "To-self delay is 144 blocks");
}

// Test channel opening
void test_channel_opening() {
    std::cout << "\n=== Testing Channel Opening ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();

    uint64_t capacity = 1000000;  // 1M sats
    bool opened = channel.open(keypair.public_key, capacity);

    test_assert(opened, "Channel opened successfully");
    test_assert(channel.state == ChannelState::OPEN, "Channel state is OPEN");
    test_assert(channel.capacity_sat == capacity, "Channel capacity set correctly");
    test_assert(channel.local_balance_sat == capacity, "Local balance equals capacity");
    test_assert(channel.remote_balance_sat == 0, "Remote balance is zero");
}

// Test channel can_send
void test_channel_can_send() {
    std::cout << "\n=== Testing Channel Can Send ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    test_assert(channel.can_send(100000), "Can send 100k sats");
    test_assert(!channel.can_send(2000000), "Cannot send more than capacity");

    // Account for channel reserve
    uint64_t available = channel.available_to_send();
    test_assert(available == 1000000 - channel.channel_reserve_sat, "Available accounts for reserve");
}

// Test HTLC addition
void test_htlc_addition() {
    std::cout << "\n=== Testing HTLC Addition ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    Hash256 payment_hash;
    std::fill(payment_hash.begin(), payment_hash.end(), 0xCC);

    bool added = channel.add_htlc(10000, payment_hash, 500000, {});

    test_assert(added, "HTLC added successfully");
    test_assert(channel.pending_htlcs.size() == 1, "Channel has 1 pending HTLC");
}

// Test HTLC settlement
void test_htlc_settlement() {
    std::cout << "\n=== Testing HTLC Settlement ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    // Create preimage and hash
    std::vector<uint8_t> preimage = {1, 2, 3, 4, 5};
    Hash256 payment_hash = crypto::SHA3_256::hash(preimage.data(), preimage.size());

    // Add HTLC
    channel.add_htlc(10000, payment_hash, 500000, {});
    uint64_t htlc_id = 0;

    // Settle with correct preimage
    bool settled = channel.settle_htlc(htlc_id, preimage);

    test_assert(settled, "HTLC settled successfully");
    test_assert(channel.pending_htlcs.empty(), "No pending HTLCs after settlement");
}

// Test HTLC failure
void test_htlc_failure() {
    std::cout << "\n=== Testing HTLC Failure ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    Hash256 payment_hash;
    std::fill(payment_hash.begin(), payment_hash.end(), 0xDD);

    channel.add_htlc(10000, payment_hash, 500000, {});
    uint64_t htlc_id = 0;

    // Fail the HTLC
    bool failed = channel.fail_htlc(htlc_id);

    test_assert(failed, "HTLC failed successfully");
    test_assert(channel.pending_htlcs.empty(), "No pending HTLCs after failure");
}

// Test channel cooperative close
void test_channel_cooperative_close() {
    std::cout << "\n=== Testing Channel Cooperative Close ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    bool closed = channel.close_cooperative();

    test_assert(closed, "Channel closed cooperatively");
    test_assert(channel.state == ChannelState::CLOSING, "Channel state is CLOSING");
}

// Test channel force close
void test_channel_force_close() {
    std::cout << "\n=== Testing Channel Force Close ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    bool closed = channel.close_unilateral();

    test_assert(closed, "Channel force closed");
    test_assert(channel.state == ChannelState::FORCE_CLOSING, "Channel state is FORCE_CLOSING");
}

// Test commitment transaction creation
void test_commitment_transaction() {
    std::cout << "\n=== Testing Commitment Transaction ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    bool created = channel.create_new_commitment();

    test_assert(created, "Commitment transaction created");
    test_assert(channel.latest_commitment != nullptr, "Latest commitment exists");
    test_assert(channel.commitment_number == 1, "Commitment number incremented");
}

// Test commitment signing
void test_commitment_signing() {
    std::cout << "\n=== Testing Commitment Signing ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);
    channel.create_new_commitment();

    bool signed = channel.sign_commitment(keypair);

    test_assert(signed, "Commitment signed successfully");
    test_assert(channel.latest_commitment != nullptr, "Commitment still exists");
}

// Test Lightning node creation
void test_lightning_node() {
    std::cout << "\n=== Testing Lightning Node ===" << std::endl;

    auto keypair = crypto::Dilithium::generate_keypair();
    LightningNode node(keypair);

    test_assert(node.active_channel_count() == 0, "New node has no channels");

    auto stats = node.get_stats();
    test_assert(stats.total_channels == 0, "Stats show 0 channels");
    test_assert(stats.successful_payments == 0, "No successful payments");
}

// Test channel opening via node
void test_node_open_channel() {
    std::cout << "\n=== Testing Node Open Channel ===" << std::endl;

    auto keypair1 = crypto::Dilithium::generate_keypair();
    auto keypair2 = crypto::Dilithium::generate_keypair();

    LightningNode node(keypair1);

    auto channel_id = node.open_channel(keypair2.public_key, 1000000, 0);

    test_assert(channel_id.has_value(), "Channel opened via node");
    test_assert(node.active_channel_count() == 1, "Node has 1 active channel");
}

// Test invoice creation
void test_invoice_creation() {
    std::cout << "\n=== Testing Invoice Creation ===" << std::endl;

    auto keypair = crypto::Dilithium::generate_keypair();
    LightningNode node(keypair);

    auto invoice = node.create_invoice(50000, "Test payment");

    test_assert(!invoice.encoded_invoice.empty(), "Invoice has encoded string");
    test_assert(invoice.amount_sat == 50000, "Invoice amount correct");
    test_assert(invoice.description == "Test payment", "Invoice description correct");
    test_assert(invoice.preimage.size() == 32, "Preimage is 32 bytes");
}

// Test node statistics
void test_node_statistics() {
    std::cout << "\n=== Testing Node Statistics ===" << std::endl;

    auto keypair = crypto::Dilithium::generate_keypair();
    LightningNode node(keypair);

    // Open some channels
    auto remote_keypair = crypto::Dilithium::generate_keypair();
    node.open_channel(remote_keypair.public_key, 1000000, 0);
    node.open_channel(remote_keypair.public_key, 2000000, 0);

    auto stats = node.get_stats();

    test_assert(stats.total_channels == 2, "Stats show 2 channels");
    test_assert(stats.total_capacity_sat == 3000000, "Total capacity is 3M sats");
    test_assert(stats.total_local_balance_sat == 3000000, "Local balance is 3M sats");
}

// Test channel serialization
void test_channel_serialization() {
    std::cout << "\n=== Testing Channel Serialization ===" << std::endl;

    Channel channel;
    auto keypair = crypto::Dilithium::generate_keypair();
    channel.open(keypair.public_key, 1000000);

    // Serialize
    std::vector<uint8_t> serialized = channel.serialize();
    test_assert(!serialized.empty(), "Channel serialization produces data");

    // Deserialize
    Channel deserialized = Channel::deserialize(serialized);
    test_assert(deserialized.capacity_sat == channel.capacity_sat, "Capacity matches");
    test_assert(deserialized.state == channel.state, "State matches");
}

// Test Lightning message serialization
void test_lightning_messages() {
    std::cout << "\n=== Testing Lightning Messages ===" << std::endl;

    messages::Message msg;
    msg.type = messages::MessageType::OPEN_CHANNEL;
    msg.payload = {1, 2, 3, 4, 5};

    // Serialize
    std::vector<uint8_t> serialized = msg.serialize();
    test_assert(!serialized.empty(), "Message serialization produces data");

    // Deserialize
    messages::Message deserialized = messages::Message::deserialize(serialized);
    test_assert(deserialized.type == msg.type, "Message type matches");
    test_assert(deserialized.payload.size() == msg.payload.size(), "Payload size matches");
}

// Test route finding (basic)
void test_route_finding() {
    std::cout << "\n=== Testing Route Finding ===" << std::endl;

    auto keypair = crypto::Dilithium::generate_keypair();
    LightningNode node(keypair);

    auto destination = crypto::Dilithium::generate_keypair().public_key;

    // With no network graph, should return empty route
    auto route = node.find_route(destination, 10000);

    test_assert(route.empty(), "No route without network graph");
}

// Main test runner
int main() {
    std::cout << "INTcoin Lightning Network Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;

    // Run all tests
    test_htlc_creation();
    test_htlc_serialization();
    test_channel_creation();
    test_channel_opening();
    test_channel_can_send();
    test_htlc_addition();
    test_htlc_settlement();
    test_htlc_failure();
    test_channel_cooperative_close();
    test_channel_force_close();
    test_commitment_transaction();
    test_commitment_signing();
    test_lightning_node();
    test_node_open_channel();
    test_invoice_creation();
    test_node_statistics();
    test_channel_serialization();
    test_lightning_messages();
    test_route_finding();

    // Summary
    std::cout << "\n=====================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "=====================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
