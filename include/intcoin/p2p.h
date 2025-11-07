// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// P2P networking layer for node communication.

#ifndef INTCOIN_P2P_H
#define INTCOIN_P2P_H

#include "primitives.h"
#include "block.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>

namespace intcoin {
namespace p2p {

/**
 * Network message types
 */
enum class MessageType : uint32_t {
    VERSION = 1,        // Initial handshake
    VERACK,            // Acknowledge version
    PING,              // Keepalive
    PONG,              // Pong response
    GETADDR,           // Request peer addresses
    ADDR,              // Peer addresses
    INV,               // Inventory advertisement
    GETDATA,           // Request data
    BLOCK,             // Block data
    TX,                // Transaction data
    GETBLOCKS,         // Request block hashes
    GETHEADERS,        // Request block headers
    HEADERS,           // Block headers
    MEMPOOL,           // Request mempool
    REJECT,            // Reject message
    NOTFOUND           // Data not found
};

/**
 * Peer address information
 */
struct PeerAddress {
    std::string ip;
    uint16_t port;
    uint64_t timestamp;
    uint64_t services;  // Service flags

    PeerAddress() : ip(""), port(0), timestamp(0), services(0) {}
    PeerAddress(const std::string& addr, uint16_t p)
        : ip(addr), port(p), timestamp(0), services(1) {}

    std::string to_string() const {
        return ip + ":" + std::to_string(port);
    }
};

/**
 * Network message header
 */
struct MessageHeader {
    uint32_t magic;         // Network magic bytes
    MessageType type;       // Message type
    uint32_t length;        // Payload length
    Hash256 checksum;       // Payload checksum

    MessageHeader() : magic(0), type(MessageType::VERSION), length(0), checksum{} {}

    std::vector<uint8_t> serialize() const;
    static MessageHeader deserialize(const std::vector<uint8_t>& data);
};

/**
 * Network message
 */
class Message {
public:
    MessageHeader header;
    std::vector<uint8_t> payload;

    Message() = default;
    Message(MessageType type, const std::vector<uint8_t>& data);

    std::vector<uint8_t> serialize() const;
    static Message deserialize(const std::vector<uint8_t>& data);

    Hash256 get_checksum() const;
};

/**
 * Inventory vector (announces objects)
 */
struct InvVector {
    enum class Type : uint32_t {
        ERROR = 0,
        TX = 1,
        BLOCK = 2,
        FILTERED_BLOCK = 3
    };

    Type type;
    Hash256 hash;

    InvVector() : type(Type::ERROR), hash{} {}
    InvVector(Type t, const Hash256& h) : type(t), hash(h) {}

    std::vector<uint8_t> serialize() const;
    static InvVector deserialize(const std::vector<uint8_t>& data);
};

/**
 * Peer connection
 */
class Peer {
public:
    PeerAddress address;
    bool connected;
    bool inbound;           // True if peer connected to us
    uint64_t last_seen;
    uint32_t version;
    std::string user_agent;
    uint32_t start_height;
    int socket_fd;          // Socket file descriptor
    uint32_t protocol_version;
    uint64_t services;

    Peer() : connected(false), inbound(false), last_seen(0), version(0), start_height(0),
             socket_fd(-1), protocol_version(0), services(0) {}
    explicit Peer(const PeerAddress& addr)
        : address(addr), connected(false), inbound(false), last_seen(0), version(0), start_height(0),
          socket_fd(-1), protocol_version(0), services(0) {}

    bool is_alive() const;
    void update_last_seen();
};

/**
 * P2P Network manager
 */
class Network {
public:
    Network(uint16_t port, bool is_testnet = false);
    ~Network();

    // Connection management
    bool start();
    void stop();
    bool connect_to_peer(const PeerAddress& addr);
    void disconnect_peer(const PeerAddress& addr);

    // Message handling
    void broadcast_block(const Block& block);
    void broadcast_transaction(const Transaction& tx);
    void send_message(const PeerAddress& addr, const Message& msg);

    // Peer discovery
    std::vector<PeerAddress> get_peers() const;
    void add_seed_node(const PeerAddress& addr);

    // Callbacks
    using BlockCallback = std::function<void(const Block&, const PeerAddress&)>;
    using TxCallback = std::function<void(const Transaction&, const PeerAddress&)>;
    using BlockLookupCallback = std::function<std::optional<Block>(const Hash256&)>;
    using TxLookupCallback = std::function<std::optional<Transaction>(const Hash256&)>;

    void set_block_callback(BlockCallback cb) { block_callback_ = cb; }
    void set_tx_callback(TxCallback cb) { tx_callback_ = cb; }
    void set_block_lookup_callback(BlockLookupCallback cb) { block_lookup_callback_ = cb; }
    void set_tx_lookup_callback(TxLookupCallback cb) { tx_lookup_callback_ = cb; }

    // Status
    size_t peer_count() const;
    bool is_running() const { return running_; }

private:
    [[maybe_unused]] uint16_t port_;
    [[maybe_unused]] bool is_testnet_;
    bool running_;

    int listen_socket_;
    std::thread discovery_thread_;
    std::thread maintenance_thread_;
    std::thread accept_thread_;

    std::vector<Peer> peers_;
    std::vector<PeerAddress> seed_nodes_;

    BlockCallback block_callback_;
    TxCallback tx_callback_;
    BlockLookupCallback block_lookup_callback_;
    TxLookupCallback tx_lookup_callback_;

    // Internal message handlers
    void handle_version(const Message& msg, const PeerAddress& from);
    void handle_inv(const Message& msg, const PeerAddress& from);
    void handle_getdata(const Message& msg, const PeerAddress& from);
    void handle_block(const Message& msg, const PeerAddress& from);
    void handle_tx(const Message& msg, const PeerAddress& from);

    // Peer management
    void discover_peers();
    void maintain_connections();
    Peer* find_peer(const PeerAddress& addr);
};

/**
 * Protocol constants
 */
namespace protocol {
    static constexpr uint32_t PROTOCOL_VERSION = 1;
    static constexpr size_t MAX_MESSAGE_SIZE = 32 * 1024 * 1024;  // 32 MB
    static constexpr size_t MAX_PEERS = 125;
    static constexpr size_t MIN_PEERS = 8;
    static constexpr size_t MAX_OUTBOUND_CONNECTIONS = 8;
    static constexpr uint64_t TIMEOUT_SECONDS = 20;
    static constexpr uint64_t PING_INTERVAL_SECONDS = 120;
    static constexpr uint16_t DEFAULT_PORT = 8333;
    static constexpr uint16_t DEFAULT_PORT_TESTNET = 18333;
}

} // namespace p2p
} // namespace intcoin

#endif // INTCOIN_P2P_H
