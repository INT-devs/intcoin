// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Network Port Definitions - Independent from Bitcoin and other projects

#ifndef INTCOIN_NETWORK_PORTS_H
#define INTCOIN_NETWORK_PORTS_H

#include <cstdint>

namespace intcoin {
namespace network {

/**
 * INTcoin Network Port Allocation
 *
 * Base Port Range: 9330-9349 (20 ports reserved)
 *
 * This range is specifically chosen to:
 * - Avoid Bitcoin ports (8333, 8334, 18333, 18444)
 * - Avoid standard I2P ports (7656-7660, 4444-4447)
 * - Avoid Tor ports (9050, 9051, 9150, 9151)
 * - Avoid common cryptocurrency ports
 *
 * WARNING: Port 9333 conflicts with Litecoin! Do not run both on the same machine
 * without reconfiguring one of them.
 */

// ===== MAINNET PORTS =====

/** Main P2P network port for blockchain peer-to-peer communication */
constexpr uint16_t MAINNET_P2P_PORT = 9333;

/** JSON-RPC API server port (administrative access - DO NOT EXPOSE PUBLICLY) */
constexpr uint16_t MAINNET_RPC_PORT = 9334;

/**
 * Lightning Network P2P port
 * COMPLETELY INDEPENDENT from Bitcoin Lightning Network (port 9735)
 * Uses quantum-resistant cryptography (Dilithium5 + Kyber1024)
 * No Bitcoin dependency - runs entirely on INTcoin blockchain
 */
constexpr uint16_t MAINNET_LIGHTNING_PORT = 9335;

/** I2P SAM (Simple Anonymous Messaging) bridge port */
constexpr uint16_t MAINNET_I2P_SAM_PORT = 9336;

/** I2P router internal port for I2P network routing */
constexpr uint16_t MAINNET_I2P_ROUTER_PORT = 9337;

/** Custom Tor control port (if running dedicated instance) */
constexpr uint16_t MAINNET_TOR_CONTROL_PORT = 9338;

/** Lightning Network watchtower service port */
constexpr uint16_t MAINNET_WATCHTOWER_PORT = 9339;

/** HTTP-based blockchain explorer web interface */
constexpr uint16_t MAINNET_EXPLORER_PORT = 9340;

/** WebSocket API for real-time blockchain updates */
constexpr uint16_t MAINNET_WEBSOCKET_PORT = 9341;

/** gRPC API for high-performance client applications */
constexpr uint16_t MAINNET_GRPC_PORT = 9342;

// ===== TESTNET PORTS (Add +10000) =====

constexpr uint16_t TESTNET_P2P_PORT = 19333;
constexpr uint16_t TESTNET_RPC_PORT = 19334;
constexpr uint16_t TESTNET_LIGHTNING_PORT = 19335;
constexpr uint16_t TESTNET_I2P_SAM_PORT = 19336;
constexpr uint16_t TESTNET_I2P_ROUTER_PORT = 19337;
constexpr uint16_t TESTNET_TOR_CONTROL_PORT = 19338;
constexpr uint16_t TESTNET_WATCHTOWER_PORT = 19339;
constexpr uint16_t TESTNET_EXPLORER_PORT = 19340;
constexpr uint16_t TESTNET_WEBSOCKET_PORT = 19341;
constexpr uint16_t TESTNET_GRPC_PORT = 19342;

// ===== REGTEST PORTS (Add +20000) =====

constexpr uint16_t REGTEST_P2P_PORT = 29333;
constexpr uint16_t REGTEST_RPC_PORT = 29334;
constexpr uint16_t REGTEST_LIGHTNING_PORT = 29335;
constexpr uint16_t REGTEST_I2P_SAM_PORT = 29336;
constexpr uint16_t REGTEST_I2P_ROUTER_PORT = 29337;
constexpr uint16_t REGTEST_TOR_CONTROL_PORT = 29338;
constexpr uint16_t REGTEST_WATCHTOWER_PORT = 29339;
constexpr uint16_t REGTEST_EXPLORER_PORT = 29340;
constexpr uint16_t REGTEST_WEBSOCKET_PORT = 29341;
constexpr uint16_t REGTEST_GRPC_PORT = 29342;

/**
 * Network type enumeration
 */
enum class NetworkType {
    MAINNET,
    TESTNET,
    REGTEST
};

/**
 * Service type enumeration
 */
enum class ServiceType {
    P2P,
    RPC,
    LIGHTNING,
    I2P_SAM,
    I2P_ROUTER,
    TOR_CONTROL,
    WATCHTOWER,
    EXPLORER,
    WEBSOCKET,
    GRPC
};

/**
 * Get port for a specific network and service
 */
inline uint16_t get_port(NetworkType network, ServiceType service) {
    // Base ports for mainnet
    constexpr uint16_t base_ports[] = {
        MAINNET_P2P_PORT,           // P2P
        MAINNET_RPC_PORT,           // RPC
        MAINNET_LIGHTNING_PORT,     // LIGHTNING
        MAINNET_I2P_SAM_PORT,       // I2P_SAM
        MAINNET_I2P_ROUTER_PORT,    // I2P_ROUTER
        MAINNET_TOR_CONTROL_PORT,   // TOR_CONTROL
        MAINNET_WATCHTOWER_PORT,    // WATCHTOWER
        MAINNET_EXPLORER_PORT,      // EXPLORER
        MAINNET_WEBSOCKET_PORT,     // WEBSOCKET
        MAINNET_GRPC_PORT           // GRPC
    };

    uint16_t base_port = base_ports[static_cast<int>(service)];

    switch (network) {
        case NetworkType::MAINNET:
            return base_port;
        case NetworkType::TESTNET:
            return base_port + 10000;
        case NetworkType::REGTEST:
            return base_port + 20000;
        default:
            return base_port;
    }
}

/**
 * Port validation - check if port is in valid INTcoin range
 */
inline bool is_valid_intcoin_port(uint16_t port) {
    // Mainnet range
    if (port >= 9333 && port <= 9342) return true;
    // Testnet range
    if (port >= 19333 && port <= 19342) return true;
    // Regtest range
    if (port >= 29333 && port <= 29342) return true;
    return false;
}

/**
 * Check if port is safe to expose publicly
 */
inline bool is_safe_to_expose(ServiceType service) {
    switch (service) {
        case ServiceType::P2P:
        case ServiceType::LIGHTNING:
        case ServiceType::WATCHTOWER:
        case ServiceType::EXPLORER:
            return true;  // These are designed for public access
        case ServiceType::RPC:
        case ServiceType::I2P_SAM:
        case ServiceType::I2P_ROUTER:
        case ServiceType::TOR_CONTROL:
        case ServiceType::WEBSOCKET:
        case ServiceType::GRPC:
            return false;  // These should be firewalled or use authentication
        default:
            return false;
    }
}

/**
 * Port conflict detection
 */
struct PortConflict {
    uint16_t port;
    const char* other_project;
    const char* service;
};

/**
 * Known port conflicts with other projects
 */
inline PortConflict get_known_conflicts() {
    // INTcoin port 9333 conflicts with Litecoin P2P
    return PortConflict{9333, "Litecoin", "P2P network"};
}

/**
 * Lightning Network Independence Declaration
 *
 * INTcoin Lightning Network is 100% INDEPENDENT from Bitcoin Lightning:
 * - Different port: 9335 (not 9735)
 * - Different cryptography: Dilithium5 + Kyber1024 (not ECDSA)
 * - Different message format: Modified BOLT for post-quantum
 * - Different invoice format: lnint prefix (not lnbc)
 * - Different network: INTcoin blockchain (not Bitcoin)
 * - NO Bitcoin dependency
 * - NO cross-chain compatibility with Bitcoin Lightning
 *
 * This is a completely separate implementation adapted for quantum resistance.
 */
namespace lightning {
    constexpr uint16_t BITCOIN_LIGHTNING_PORT = 9735;
    constexpr uint16_t INTCOIN_LIGHTNING_PORT = 9335;

    static_assert(BITCOIN_LIGHTNING_PORT != INTCOIN_LIGHTNING_PORT,
                  "INTcoin Lightning must use different port from Bitcoin");

    constexpr bool INDEPENDENT_FROM_BITCOIN = true;
    constexpr bool QUANTUM_RESISTANT = true;
    constexpr const char* INVOICE_PREFIX = "lnint";  // Not "lnbc"
}

/**
 * I2P Network Independence Declaration
 *
 * INTcoin I2P integration uses custom ports to avoid conflicts:
 * - SAM bridge: 9336 (not standard 7656)
 * - Router: 9337 (not standard 7654-7660 range)
 *
 * This allows running INTcoin alongside other I2P applications.
 */
namespace i2p {
    constexpr uint16_t STANDARD_SAM_PORT = 7656;
    constexpr uint16_t INTCOIN_SAM_PORT = 9336;

    constexpr uint16_t STANDARD_ROUTER_PORT_MIN = 7654;
    constexpr uint16_t STANDARD_ROUTER_PORT_MAX = 7660;
    constexpr uint16_t INTCOIN_ROUTER_PORT = 9337;

    static_assert(INTCOIN_SAM_PORT != STANDARD_SAM_PORT,
                  "INTcoin I2P must use different SAM port");
    static_assert(INTCOIN_ROUTER_PORT < STANDARD_ROUTER_PORT_MIN ||
                  INTCOIN_ROUTER_PORT > STANDARD_ROUTER_PORT_MAX,
                  "INTcoin I2P router must use port outside standard range");
}

/**
 * Tor Network Configuration
 *
 * INTcoin can use standard Tor ports or custom instance:
 * - Standard Tor SOCKS5: 9050
 * - Standard Tor control: 9051
 * - Custom INTcoin Tor control: 9338
 */
namespace tor {
    constexpr uint16_t STANDARD_SOCKS_PORT = 9050;
    constexpr uint16_t STANDARD_CONTROL_PORT = 9051;
    constexpr uint16_t INTCOIN_CONTROL_PORT = 9338;
}

/**
 * Default port configuration
 */
struct DefaultPorts {
    uint16_t p2p;
    uint16_t rpc;
    uint16_t lightning;
    uint16_t i2p_sam;
    uint16_t i2p_router;
    uint16_t tor_control;
    uint16_t watchtower;
    uint16_t explorer;
    uint16_t websocket;
    uint16_t grpc;

    static DefaultPorts mainnet() {
        return DefaultPorts{
            MAINNET_P2P_PORT,
            MAINNET_RPC_PORT,
            MAINNET_LIGHTNING_PORT,
            MAINNET_I2P_SAM_PORT,
            MAINNET_I2P_ROUTER_PORT,
            MAINNET_TOR_CONTROL_PORT,
            MAINNET_WATCHTOWER_PORT,
            MAINNET_EXPLORER_PORT,
            MAINNET_WEBSOCKET_PORT,
            MAINNET_GRPC_PORT
        };
    }

    static DefaultPorts testnet() {
        return DefaultPorts{
            TESTNET_P2P_PORT,
            TESTNET_RPC_PORT,
            TESTNET_LIGHTNING_PORT,
            TESTNET_I2P_SAM_PORT,
            TESTNET_I2P_ROUTER_PORT,
            TESTNET_TOR_CONTROL_PORT,
            TESTNET_WATCHTOWER_PORT,
            TESTNET_EXPLORER_PORT,
            TESTNET_WEBSOCKET_PORT,
            TESTNET_GRPC_PORT
        };
    }

    static DefaultPorts regtest() {
        return DefaultPorts{
            REGTEST_P2P_PORT,
            REGTEST_RPC_PORT,
            REGTEST_LIGHTNING_PORT,
            REGTEST_I2P_SAM_PORT,
            REGTEST_I2P_ROUTER_PORT,
            REGTEST_TOR_CONTROL_PORT,
            REGTEST_WATCHTOWER_PORT,
            REGTEST_EXPLORER_PORT,
            REGTEST_WEBSOCKET_PORT,
            REGTEST_GRPC_PORT
        };
    }
};

} // namespace network
} // namespace intcoin

#endif // INTCOIN_NETWORK_PORTS_H
