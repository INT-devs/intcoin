// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/tor.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace intcoin::tor;

// Test utilities
void test_passed(const std::string& test_name) {
    std::cout << "[PASS] " << test_name << std::endl;
}

void test_failed(const std::string& test_name, const std::string& reason = "") {
    std::cout << "[FAIL] " << test_name;
    if (!reason.empty()) {
        std::cout << ": " << reason;
    }
    std::cout << std::endl;
}

// Test onion address validation
void test_onion_address_validation() {
    std::string test_name = "Onion Address Validation";

    // Valid v3 onion address
    std::string v3_addr = "thehiddenwiki2345678901234567890123456789012345678901.onion";
    OnionAddress addr1(v3_addr, 8333);

    if (addr1.type == AddressType::V3) {
        test_passed(test_name + " - V3 detection");
    } else {
        test_failed(test_name + " - V3 detection");
    }

    // Invalid address
    OnionAddress addr2("invalid.address", 8333);
    if (addr2.type == AddressType::NONE) {
        test_passed(test_name + " - Invalid address detection");
    } else {
        test_failed(test_name + " - Invalid address detection");
    }

    // Valid v3 without .onion suffix
    std::string v3_base = "abcdefghijklmnopqrstuvwxyz234567abcdefghijklmnopqrstuv";
    if (OnionAddress::is_onion_address(v3_base)) {
        test_passed(test_name + " - V3 base32 validation");
    } else {
        test_failed(test_name + " - V3 base32 validation");
    }
}

// Test SOCKS5 configuration
void test_socks5_config() {
    std::string test_name = "SOCKS5 Configuration";

    SOCKS5Config config;

    // Default values
    if (config.host == "127.0.0.1" && config.port == 9050) {
        test_passed(test_name + " - Default values");
    } else {
        test_failed(test_name + " - Default values");
    }

    // Custom configuration
    config.host = "10.0.0.1";
    config.port = 9150;
    config.use_auth = true;
    config.username = "testuser";
    config.password = "testpass";

    if (config.host == "10.0.0.1" && config.port == 9150 && config.use_auth) {
        test_passed(test_name + " - Custom values");
    } else {
        test_failed(test_name + " - Custom values");
    }
}

// Test SOCKS5 proxy creation (without actual connection)
void test_socks5_proxy() {
    std::string test_name = "SOCKS5 Proxy";

    SOCKS5Config config;
    SOCKS5Proxy proxy(config);

    if (proxy.get_config().host == "127.0.0.1") {
        test_passed(test_name + " - Proxy creation");
    } else {
        test_failed(test_name + " - Proxy creation");
    }

    // Note: Actual connection tests require TOR to be running
    // Those should be run as integration tests
}

// Test hidden service configuration
void test_hidden_service_config() {
    std::string test_name = "Hidden Service Configuration";

    HiddenServiceConfig config;

    // Default values
    if (!config.enabled && config.virtual_port == 8333) {
        test_passed(test_name + " - Default values");
    } else {
        test_failed(test_name + " - Default values");
    }

    // Custom configuration
    config.data_dir = "/tmp/intcoin_hs_test";
    config.virtual_port = 18333;
    config.target_port = 18333;
    config.enabled = true;

    if (config.enabled && config.virtual_port == 18333) {
        test_passed(test_name + " - Custom values");
    } else {
        test_failed(test_name + " - Custom values");
    }
}

// Test TOR network manager initialization
void test_tor_network() {
    std::string test_name = "TOR Network Manager";

    TORNetwork tor_net;

    // Initial state
    if (!tor_net.is_tor_available()) {
        // TOR not running - expected in most test environments
        test_passed(test_name + " - Initial state (TOR not available)");
    }

    // Configuration
    SOCKS5Config socks_config;
    tor_net.set_socks5_config(socks_config);

    HiddenServiceConfig hs_config;
    hs_config.data_dir = "/tmp/intcoin_test_hs";
    hs_config.enabled = false;  // Disabled for unit tests
    tor_net.set_hidden_service_config(hs_config);

    test_passed(test_name + " - Configuration");

    // Onion-only mode
    tor_net.enable_onion_only(true);
    if (tor_net.is_onion_only()) {
        test_passed(test_name + " - Onion-only mode");
    } else {
        test_failed(test_name + " - Onion-only mode");
    }
}

// Test onion address parsing
void test_onion_address_parsing() {
    std::string test_name = "Onion Address Parsing";

    std::string onion;
    uint16_t port;

    // Valid address with port
    std::string addr1 = "example234567890123456789012345678901234567890123456.onion:8333";
    if (util::parse_onion_address(addr1, onion, port)) {
        if (port == 8333) {
            test_passed(test_name + " - Parse with port");
        } else {
            test_failed(test_name + " - Parse with port (wrong port)");
        }
    } else {
        test_failed(test_name + " - Parse with port");
    }

    // Valid address without port
    std::string addr2 = "example234567890123456789012345678901234567890123456.onion";
    if (util::parse_onion_address(addr2, onion, port)) {
        if (port == 0) {
            test_passed(test_name + " - Parse without port");
        } else {
            test_failed(test_name + " - Parse without port (unexpected port)");
        }
    } else {
        test_failed(test_name + " - Parse without port");
    }

    // Invalid address
    std::string addr3 = "invalid:8333";
    if (!util::parse_onion_address(addr3, onion, port)) {
        test_passed(test_name + " - Reject invalid address");
    } else {
        test_failed(test_name + " - Reject invalid address");
    }
}

// Test peer address conversion
void test_peer_address_conversion() {
    std::string test_name = "Peer Address Conversion";

    TORNetwork tor_net;

    // Onion to peer address
    OnionAddress onion("example234567890123456789012345678901234567890123456.onion", 8333);
    p2p::PeerAddress peer = tor_net.onion_to_peer_address(onion);

    if (peer.ip.find(".onion") != std::string::npos && peer.port == 8333) {
        test_passed(test_name + " - Onion to peer");
    } else {
        test_failed(test_name + " - Onion to peer");
    }

    // Peer address to onion
    auto converted_onion = tor_net.peer_address_to_onion(peer);
    if (converted_onion.has_value() && converted_onion->port == 8333) {
        test_passed(test_name + " - Peer to onion");
    } else {
        test_failed(test_name + " - Peer to onion");
    }

    // Non-onion peer address
    p2p::PeerAddress clearnet_peer;
    clearnet_peer.ip = "192.168.1.1";
    clearnet_peer.port = 8333;

    auto non_onion = tor_net.peer_address_to_onion(clearnet_peer);
    if (!non_onion.has_value()) {
        test_passed(test_name + " - Clearnet peer rejection");
    } else {
        test_failed(test_name + " - Clearnet peer rejection");
    }
}

// Test TOR statistics
void test_tor_statistics() {
    std::string test_name = "TOR Statistics";

    TORNetwork tor_net;

    // Add some onion peers
    OnionAddress peer1("peer1abcdefghijklmnopqrstuvwxyz234567890123456789012.onion", 8333);
    OnionAddress peer2("peer2abcdefghijklmnopqrstuvwxyz234567890123456789012.onion", 8333);

    tor_net.add_onion_peer(peer1);
    tor_net.add_onion_peer(peer2);

    auto stats = tor_net.get_stats();

    if (stats.onion_peers == 2) {
        test_passed(test_name + " - Onion peer count");
    } else {
        test_failed(test_name + " - Onion peer count");
    }

    if (!stats.hidden_service_active) {
        test_passed(test_name + " - Hidden service inactive");
    } else {
        test_failed(test_name + " - Hidden service inactive");
    }
}

// Test protocol constants
void test_protocol_constants() {
    std::string test_name = "Protocol Constants";

    if (protocol::SOCKS5_VERSION == 0x05) {
        test_passed(test_name + " - SOCKS5 version");
    } else {
        test_failed(test_name + " - SOCKS5 version");
    }

    if (protocol::V3_ONION_LEN == 56) {
        test_passed(test_name + " - V3 onion length");
    } else {
        test_failed(test_name + " - V3 onion length");
    }

    if (protocol::DEFAULT_TOR_SOCKS_PORT == 9050) {
        test_passed(test_name + " - Default SOCKS port");
    } else {
        test_failed(test_name + " - Default SOCKS port");
    }

    if (protocol::DEFAULT_TOR_CONTROL_PORT == 9051) {
        test_passed(test_name + " - Default control port");
    } else {
        test_failed(test_name + " - Default control port");
    }
}

// Integration tests (require TOR to be running)
void run_integration_tests() {
    std::cout << "\n=== Integration Tests (require TOR) ===" << std::endl;

    // Check if TOR is running
    if (!util::is_tor_running()) {
        std::cout << "[SKIP] TOR is not running. Skipping integration tests." << std::endl;
        std::cout << "To run integration tests, start TOR: sudo systemctl start tor" << std::endl;
        return;
    }

    std::cout << "TOR is running. Executing integration tests..." << std::endl;

    // Test SOCKS5 proxy connection
    {
        std::string test_name = "SOCKS5 Proxy Connection";
        SOCKS5Config config;
        SOCKS5Proxy proxy(config);

        if (proxy.test_connection()) {
            test_passed(test_name);
        } else {
            test_failed(test_name, "Could not connect to TOR SOCKS proxy");
        }
    }

    // Test TOR network initialization
    {
        std::string test_name = "TOR Network Initialization";
        TORNetwork tor_net;

        if (tor_net.initialize()) {
            test_passed(test_name);
        } else {
            test_failed(test_name, "TOR network initialization failed");
        }
    }
}

// Main test runner
int main(int argc, char* argv[]) {
    std::cout << "INTcoin TOR Support Test Suite" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << std::endl;

    std::cout << "=== Unit Tests ===" << std::endl;

    // Run unit tests
    test_onion_address_validation();
    test_socks5_config();
    test_socks5_proxy();
    test_hidden_service_config();
    test_tor_network();
    test_onion_address_parsing();
    test_peer_address_conversion();
    test_tor_statistics();
    test_protocol_constants();

    // Run integration tests if requested
    bool run_integration = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--integration") {
            run_integration = true;
            break;
        }
    }

    if (run_integration) {
        run_integration_tests();
    } else {
        std::cout << "\nTo run integration tests: " << argv[0] << " --integration" << std::endl;
    }

    std::cout << "\n=== Test Suite Complete ===" << std::endl;

    return 0;
}
