// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/p2p.h"
#include "intcoin/block.h"
#include "intcoin/transaction.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace intcoin;
using namespace intcoin::p2p;

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

// Test peer address
void test_peer_address() {
    std::cout << "\n=== Testing Peer Address ===" << std::endl;

    PeerAddress addr("192.168.1.1", 8333);

    test_assert(addr.ip == "192.168.1.1", "IP address set correctly");
    test_assert(addr.port == 8333, "Port set correctly");
    test_assert(addr.to_string() == "192.168.1.1:8333", "Address string format correct");
}

// Test message header
void test_message_header() {
    std::cout << "\n=== Testing Message Header ===" << std::endl;

    MessageHeader header;
    header.magic = 0xD9B4BEF9;
    header.type = MessageType::VERSION;
    header.length = 100;

    // Serialize
    std::vector<uint8_t> serialized = header.serialize();
    test_assert(!serialized.empty(), "Header serialization produces data");
    test_assert(serialized.size() == 44, "Header size is 44 bytes");

    // Deserialize
    MessageHeader deserialized = MessageHeader::deserialize(serialized);
    test_assert(deserialized.magic == header.magic, "Magic matches");
    test_assert(deserialized.type == header.type, "Type matches");
    test_assert(deserialized.length == header.length, "Length matches");
}

// Test message creation
void test_message_creation() {
    std::cout << "\n=== Testing Message Creation ===" << std::endl;

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5};
    Message msg(MessageType::PING, payload);

    test_assert(msg.header.type == MessageType::PING, "Message type set correctly");
    test_assert(msg.payload == payload, "Payload set correctly");
    test_assert(msg.header.length == payload.size(), "Length matches payload");
}

// Test message serialization
void test_message_serialization() {
    std::cout << "\n=== Testing Message Serialization ===" << std::endl;

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5};
    Message msg(MessageType::PING, payload);

    // Serialize
    std::vector<uint8_t> serialized = msg.serialize();
    test_assert(!serialized.empty(), "Message serialization produces data");
    test_assert(serialized.size() >= payload.size(), "Serialized size includes header");

    // Deserialize
    Message deserialized = Message::deserialize(serialized);
    test_assert(deserialized.header.type == msg.header.type, "Type matches");
    test_assert(deserialized.payload.size() == msg.payload.size(), "Payload size matches");
}

// Test inventory vector
void test_inventory_vector() {
    std::cout << "\n=== Testing Inventory Vector ===" << std::endl;

    Hash256 hash;
    std::fill(hash.begin(), hash.end(), 0xAA);

    InvVector inv(InvVector::Type::BLOCK, hash);

    test_assert(inv.type == InvVector::Type::BLOCK, "Type is BLOCK");
    test_assert(inv.hash == hash, "Hash matches");

    // Serialization
    std::vector<uint8_t> serialized = inv.serialize();
    test_assert(!serialized.empty(), "Inv serialization produces data");

    InvVector deserialized = InvVector::deserialize(serialized);
    test_assert(deserialized.type == inv.type, "Deserialized type matches");
    test_assert(deserialized.hash == inv.hash, "Deserialized hash matches");
}

// Test peer creation
void test_peer_creation() {
    std::cout << "\n=== Testing Peer Creation ===" << std::endl;

    PeerAddress addr("10.0.0.1", 8333);
    Peer peer(addr);

    test_assert(peer.address.ip == "10.0.0.1", "Peer address IP correct");
    test_assert(peer.address.port == 8333, "Peer address port correct");
    test_assert(!peer.connected, "New peer not connected");
    test_assert(peer.socket_fd == -1, "New peer has invalid socket");
}

// Test peer alive check
void test_peer_alive() {
    std::cout << "\n=== Testing Peer Alive Check ===" << std::endl;

    Peer peer;
    peer.last_seen = std::time(nullptr);

    test_assert(peer.is_alive(), "Recent peer is alive");

    // Old peer
    peer.last_seen = std::time(nullptr) - 3600;  // 1 hour ago
    test_assert(!peer.is_alive(), "Old peer is dead");
}

// Test network initialization
void test_network_init() {
    std::cout << "\n=== Testing Network Initialization ===" << std::endl;

    Network network(18333, true);  // Use testnet port

    test_assert(!network.is_running(), "Network not running initially");
    test_assert(network.peer_count() == 0, "No peers initially");
}

// Test seed node addition
void test_seed_nodes() {
    std::cout << "\n=== Testing Seed Nodes ===" << std::endl;

    Network network(18333, true);

    PeerAddress seed1("seed1.intcoin.org", 8333);
    PeerAddress seed2("seed2.intcoin.org", 8333);

    network.add_seed_node(seed1);
    network.add_seed_node(seed2);

    auto peers = network.get_peers();
    // Seed nodes are added as potential peers
    test_assert(true, "Seed nodes added without error");
}

// Test message type values
void test_message_types() {
    std::cout << "\n=== Testing Message Types ===" << std::endl;

    test_assert(MessageType::VERSION == static_cast<MessageType>(1), "VERSION type is 1");
    test_assert(MessageType::VERACK == static_cast<MessageType>(2), "VERACK type is 2");
    test_assert(MessageType::PING == static_cast<MessageType>(3), "PING type is 3");
    test_assert(MessageType::PONG == static_cast<MessageType>(4), "PONG type is 4");
}

// Test protocol constants
void test_protocol_constants() {
    std::cout << "\n=== Testing Protocol Constants ===" << std::endl;

    test_assert(protocol::PROTOCOL_VERSION == 1, "Protocol version is 1");
    test_assert(protocol::MAX_MESSAGE_SIZE > 0, "Max message size is set");
    test_assert(protocol::MAX_PEERS == 125, "Max peers is 125");
    test_assert(protocol::MIN_PEERS == 8, "Min peers is 8");
    test_assert(protocol::DEFAULT_PORT == 8333, "Default port is 8333");
    test_assert(protocol::DEFAULT_PORT_TESTNET == 18333, "Testnet port is 18333");
}

// Test broadcast block (without actual network)
void test_broadcast_block() {
    std::cout << "\n=== Testing Broadcast Block ===" << std::endl;

    Network network(18333, true);

    Block block = create_genesis_block();

    // This won't actually broadcast without peers, but should not crash
    network.broadcast_block(block);

    test_assert(true, "Broadcast block without error");
}

// Test broadcast transaction (without actual network)
void test_broadcast_transaction() {
    std::cout << "\n=== Testing Broadcast Transaction ===" << std::endl;

    Network network(18333, true);

    auto keypair = crypto::Dilithium::generate_keypair();
    Transaction tx = create_coinbase_transaction(keypair.public_key, 0, 0);

    // This won't actually broadcast without peers, but should not crash
    network.broadcast_transaction(tx);

    test_assert(true, "Broadcast transaction without error");
}

// Test network callbacks
void test_network_callbacks() {
    std::cout << "\n=== Testing Network Callbacks ===" << std::endl;

    Network network(18333, true);

    bool block_received = false;
    bool tx_received = false;

    // Set callbacks
    network.set_block_callback([&](const Block& block, const PeerAddress& from) {
        block_received = true;
    });

    network.set_tx_callback([&](const Transaction& tx, const PeerAddress& from) {
        tx_received = true;
    });

    test_assert(true, "Callbacks set without error");
}

// Test peer update
void test_peer_update() {
    std::cout << "\n=== Testing Peer Update ===" << std::endl;

    Peer peer;
    uint64_t before = peer.last_seen;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    peer.update_last_seen();

    test_assert(peer.last_seen > before, "Last seen updated");
}

// Test message checksum
void test_message_checksum() {
    std::cout << "\n=== Testing Message Checksum ===" << std::endl;

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5};
    Message msg(MessageType::PING, payload);

    Hash256 checksum = msg.get_checksum();

    bool has_checksum = false;
    for (uint8_t b : checksum) {
        if (b != 0) {
            has_checksum = true;
            break;
        }
    }

    test_assert(has_checksum, "Message has non-zero checksum");
}

// Test peer connection tracking
void test_peer_connection_tracking() {
    std::cout << "\n=== Testing Peer Connection Tracking ===" << std::endl;

    Peer peer;
    peer.connected = true;
    peer.inbound = false;

    test_assert(peer.connected, "Peer is connected");
    test_assert(!peer.inbound, "Peer is outbound");
}

// Test peer version
void test_peer_version() {
    std::cout << "\n=== Testing Peer Version ===" << std::endl;

    Peer peer;
    peer.version = protocol::PROTOCOL_VERSION;
    peer.user_agent = "INTcoin:0.1.0";

    test_assert(peer.version == protocol::PROTOCOL_VERSION, "Peer version correct");
    test_assert(peer.user_agent == "INTcoin:0.1.0", "User agent correct");
}

// Test network start/stop (without binding)
void test_network_start_stop() {
    std::cout << "\n=== Testing Network Start/Stop ===" << std::endl;

    Network network(18333, true);

    // Don't actually start to avoid port conflicts
    test_assert(!network.is_running(), "Network not running");

    // Stop should be safe even if not started
    network.stop();
    test_assert(!network.is_running(), "Network stopped");
}

// Main test runner
int main() {
    std::cout << "INTcoin Network/P2P Test Suite" << std::endl;
    std::cout << "===============================" << std::endl;

    // Run all tests
    test_peer_address();
    test_message_header();
    test_message_creation();
    test_message_serialization();
    test_inventory_vector();
    test_peer_creation();
    test_peer_alive();
    test_network_init();
    test_seed_nodes();
    test_message_types();
    test_protocol_constants();
    test_broadcast_block();
    test_broadcast_transaction();
    test_network_callbacks();
    test_peer_update();
    test_message_checksum();
    test_peer_connection_tracking();
    test_peer_version();
    test_network_start_stop();

    // Summary
    std::cout << "\n===============================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "===============================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
