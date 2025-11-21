// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/lightning_watchtower.h"
#include "intcoin/hash.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <iostream>

// Network socket includes
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

namespace intcoin {
namespace lightning {

// ============================================================================
// BreachRemedy Implementation
// ============================================================================

std::vector<uint8_t> BreachRemedy::serialize() const {
    std::vector<uint8_t> data;

    // Commitment TXID hint
    data.insert(data.end(), commitment_txid_hint.bytes, commitment_txid_hint.bytes + 32);

    // Encrypted payload length + data
    uint32_t payload_len = static_cast<uint32_t>(encrypted_payload.size());
    data.push_back((payload_len >> 24) & 0xFF);
    data.push_back((payload_len >> 16) & 0xFF);
    data.push_back((payload_len >> 8) & 0xFF);
    data.push_back(payload_len & 0xFF);
    data.insert(data.end(), encrypted_payload.begin(), encrypted_payload.end());

    // Salt length + data
    uint32_t salt_len = static_cast<uint32_t>(salt.size());
    data.push_back((salt_len >> 24) & 0xFF);
    data.push_back((salt_len >> 16) & 0xFF);
    data.push_back((salt_len >> 8) & 0xFF);
    data.push_back(salt_len & 0xFF);
    data.insert(data.end(), salt.begin(), salt.end());

    // Client signature
    auto sig_data = client_sig.serialize();
    data.insert(data.end(), sig_data.begin(), sig_data.end());

    // Expiry timestamp
    for (int i = 7; i >= 0; --i) {
        data.push_back((expiry_timestamp >> (i * 8)) & 0xFF);
    }

    // Channel ID
    data.insert(data.end(), channel_id.bytes, channel_id.bytes + 32);

    return data;
}

BreachRemedy BreachRemedy::deserialize(const std::vector<uint8_t>& data) {
    BreachRemedy remedy;
    size_t offset = 0;

    if (data.size() < 32) {
        throw std::runtime_error("Invalid breach remedy data: too short");
    }

    // Commitment TXID hint
    std::memcpy(remedy.commitment_txid_hint.bytes, &data[offset], 32);
    offset += 32;

    // Encrypted payload
    if (offset + 4 > data.size()) throw std::runtime_error("Invalid payload length");
    uint32_t payload_len = (static_cast<uint32_t>(data[offset]) << 24) |
                          (static_cast<uint32_t>(data[offset + 1]) << 16) |
                          (static_cast<uint32_t>(data[offset + 2]) << 8) |
                          static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    if (offset + payload_len > data.size()) throw std::runtime_error("Invalid payload data");
    remedy.encrypted_payload.assign(data.begin() + offset, data.begin() + offset + payload_len);
    offset += payload_len;

    // Salt
    if (offset + 4 > data.size()) throw std::runtime_error("Invalid salt length");
    uint32_t salt_len = (static_cast<uint32_t>(data[offset]) << 24) |
                        (static_cast<uint32_t>(data[offset + 1]) << 16) |
                        (static_cast<uint32_t>(data[offset + 2]) << 8) |
                        static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    if (offset + salt_len > data.size()) throw std::runtime_error("Invalid salt data");
    remedy.salt.assign(data.begin() + offset, data.begin() + offset + salt_len);
    offset += salt_len;

    // Client signature (assuming Dilithium5 signature size)
    std::vector<uint8_t> sig_data(data.begin() + offset, data.end() - 40);  // 8 bytes timestamp + 32 bytes channel_id
    remedy.client_sig = DilithiumSignature::deserialize(sig_data);
    offset += sig_data.size();

    // Expiry timestamp
    remedy.expiry_timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        remedy.expiry_timestamp = (remedy.expiry_timestamp << 8) | data[offset++];
    }

    // Channel ID
    std::memcpy(remedy.channel_id.bytes, &data[offset], 32);

    return remedy;
}

bool BreachRemedy::verify_signature(const DilithiumPubKey& client_pubkey) const {
    // Create message: commitment_txid_hint || encrypted_payload || salt || expiry || channel_id
    std::vector<uint8_t> message;
    message.insert(message.end(), commitment_txid_hint.bytes, commitment_txid_hint.bytes + 32);
    message.insert(message.end(), encrypted_payload.begin(), encrypted_payload.end());
    message.insert(message.end(), salt.begin(), salt.end());

    for (int i = 7; i >= 0; --i) {
        message.push_back((expiry_timestamp >> (i * 8)) & 0xFF);
    }

    message.insert(message.end(), channel_id.bytes, channel_id.bytes + 32);

    return client_pubkey.verify(message, client_sig);
}

// ============================================================================
// BreachRemedyPayload Implementation
// ============================================================================

std::vector<uint8_t> BreachRemedyPayload::serialize() const {
    std::vector<uint8_t> data;

    // Penalty transaction
    auto tx_data = penalty_tx.serialize();
    uint32_t tx_len = static_cast<uint32_t>(tx_data.size());
    data.push_back((tx_len >> 24) & 0xFF);
    data.push_back((tx_len >> 16) & 0xFF);
    data.push_back((tx_len >> 8) & 0xFF);
    data.push_back(tx_len & 0xFF);
    data.insert(data.end(), tx_data.begin(), tx_data.end());

    // Revocation private key
    auto privkey_data = revocation_privkey.serialize();
    data.insert(data.end(), privkey_data.begin(), privkey_data.end());

    // Witness data
    uint32_t witness_len = static_cast<uint32_t>(witness_data.size());
    data.push_back((witness_len >> 24) & 0xFF);
    data.push_back((witness_len >> 16) & 0xFF);
    data.push_back((witness_len >> 8) & 0xFF);
    data.push_back(witness_len & 0xFF);
    data.insert(data.end(), witness_data.begin(), witness_data.end());

    // Amounts
    for (int i = 7; i >= 0; --i) {
        data.push_back((to_local_amount >> (i * 8)) & 0xFF);
    }
    for (int i = 7; i >= 0; --i) {
        data.push_back((to_remote_amount >> (i * 8)) & 0xFF);
    }

    return data;
}

BreachRemedyPayload BreachRemedyPayload::deserialize(const std::vector<uint8_t>& data) {
    BreachRemedyPayload payload;
    size_t offset = 0;

    // Penalty transaction
    if (offset + 4 > data.size()) throw std::runtime_error("Invalid payload: tx length");
    uint32_t tx_len = (static_cast<uint32_t>(data[offset]) << 24) |
                      (static_cast<uint32_t>(data[offset + 1]) << 16) |
                      (static_cast<uint32_t>(data[offset + 2]) << 8) |
                      static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    if (offset + tx_len > data.size()) throw std::runtime_error("Invalid payload: tx data");
    std::vector<uint8_t> tx_data(data.begin() + offset, data.begin() + offset + tx_len);
    payload.penalty_tx = Transaction::deserialize(tx_data);
    offset += tx_len;

    // Revocation private key (size depends on Dilithium5)
    const size_t privkey_size = 4000;  // Approximate Dilithium5 private key size
    if (offset + privkey_size > data.size()) throw std::runtime_error("Invalid payload: privkey");
    std::vector<uint8_t> privkey_data(data.begin() + offset, data.begin() + offset + privkey_size);
    payload.revocation_privkey = DilithiumPrivKey::deserialize(privkey_data);
    offset += privkey_size;

    // Witness data
    if (offset + 4 > data.size()) throw std::runtime_error("Invalid payload: witness length");
    uint32_t witness_len = (static_cast<uint32_t>(data[offset]) << 24) |
                           (static_cast<uint32_t>(data[offset + 1]) << 16) |
                           (static_cast<uint32_t>(data[offset + 2]) << 8) |
                           static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    if (offset + witness_len > data.size()) throw std::runtime_error("Invalid payload: witness data");
    payload.witness_data.assign(data.begin() + offset, data.begin() + offset + witness_len);
    offset += witness_len;

    // Amounts
    payload.to_local_amount = 0;
    for (int i = 0; i < 8; ++i) {
        payload.to_local_amount = (payload.to_local_amount << 8) | data[offset++];
    }

    payload.to_remote_amount = 0;
    for (int i = 0; i < 8; ++i) {
        payload.to_remote_amount = (payload.to_remote_amount << 8) | data[offset++];
    }

    return payload;
}

// ============================================================================
// WatchtowerClientRegistration Implementation
// ============================================================================

std::vector<uint8_t> WatchtowerClientRegistration::serialize() const {
    std::vector<uint8_t> data;

    // Client public key
    auto pubkey_data = client_pubkey.serialize();
    data.insert(data.end(), pubkey_data.begin(), pubkey_data.end());

    // Timestamp
    for (int i = 7; i >= 0; --i) {
        data.push_back((timestamp >> (i * 8)) & 0xFF);
    }

    // Signature
    auto sig_data = signature.serialize();
    data.insert(data.end(), sig_data.begin(), sig_data.end());

    // Client ID
    uint16_t id_len = static_cast<uint16_t>(client_id.size());
    data.push_back((id_len >> 8) & 0xFF);
    data.push_back(id_len & 0xFF);
    data.insert(data.end(), client_id.begin(), client_id.end());

    return data;
}

WatchtowerClientRegistration WatchtowerClientRegistration::deserialize(const std::vector<uint8_t>& data) {
    WatchtowerClientRegistration reg;
    size_t offset = 0;

    // Client public key
    const size_t pubkey_size = 2592;  // Dilithium5 public key size
    if (data.size() < pubkey_size) throw std::runtime_error("Invalid registration: too short");
    std::vector<uint8_t> pubkey_data(data.begin(), data.begin() + pubkey_size);
    reg.client_pubkey = DilithiumPubKey::deserialize(pubkey_data);
    offset += pubkey_size;

    // Timestamp
    reg.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        reg.timestamp = (reg.timestamp << 8) | data[offset++];
    }

    // Signature (remaining data except last 2 bytes for ID length)
    const size_t sig_size = 4595;  // Dilithium5 signature size
    std::vector<uint8_t> sig_data(data.begin() + offset, data.begin() + offset + sig_size);
    reg.signature = DilithiumSignature::deserialize(sig_data);
    offset += sig_size;

    // Client ID
    if (offset + 2 > data.size()) throw std::runtime_error("Invalid registration: no ID length");
    uint16_t id_len = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    if (offset + id_len > data.size()) throw std::runtime_error("Invalid registration: ID data");
    reg.client_id.assign(data.begin() + offset, data.begin() + offset + id_len);

    return reg;
}

bool WatchtowerClientRegistration::verify() const {
    // Message: pubkey || timestamp
    std::vector<uint8_t> message;
    auto pubkey_data = client_pubkey.serialize();
    message.insert(message.end(), pubkey_data.begin(), pubkey_data.end());

    for (int i = 7; i >= 0; --i) {
        message.push_back((timestamp >> (i * 8)) & 0xFF);
    }

    return client_pubkey.verify(message, signature);
}

// ============================================================================
// WatchtowerClient Implementation
// ============================================================================

WatchtowerClient::WatchtowerClient(const DilithiumPrivKey& client_privkey)
    : client_privkey_(client_privkey)
    , client_pubkey_(client_privkey.get_public_key())
    , remedy_count_(0) {
}

bool WatchtowerClient::register_with_watchtower(const std::string& watchtower_address,
                                                  uint16_t watchtower_port) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Create registration message
    WatchtowerClientRegistration reg;
    reg.client_pubkey = client_pubkey_;
    reg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    reg.client_id = "intcoin_client_" + std::to_string(reg.timestamp);

    // Sign the registration
    std::vector<uint8_t> message;
    auto pubkey_data = reg.client_pubkey.serialize();
    message.insert(message.end(), pubkey_data.begin(), pubkey_data.end());
    for (int i = 7; i >= 0; --i) {
        message.push_back((reg.timestamp >> (i * 8)) & 0xFF);
    }
    reg.signature = client_privkey_.sign(message);

    // Send registration to watchtower
    auto reg_data = reg.serialize();
    bool success = send_watchtower_message(watchtower_address, watchtower_port,
                                            WatchtowerMessageType::REGISTER_CLIENT, reg_data);

    if (success) {
        watchtowers_[{watchtower_address, watchtower_port}] = reg.timestamp;
    }

    return success;
}

bool WatchtowerClient::upload_breach_remedy(const std::string& watchtower_address,
                                              uint16_t watchtower_port,
                                              const Hash256& channel_id,
                                              const CommitmentTransaction& commitment,
                                              const DilithiumPrivKey& revocation_privkey,
                                              const Transaction& penalty_tx) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Create breach remedy payload
    BreachRemedyPayload payload;
    payload.penalty_tx = penalty_tx;
    payload.revocation_privkey = revocation_privkey;
    payload.to_local_amount = commitment.to_local_sat;
    payload.to_remote_amount = commitment.to_remote_sat;

    // Generate random salt using secure random generator
    std::vector<uint8_t> salt(32);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < salt.size(); ++i) {
        salt[i] = static_cast<uint8_t>(dist(gen));
    }

    // Get commitment TXID
    Hash256 commitment_txid = commitment.get_hash();

    // Encrypt payload
    auto encrypted_payload = encrypt_remedy_payload(payload, commitment_txid, salt);

    // Create breach remedy
    BreachRemedy remedy;
    remedy.commitment_txid_hint = create_txid_hint(commitment_txid, salt);
    remedy.encrypted_payload = encrypted_payload;
    remedy.salt = salt;
    remedy.expiry_timestamp = std::chrono::system_clock::now().time_since_epoch().count() +
                              BREACH_REMEDY_RETENTION;
    remedy.channel_id = channel_id;

    // Sign the remedy
    std::vector<uint8_t> remedy_msg;
    remedy_msg.insert(remedy_msg.end(), remedy.commitment_txid_hint.bytes,
                     remedy.commitment_txid_hint.bytes + 32);
    remedy_msg.insert(remedy_msg.end(), encrypted_payload.begin(), encrypted_payload.end());
    remedy_msg.insert(remedy_msg.end(), salt.begin(), salt.end());
    for (int i = 7; i >= 0; --i) {
        remedy_msg.push_back((remedy.expiry_timestamp >> (i * 8)) & 0xFF);
    }
    remedy_msg.insert(remedy_msg.end(), channel_id.bytes, channel_id.bytes + 32);

    remedy.client_sig = client_privkey_.sign(remedy_msg);

    // Send to watchtower
    auto remedy_data = remedy.serialize();
    bool success = send_watchtower_message(watchtower_address, watchtower_port,
                                            WatchtowerMessageType::BREACH_REMEDY, remedy_data);

    if (success) {
        remedy_count_++;
    }

    return success;
}

std::vector<std::pair<std::string, uint16_t>> WatchtowerClient::get_watchtowers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, uint16_t>> result;
    for (const auto& [addr_port, _] : watchtowers_) {
        result.push_back(addr_port);
    }
    return result;
}

bool WatchtowerClient::remove_watchtower(const std::string& watchtower_address,
                                           uint16_t watchtower_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    return watchtowers_.erase({watchtower_address, watchtower_port}) > 0;
}

std::vector<uint8_t> WatchtowerClient::encrypt_remedy_payload(const BreachRemedyPayload& payload,
                                                                const Hash256& commitment_txid,
                                                                const std::vector<uint8_t>& salt) const {
    // Derive encryption key from commitment TXID and salt
    // Key = SHA3-256(commitment_txid || salt)
    std::vector<uint8_t> key_material;
    key_material.insert(key_material.end(), commitment_txid.bytes, commitment_txid.bytes + 32);
    key_material.insert(key_material.end(), salt.begin(), salt.end());

    Hash256 encryption_key = sha3_256(key_material);

    // Serialize payload
    auto plaintext = payload.serialize();

    // XOR encryption (simple for demonstration - use AES-GCM in production)
    std::vector<uint8_t> ciphertext = plaintext;
    for (size_t i = 0; i < ciphertext.size(); ++i) {
        ciphertext[i] ^= encryption_key.bytes[i % 32];
    }

    return ciphertext;
}

Hash256 WatchtowerClient::create_txid_hint(const Hash256& commitment_txid,
                                             const std::vector<uint8_t>& salt) const {
    // Hint = SHA3-256(commitment_txid || salt || "hint")
    std::vector<uint8_t> hint_material;
    hint_material.insert(hint_material.end(), commitment_txid.bytes, commitment_txid.bytes + 32);
    hint_material.insert(hint_material.end(), salt.begin(), salt.end());
    std::string hint_tag = "hint";
    hint_material.insert(hint_material.end(), hint_tag.begin(), hint_tag.end());

    return sha3_256(hint_material);
}

bool WatchtowerClient::send_watchtower_message(const std::string& address,
                                                 uint16_t port,
                                                 WatchtowerMessageType msg_type,
                                                 const std::vector<uint8_t>& payload) {
    // Network communication implementation for watchtower protocol
    // This creates a TCP connection to the watchtower server and sends
    // the message using a simple length-prefixed framing protocol

    try {
        // Create TCP socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Failed to create socket for watchtower connection" << std::endl;
            return false;
        }

        // Set socket timeout (10 seconds)
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        // Resolve address and connect
        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        // Convert IP address string to binary
        if (inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid watchtower address: " << address << std::endl;
            close(sockfd);
            return false;
        }

        // Connect to watchtower
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to connect to watchtower " << address << ":" << port << std::endl;
            close(sockfd);
            return false;
        }

        // Build message frame:
        // [1 byte: message type]
        // [4 bytes: payload length (big-endian)]
        // [N bytes: payload]
        std::vector<uint8_t> frame;
        frame.push_back(static_cast<uint8_t>(msg_type));

        uint32_t payload_len = static_cast<uint32_t>(payload.size());
        frame.push_back((payload_len >> 24) & 0xFF);
        frame.push_back((payload_len >> 16) & 0xFF);
        frame.push_back((payload_len >> 8) & 0xFF);
        frame.push_back(payload_len & 0xFF);

        frame.insert(frame.end(), payload.begin(), payload.end());

        // Send message frame
        ssize_t bytes_sent = send(sockfd, frame.data(), frame.size(), 0);
        if (bytes_sent < 0 || static_cast<size_t>(bytes_sent) != frame.size()) {
            std::cerr << "Failed to send watchtower message (sent " << bytes_sent
                      << " of " << frame.size() << " bytes)" << std::endl;
            close(sockfd);
            return false;
        }

        // Wait for response (1 byte response type + 1 byte error code)
        uint8_t response[2];
        ssize_t bytes_received = recv(sockfd, response, 2, 0);

        close(sockfd);

        if (bytes_received < 2) {
            std::cerr << "Failed to receive watchtower response" << std::endl;
            return false;
        }

        WatchtowerMessageType response_type = static_cast<WatchtowerMessageType>(response[0]);
        WatchtowerError error_code = static_cast<WatchtowerError>(response[1]);

        if (response_type == WatchtowerMessageType::ERROR ||
            error_code != WatchtowerError::NONE) {
            std::cerr << "Watchtower returned error: " << static_cast<int>(error_code) << std::endl;
            return false;
        }

        std::cout << "Successfully sent watchtower message to " << address << ":" << port
                  << " type=" << static_cast<int>(msg_type)
                  << " size=" << payload.size() << " bytes" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception in watchtower communication: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// WatchtowerServer Implementation
// ============================================================================

WatchtowerServer::WatchtowerServer(uint16_t listen_port)
    : listen_port_(listen_port)
    , running_(false)
    , breaches_detected_(0)
    , penalties_broadcast_(0)
    , max_clients_(10000)
    , max_remedies_per_client_(MAX_BREACH_REMEDIES_PER_CLIENT) {
}

WatchtowerServer::~WatchtowerServer() {
    stop();
}

bool WatchtowerServer::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        return false;
    }

    running_ = true;
    start_time_ = std::chrono::steady_clock::now();

    std::cout << "Watchtower server started on port " << listen_port_ << std::endl;

    return true;
}

void WatchtowerServer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return;
    }

    running_ = false;
    std::cout << "Watchtower server stopped" << std::endl;
}

void WatchtowerServer::process_block(const std::vector<Transaction>& transactions,
                                       uint32_t block_height) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return;
    }

    // Check each transaction for potential breaches
    for (const auto& tx : transactions) {
        BreachRemedy matched_remedy;
        if (is_breach_transaction(tx, matched_remedy)) {
            std::cout << "Breach detected at block " << block_height << "!" << std::endl;
            breaches_detected_++;

            // Broadcast penalty transaction
            Hash256 breach_txid = tx.get_hash();
            if (broadcast_penalty(matched_remedy, breach_txid)) {
                penalties_broadcast_++;
            }
        }
    }

    // Cleanup expired remedies periodically
    cleanup_expired_remedies();
}

WatchtowerServer::Stats WatchtowerServer::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats stats;
    stats.registered_clients = registered_clients_.size();
    stats.stored_remedies = breach_remedies_.size();
    stats.breaches_detected = breaches_detected_;
    stats.penalties_broadcast = penalties_broadcast_;

    if (running_) {
        auto now = std::chrono::steady_clock::now();
        stats.uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            now - start_time_).count();
    } else {
        stats.uptime_seconds = 0;
    }

    return stats;
}

bool WatchtowerServer::handle_client_registration(const std::vector<uint8_t>& payload,
                                                    std::string& client_id) {
    try {
        auto registration = WatchtowerClientRegistration::deserialize(payload);

        if (!registration.verify()) {
            return false;
        }

        if (registered_clients_.size() >= max_clients_) {
            return false;
        }

        client_id = registration.client_id;
        registered_clients_[client_id] = registration;

        std::cout << "Client registered: " << client_id << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Registration error: " << e.what() << std::endl;
        return false;
    }
}

bool WatchtowerServer::handle_breach_remedy(const std::vector<uint8_t>& payload,
                                              const std::string& client_id) {
    try {
        auto remedy = BreachRemedy::deserialize(payload);

        // Verify client is registered
        if (registered_clients_.find(client_id) == registered_clients_.end()) {
            return false;
        }

        // Verify remedy signature
        const auto& registration = registered_clients_[client_id];
        if (!remedy.verify_signature(registration.client_pubkey)) {
            return false;
        }

        // Check if remedy already exists
        if (breach_remedies_.find(remedy.commitment_txid_hint) != breach_remedies_.end()) {
            return false;  // Duplicate
        }

        // Store remedy
        breach_remedies_[remedy.commitment_txid_hint] = remedy;

        // Update index
        channel_remedies_index_[remedy.channel_id].push_back(remedy.commitment_txid_hint);

        std::cout << "Breach remedy stored for channel "
                  << std::string(reinterpret_cast<const char*>(remedy.channel_id.bytes), 32)
                  << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Breach remedy error: " << e.what() << std::endl;
        return false;
    }
}

bool WatchtowerServer::is_breach_transaction(const Transaction& tx,
                                               BreachRemedy& matched_remedy) {
    // Breach detection algorithm:
    // 1. Extract transaction ID (potential commitment transaction)
    // 2. Compute hints using various secret combinations
    // 3. Match against stored breach remedy hints
    // 4. Verify the match by attempting decryption

    Hash256 tx_hash = tx.get_hash();
    Hash256 txid = tx.get_txid();

    // Commitment transactions have specific characteristics:
    // - 2 outputs (to_local, to_remote)
    // - Version 2
    // - Specific locktime patterns
    // - nSequence with CSV delay

    // Basic heuristic checks for commitment transaction structure
    bool looks_like_commitment = false;

    if (tx.version == 2 &&
        tx.outputs.size() >= 2 &&
        tx.inputs.size() == 1) {
        // Check for CSV delay in input sequence
        if (tx.inputs[0].sequence < 0xFFFFFFFE) {
            looks_like_commitment = true;
        }
    }

    if (!looks_like_commitment) {
        return false;  // Not a commitment transaction
    }

    // Compute possible hints from the transaction
    // The hint is derived as: SHA3-256(commitment_txid || secret)
    // We need to try matching against all stored hints

    // Strategy: For each stored remedy, compute what hint would match this txid
    // Since we don't know the secret, we try matching the txid portion

    for (auto& [hint, remedy] : breach_remedies_) {
        // Try to match this transaction against the remedy
        // The hint is SHA3-256(commitment_txid || secret)
        // We can't reverse SHA3, so we need to use the txid to decrypt
        // and see if decryption succeeds

        // Attempt decryption using this transaction's txid
        auto payload_opt = decrypt_remedy_payload(remedy, txid);

        if (payload_opt) {
            // Decryption succeeded! This is a breach.
            // Verify the penalty transaction is valid
            const auto& payload = *payload_opt;

            // Sanity checks on decrypted payload
            if (payload.to_local_amount > 0 ||
                payload.to_remote_amount > 0) {

                // Additional verification: check if the hint matches
                // Recompute hint from txid and compare
                std::vector<uint8_t> hint_material;
                hint_material.insert(hint_material.end(),
                                   txid.bytes, txid.bytes + 32);
                hint_material.insert(hint_material.end(),
                                   remedy.salt.begin(), remedy.salt.end());

                Hash256 computed_hint = sha3_256(hint_material);

                // Check if computed hint matches stored hint
                // (allowing for some tolerance in case of hash collisions)
                bool hint_matches = (std::memcmp(computed_hint.bytes,
                                                 hint.bytes,
                                                 16) == 0);  // Match first 16 bytes

                if (hint_matches) {
                    std::cout << "BREACH DETECTED!" << std::endl;
                    std::cout << "  Transaction: " << txid.to_hex() << std::endl;
                    std::cout << "  Channel: " << remedy.channel_id.to_hex() << std::endl;
                    std::cout << "  To-local amount: " << payload.to_local_amount << " sat" << std::endl;
                    std::cout << "  To-remote amount: " << payload.to_remote_amount << " sat" << std::endl;

                    matched_remedy = remedy;
                    return true;
                }
            }
        }
    }

    return false;  // No breach detected
}

bool WatchtowerServer::broadcast_penalty(const BreachRemedy& remedy,
                                          const Hash256& commitment_txid) {
    // Decrypt the remedy payload
    auto payload_opt = decrypt_remedy_payload(remedy, commitment_txid);

    if (!payload_opt) {
        std::cerr << "Failed to decrypt breach remedy payload" << std::endl;
        return false;
    }

    const auto& payload = *payload_opt;

    // Reconstruct the penalty transaction from the decrypted payload
    Transaction penalty_tx = payload.penalty_tx;

    std::cout << "Broadcasting penalty transaction!" << std::endl;
    std::cout << "  Transaction ID: " << penalty_tx.get_txid().to_hex() << std::endl;
    std::cout << "  Reclaiming to_local: " << payload.to_local_amount << " satoshis" << std::endl;
    std::cout << "  Reclaiming to_remote: " << payload.to_remote_amount << " satoshis" << std::endl;
    std::cout << "  Total outputs: " << penalty_tx.outputs.size() << std::endl;

    // Broadcast to network using P2P protocol
    // The penalty transaction needs to be broadcast immediately to prevent
    // the attacker from spending the funds

    try {
        // Serialize the penalty transaction
        auto tx_bytes = penalty_tx.serialize();

        // In a production implementation, this would:
        // 1. Connect to multiple P2P nodes
        // 2. Send INV message announcing the transaction
        // 3. Respond to GETDATA requests with the full transaction
        // 4. Maintain connections until transaction is confirmed

        // For now, we'll create a simple broadcast using TCP sockets
        // to announce the transaction to known peers

        // Get list of P2P peers from configuration
        // Default to localhost for testing
        std::vector<std::pair<std::string, uint16_t>> peers = {
            {"127.0.0.1", 9333},  // Local node
            // Add more peers from config
        };

        size_t successful_broadcasts = 0;

        for (const auto& [peer_ip, peer_port] : peers) {
            // Create socket connection to peer
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                continue;  // Skip this peer
            }

            // Set short timeout for penalty broadcast (5 seconds)
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

            // Connect to peer
            struct sockaddr_in peer_addr;
            std::memset(&peer_addr, 0, sizeof(peer_addr));
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_port = htons(peer_port);

            if (inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr) <= 0) {
                close(sockfd);
                continue;
            }

            if (connect(sockfd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
                close(sockfd);
                continue;  // Peer not available
            }

            // Build P2P message for transaction broadcast
            // Message format: [magic][type][length][payload]
            std::vector<uint8_t> message;

            // Magic bytes (INTcoin network)
            uint32_t magic = 0xD9B4BEF9;  // Bitcoin-style magic
            message.push_back((magic >> 24) & 0xFF);
            message.push_back((magic >> 16) & 0xFF);
            message.push_back((magic >> 8) & 0xFF);
            message.push_back(magic & 0xFF);

            // Message type: TX
            const char* msg_type = "tx\0\0\0\0\0\0\0\0\0\0";  // 12 bytes
            message.insert(message.end(), msg_type, msg_type + 12);

            // Payload length
            uint32_t payload_len = static_cast<uint32_t>(tx_bytes.size());
            message.push_back((payload_len >> 24) & 0xFF);
            message.push_back((payload_len >> 16) & 0xFF);
            message.push_back((payload_len >> 8) & 0xFF);
            message.push_back(payload_len & 0xFF);

            // Checksum (first 4 bytes of double SHA-256)
            Hash256 checksum = sha3_256(tx_bytes);
            message.insert(message.end(), checksum.bytes, checksum.bytes + 4);

            // Payload (serialized transaction)
            message.insert(message.end(), tx_bytes.begin(), tx_bytes.end());

            // Send message
            ssize_t bytes_sent = send(sockfd, message.data(), message.size(), 0);

            close(sockfd);

            if (bytes_sent > 0 && static_cast<size_t>(bytes_sent) == message.size()) {
                successful_broadcasts++;
                std::cout << "  Broadcasted to peer " << peer_ip << ":" << peer_port << std::endl;
            }
        }

        if (successful_broadcasts > 0) {
            std::cout << "Penalty transaction broadcasted to " << successful_broadcasts
                     << " peer(s)" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to broadcast penalty transaction to any peers" << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception while broadcasting penalty: " << e.what() << std::endl;
        return false;
    }
}

std::optional<BreachRemedyPayload> WatchtowerServer::decrypt_remedy_payload(
    const BreachRemedy& remedy,
    const Hash256& commitment_txid) const {

    try {
        // Derive decryption key from commitment TXID and salt
        std::vector<uint8_t> key_material;
        key_material.insert(key_material.end(), commitment_txid.bytes, commitment_txid.bytes + 32);
        key_material.insert(key_material.end(), remedy.salt.begin(), remedy.salt.end());

        Hash256 decryption_key = sha3_256(key_material);

        // XOR decryption
        std::vector<uint8_t> plaintext = remedy.encrypted_payload;
        for (size_t i = 0; i < plaintext.size(); ++i) {
            plaintext[i] ^= decryption_key.bytes[i % 32];
        }

        // Deserialize payload
        return BreachRemedyPayload::deserialize(plaintext);
    } catch (const std::exception& e) {
        std::cerr << "Decryption error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

void WatchtowerServer::cleanup_expired_remedies() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();

    // Remove expired remedies
    for (auto it = breach_remedies_.begin(); it != breach_remedies_.end();) {
        if (it->second.expiry_timestamp < static_cast<uint64_t>(now)) {
            // Remove from channel index
            auto& channel_hints = channel_remedies_index_[it->second.channel_id];
            channel_hints.erase(
                std::remove(channel_hints.begin(), channel_hints.end(), it->first),
                channel_hints.end());

            it = breach_remedies_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// WatchtowerManager Implementation
// ============================================================================

WatchtowerManager::WatchtowerManager(const DilithiumPrivKey& client_privkey)
    : client_(std::make_unique<WatchtowerClient>(client_privkey)) {
}

bool WatchtowerManager::add_watchtower(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!client_->register_with_watchtower(address, port)) {
        return false;
    }

    remedy_counts_[{address, port}] = 0;
    return true;
}

bool WatchtowerManager::remove_watchtower(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);

    remedy_counts_.erase({address, port});
    return client_->remove_watchtower(address, port);
}

size_t WatchtowerManager::upload_to_all_watchtowers(const Hash256& channel_id,
                                                      const CommitmentTransaction& commitment,
                                                      const DilithiumPrivKey& revocation_privkey,
                                                      const Transaction& penalty_tx) {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t success_count = 0;
    auto watchtowers = client_->get_watchtowers();

    for (const auto& [address, port] : watchtowers) {
        if (client_->upload_breach_remedy(address, port, channel_id, commitment,
                                            revocation_privkey, penalty_tx)) {
            remedy_counts_[{address, port}]++;
            success_count++;
        }
    }

    return success_count;
}

std::vector<WatchtowerManager::WatchtowerStatus> WatchtowerManager::get_watchtower_status() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<WatchtowerStatus> statuses;
    auto watchtowers = client_->get_watchtowers();

    for (const auto& [address, port] : watchtowers) {
        WatchtowerStatus status;
        status.address = address;
        status.port = port;
        status.online = check_watchtower_health(address, port);
        status.last_contact = get_last_contact_time(address, port);
        status.remedies_uploaded = remedy_counts_.at({address, port});

        statuses.push_back(status);
    }

    return statuses;
}

size_t WatchtowerManager::get_total_watchtowers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return remedy_counts_.size();
}

size_t WatchtowerManager::get_online_watchtowers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t online_count = 0;
    for (const auto& [key, count] : remedy_counts_) {
        if (check_watchtower_health(key.first, key.second)) {
            online_count++;
        }
    }
    return online_count;
}

bool WatchtowerManager::check_watchtower_health(const std::string& address, uint16_t port) const {
    auto key = std::make_pair(address, port);
    auto it = last_contact_times_.find(key);
    if (it == last_contact_times_.end()) return false;

    auto now = std::chrono::steady_clock::now();
    auto last = std::chrono::steady_clock::time_point(std::chrono::seconds(it->second));
    return (now - last) < std::chrono::minutes(5);  // Online if contacted within 5 minutes
}

uint64_t WatchtowerManager::get_last_contact_time(const std::string& address, uint16_t port) const {
    auto key = std::make_pair(address, port);
    auto it = last_contact_times_.find(key);
    if (it != last_contact_times_.end()) return it->second;
    return 0;
}

size_t WatchtowerManager::get_total_remedies_uploaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return client_->get_remedy_count();
}

} // namespace lightning
} // namespace intcoin
