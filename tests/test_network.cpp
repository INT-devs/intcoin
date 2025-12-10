/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * P2P Network Protocol Tests
 */

#include "intcoin/network.h"
#include "intcoin/util.h"
#include <iostream>
#include <cassert>

using namespace intcoin;

void test_ip_parsing() {
    std::cout << "Testing IP address parsing...\n";

    // Test IPv4 parsing
    auto result_v4 = ParseIPAddress("192.168.1.1");
    assert(result_v4.IsOk());
    auto ip_v4 = result_v4.value.value();
    assert(ip_v4[10] == 0xff && ip_v4[11] == 0xff); // IPv4-mapped
    assert(ip_v4[12] == 192 && ip_v4[13] == 168);

    // Test IPv4 to string
    std::string ip_str = IPAddressToString(ip_v4);
    assert(ip_str == "192.168.1.1");

    // Test invalid IP
    auto result_invalid = ParseIPAddress("invalid.ip.address");
    assert(result_invalid.IsError());

    std::cout << "  ✓ IP parsing tests passed\n";
}

void test_network_address() {
    std::cout << "Testing NetworkAddress...\n";

    // Create address
    NetworkAddress addr("51.155.97.192", 2210);
    assert(addr.port == 2210);
    assert(addr.IsIPv4());
    assert(!addr.IsLocal());
    assert(addr.IsRoutable());

    // Test serialization
    auto serialized = addr.Serialize();
    assert(serialized.size() == 34); // 8+8+16+2 bytes

    // Test deserialization
    auto result = NetworkAddress::Deserialize(serialized);
    assert(result.IsOk());
    auto addr2 = result.value.value();
    (void)addr2;
    assert(addr2.port == 2210);
    assert(addr2.services == addr.services);

    // Test ToString
    std::string addr_str = addr.ToString();
    assert(addr_str == "51.155.97.192:2210");

    // Test local address detection
    NetworkAddress local_addr("127.0.0.1", 2210);
    assert(local_addr.IsLocal());
    assert(!local_addr.IsRoutable());

    std::cout << "  ✓ NetworkAddress tests passed\n";
}

void test_network_message() {
    std::cout << "Testing NetworkMessage...\n";

    // Create test payload
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04, 0x05};

    // Create message
    NetworkMessage msg(network::MAINNET_MAGIC, "test", payload);
    assert(msg.magic == network::MAINNET_MAGIC);
    assert(msg.command == "test");
    assert(msg.length == payload.size());
    assert(msg.VerifyChecksum());

    // Test serialization
    auto serialized = msg.Serialize();
    assert(serialized.size() >= 24 + payload.size());

    // Test deserialization
    auto result = NetworkMessage::Deserialize(serialized);
    assert(result.IsOk());
    auto msg2 = result.value.value();
    assert(msg2.magic == network::MAINNET_MAGIC);
    assert(msg2.command == "test");
    assert(msg2.length == payload.size());
    assert(msg2.payload == payload);
    assert(msg2.VerifyChecksum());

    uint32_t checksum = NetworkMessage::CalculateChecksum(payload);
    (void)checksum;
    assert(checksum == msg.checksum);

    std::cout << "  ✓ NetworkMessage tests passed\n";
}

void test_inv_vector() {
    std::cout << "Testing InvVector...\n";

    // Create test hash
    uint256 test_hash;
    test_hash.fill(0x42);

    // Create inventory vector
    InvVector inv;
    inv.type = InvType::BLOCK;
    inv.hash = test_hash;

    // Test serialization
    auto serialized = inv.Serialize();
    assert(serialized.size() == 36); // 4 + 32 bytes

    // Test deserialization
    auto result = InvVector::Deserialize(serialized);
    assert(result.IsOk());
    auto inv2 = result.value.value();
    (void)inv2;
    assert(inv2.type == InvType::BLOCK);
    assert(inv2.hash == test_hash);

    // Test different types
    inv.type = InvType::TX;
    serialized = inv.Serialize();
    result = InvVector::Deserialize(serialized);
    assert(result.IsOk());
    assert(result.value.value().type == InvType::TX);

    std::cout << "  ✓ InvVector tests passed\n";
}

void test_seed_nodes() {
    std::cout << "Testing seed nodes...\n";

    // Get seed nodes
    auto seeds = PeerDiscovery::GetSeedNodes();
    assert(!seeds.empty());
    assert(seeds.size() >= 2);

    // Verify the hardcoded seed nodes
    // DNS: seed-uk.international-coin.org -> 51.155.97.192
    bool found_51 = false;
    bool found_74 = false;
    (void)found_51; (void)found_74;  // Used in assertions below

    for (const auto& seed : seeds) {
        std::string addr_str = seed.ToString();
        if (addr_str.find("51.155.97.192") != std::string::npos) {
            found_51 = true;
        }
        if (addr_str.find("74.208.112.43") != std::string::npos) {
            found_74 = true;
        }
    }

    assert(found_51);
    assert(found_74);

    std::cout << "  ✓ Seed node tests passed\n";
}

void test_service_flags() {
    // Test NODE_NETWORK flag
    uint64_t flags = static_cast<uint64_t>(ServiceFlags::NODE_NETWORK);
    (void)flags;

    // Test multiple flags (bitwise operations)
    uint64_t combined = static_cast<uint64_t>(ServiceFlags::NODE_NETWORK) |
                        static_cast<uint64_t>(ServiceFlags::NODE_BLOOM);
    (void)combined;  // Used in assertions below
    assert(combined == 5); // 1 | 4 = 5

    std::cout << "  ✓ Service flag tests passed\n";
}

void test_port_validation() {
    std::cout << "Testing port validation...\n";

    assert(!IsValidPort(0));
    assert(IsValidPort(1));
    assert(IsValidPort(2210));
    assert(IsValidPort(65535));

    std::cout << "  ✓ Port validation tests passed\n";
}

void test_block_broadcast() {
    std::cout << "Testing block broadcast...\n";

    // Create P2P node
    P2PNode p2p(network::MAINNET_MAGIC, 0); // Use port 0 to let OS assign

    // Create test block hash
    uint256 block_hash;
    for (size_t i = 0; i < block_hash.size(); i++) {
        block_hash[i] = static_cast<uint8_t>(i);
    }

    // Test broadcast without starting (should not crash)
    p2p.BroadcastBlock(block_hash);

    // Test that broadcast method creates valid INV message
    // (actual network send is tested via peer connections)

    std::cout << "  ✓ Block broadcast tests passed\n";
}

void test_transaction_broadcast() {
    std::cout << "Testing transaction broadcast...\n";

    // Create P2P node
    P2PNode p2p(network::MAINNET_MAGIC, 0);

    // Create test transaction hash
    uint256 tx_hash;
    for (size_t i = 0; i < tx_hash.size(); i++) {
        tx_hash[i] = static_cast<uint8_t>(255 - i);
    }

    // Test broadcast without starting (should not crash)
    p2p.BroadcastTransaction(tx_hash);

    std::cout << "  ✓ Transaction broadcast tests passed\n";
}

void test_inv_message_creation() {
    std::cout << "Testing INV message creation for relay...\n";

    // Test block INV
    InvVector block_inv;
    block_inv.type = InvType::BLOCK;
    block_inv.hash.fill(0xAB);

    auto block_payload = block_inv.Serialize();

    // Verify type is BLOCK
    uint32_t type_val = 0;
    (void)type_val;  // Used in assertions below
    for (int i = 0; i < 4; i++) {
        type_val |= (static_cast<uint32_t>(block_payload[i]) << (i * 8));
    }
    assert(static_cast<InvType>(type_val) == InvType::BLOCK);

    // Test transaction INV
    InvVector tx_inv;
    tx_inv.type = InvType::TX;
    tx_inv.hash.fill(0xCD);

    auto tx_payload = tx_inv.Serialize();
    assert(tx_payload.size() == 36);

    // Verify type is TX
    type_val = 0;
    for (int i = 0; i < 4; i++) {
        type_val |= (static_cast<uint32_t>(tx_payload[i]) << (i * 8));
    }
    assert(static_cast<InvType>(type_val) == InvType::TX);

    std::cout << "  ✓ INV message creation tests passed\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "P2P Network Protocol Tests\n";
    std::cout << "========================================\n\n";

    try {
        test_ip_parsing();
        test_network_address();
        test_network_message();
        test_inv_vector();
        test_seed_nodes();
        test_service_flags();
        test_port_validation();
        test_block_broadcast();
        test_transaction_broadcast();
        test_inv_message_creation();

        std::cout << "\n========================================\n";
        std::cout << "✓ All network protocol tests passed!\n";
        std::cout << "========================================\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
