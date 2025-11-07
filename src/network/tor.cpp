// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/tor.h"
#include "intcoin/crypto.h"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace intcoin {
namespace tor {

// OnionAddress implementation

bool OnionAddress::is_valid() const {
    if (type == AddressType::NONE) return false;
    if (port == 0) return false;

    if (!is_onion_address(address)) return false;

    // Check length based on type
    std::string addr = address;
    if (addr.size() > 6 && addr.substr(addr.size() - 6) == ".onion") {
        addr = addr.substr(0, addr.size() - 6);
    }

    if (type == AddressType::V2 && addr.length() == protocol::V2_ONION_LEN) {
        return true;
    } else if (type == AddressType::V3 && addr.length() == protocol::V3_ONION_LEN) {
        return true;
    }

    return false;
}

AddressType OnionAddress::detect_type(const std::string& addr) {
    if (!is_onion_address(addr)) return AddressType::NONE;

    std::string clean_addr = addr;
    if (clean_addr.size() > 6 && clean_addr.substr(clean_addr.size() - 6) == ".onion") {
        clean_addr = clean_addr.substr(0, clean_addr.size() - 6);
    }

    if (clean_addr.length() == protocol::V2_ONION_LEN) {
        return AddressType::V2;
    } else if (clean_addr.length() == protocol::V3_ONION_LEN) {
        return AddressType::V3;
    }

    return AddressType::NONE;
}

bool OnionAddress::is_onion_address(const std::string& addr) {
    if (addr.empty()) return false;

    // Must end with .onion
    if (addr.size() < 7) return false;

    std::string suffix = addr.substr(addr.size() - 6);
    if (suffix != ".onion") {
        // Check if it's without .onion suffix
        if (addr.length() != protocol::V2_ONION_LEN &&
            addr.length() != protocol::V3_ONION_LEN) {
            return false;
        }
    }

    // Check for valid base32 characters (a-z, 2-7)
    std::string check_addr = addr;
    if (suffix == ".onion") {
        check_addr = addr.substr(0, addr.size() - 6);
    }

    for (char c : check_addr) {
        if (!((c >= 'a' && c <= 'z') || (c >= '2' && c <= '7'))) {
            return false;
        }
    }

    return true;
}

// SOCKS5Proxy implementation

SOCKS5Proxy::SOCKS5Proxy(const SOCKS5Config& config)
    : config_(config)
{
}

SOCKS5Proxy::~SOCKS5Proxy() {
}

int SOCKS5Proxy::connect(const std::string& host, uint16_t port) {
    int socket_fd = create_proxy_socket();
    if (socket_fd < 0) {
        return -1;
    }

    // SOCKS5 handshake
    if (!socks5_handshake(socket_fd)) {
        close(socket_fd);
        return -1;
    }

    // Authenticate if required
    if (config_.use_auth) {
        if (!socks5_authenticate(socket_fd)) {
            close(socket_fd);
            return -1;
        }
    }

    // Send connect command
    if (!socks5_connect_command(socket_fd, host, port)) {
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

int SOCKS5Proxy::connect(const OnionAddress& onion_addr) {
    if (!onion_addr.is_valid()) {
        return -1;
    }
    return connect(onion_addr.address, onion_addr.port);
}

void SOCKS5Proxy::disconnect(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

bool SOCKS5Proxy::test_connection() {
    int socket_fd = create_proxy_socket();
    if (socket_fd < 0) {
        return false;
    }

    bool result = socks5_handshake(socket_fd);
    close(socket_fd);
    return result;
}

int SOCKS5Proxy::create_proxy_socket() {
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = config_.timeout_ms / 1000;
    timeout.tv_usec = (config_.timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // Connect to proxy
    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(config_.port);

    if (inet_pton(AF_INET, config_.host.c_str(), &proxy_addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }

    if (::connect(sock, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

bool SOCKS5Proxy::socks5_handshake(int socket_fd) {
    std::vector<uint8_t> request;
    request.push_back(protocol::SOCKS5_VERSION);  // Version 5

    if (config_.use_auth) {
        request.push_back(1);  // 1 auth method
        request.push_back(static_cast<uint8_t>(SOCKS5Auth::USERNAME_PASSWORD));
    } else {
        request.push_back(1);  // 1 auth method
        request.push_back(static_cast<uint8_t>(SOCKS5Auth::NO_AUTH));
    }

    if (!send_data(socket_fd, request)) {
        return false;
    }

    // Receive response
    auto response = receive_data(socket_fd, 2);
    if (response.size() != 2) {
        return false;
    }

    if (response[0] != protocol::SOCKS5_VERSION) {
        return false;
    }

    uint8_t selected_method = response[1];
    if (selected_method == static_cast<uint8_t>(SOCKS5Auth::NO_ACCEPTABLE)) {
        return false;
    }

    return true;
}

bool SOCKS5Proxy::socks5_authenticate(int socket_fd) {
    if (!config_.use_auth) {
        return true;
    }

    std::vector<uint8_t> auth_request;
    auth_request.push_back(0x01);  // Auth version

    // Username
    auth_request.push_back(static_cast<uint8_t>(config_.username.length()));
    auth_request.insert(auth_request.end(), config_.username.begin(), config_.username.end());

    // Password
    auth_request.push_back(static_cast<uint8_t>(config_.password.length()));
    auth_request.insert(auth_request.end(), config_.password.begin(), config_.password.end());

    if (!send_data(socket_fd, auth_request)) {
        return false;
    }

    // Receive response
    auto response = receive_data(socket_fd, 2);
    if (response.size() != 2) {
        return false;
    }

    return response[1] == 0x00;  // Success
}

bool SOCKS5Proxy::socks5_connect_command(int socket_fd, const std::string& host, uint16_t port) {
    std::vector<uint8_t> request;
    request.push_back(protocol::SOCKS5_VERSION);  // Version
    request.push_back(static_cast<uint8_t>(SOCKS5Command::CONNECT));  // Connect command
    request.push_back(0x00);  // Reserved

    // Address type - domain name
    request.push_back(static_cast<uint8_t>(SOCKS5AddressType::DOMAIN));

    // Domain name length and name
    request.push_back(static_cast<uint8_t>(host.length()));
    request.insert(request.end(), host.begin(), host.end());

    // Port (big-endian)
    request.push_back(static_cast<uint8_t>((port >> 8) & 0xFF));
    request.push_back(static_cast<uint8_t>(port & 0xFF));

    if (!send_data(socket_fd, request)) {
        return false;
    }

    // Receive response (at least 10 bytes)
    auto response = receive_data(socket_fd, 10);
    if (response.size() < 10) {
        return false;
    }

    if (response[0] != protocol::SOCKS5_VERSION) {
        return false;
    }

    if (response[1] != static_cast<uint8_t>(SOCKS5Reply::SUCCESS)) {
        return false;
    }

    return true;
}

bool SOCKS5Proxy::send_data(int socket_fd, const std::vector<uint8_t>& data) {
    size_t total_sent = 0;
    while (total_sent < data.size()) {
        ssize_t sent = send(socket_fd, data.data() + total_sent,
                           data.size() - total_sent, 0);
        if (sent < 0) {
            return false;
        }
        total_sent += sent;
    }
    return true;
}

std::vector<uint8_t> SOCKS5Proxy::receive_data(int socket_fd, size_t expected_size) {
    std::vector<uint8_t> buffer(expected_size);
    size_t total_received = 0;

    while (total_received < expected_size) {
        ssize_t received = recv(socket_fd, buffer.data() + total_received,
                               expected_size - total_received, 0);
        if (received <= 0) {
            buffer.resize(total_received);
            return buffer;
        }
        total_received += received;
    }

    return buffer;
}

// HiddenService implementation

HiddenService::HiddenService()
    : running_(false)
{
}

HiddenService::HiddenService(const HiddenServiceConfig& config)
    : config_(config)
    , running_(false)
{
}

bool HiddenService::initialize() {
    if (config_.data_dir.empty()) {
        return false;
    }

    // Create data directory if it doesn't exist
    mkdir(config_.data_dir.c_str(), 0700);

    // Set default paths if not specified
    if (config_.private_key_file.empty()) {
        config_.private_key_file = config_.data_dir + "/hs_ed25519_secret_key";
    }
    if (config_.hostname_file.empty()) {
        config_.hostname_file = config_.data_dir + "/hostname";
    }

    // Try to load existing keys, or generate new ones
    if (!load_keys()) {
        if (!generate_keys()) {
            return false;
        }
    }

    return true;
}

bool HiddenService::start() {
    if (running_) {
        return true;
    }

    if (!onion_address_) {
        if (!initialize()) {
            return false;
        }
    }

    running_ = true;
    return true;
}

void HiddenService::stop() {
    running_ = false;
}

std::optional<OnionAddress> HiddenService::get_onion_address() const {
    return onion_address_;
}

bool HiddenService::generate_keys() {
    if (!generate_ed25519_keypair()) {
        return false;
    }

    if (!derive_onion_address()) {
        return false;
    }

    if (!save_keys()) {
        return false;
    }

    return true;
}

bool HiddenService::load_keys() {
    // Check if key file exists
    std::ifstream key_file(config_.private_key_file, std::ios::binary);
    if (!key_file.good()) {
        return false;
    }

    // Check if hostname file exists
    std::ifstream hostname_file(config_.hostname_file);
    if (!hostname_file.good()) {
        return false;
    }

    // Read hostname
    std::string hostname;
    std::getline(hostname_file, hostname);
    hostname_file.close();

    if (!OnionAddress::is_onion_address(hostname)) {
        return false;
    }

    onion_address_ = OnionAddress(hostname, config_.virtual_port);
    return true;
}

bool HiddenService::generate_ed25519_keypair() {
    // In a real implementation, this would use proper ed25519 key generation
    // For now, we'll use a placeholder that generates random keys

    // Generate 32-byte private key (simplified)
    std::vector<uint8_t> private_key(32);
    for (size_t i = 0; i < 32; i++) {
        private_key[i] = static_cast<uint8_t>(rand() % 256);
    }

    return true;
}

bool HiddenService::derive_onion_address() {
    // In a real implementation, this would properly derive v3 onion address
    // from ed25519 public key using:
    // onion_address = base32(PUBKEY | CHECKSUM | VERSION) + ".onion"
    // where CHECKSUM = SHA3-256(".onion checksum" | PUBKEY | VERSION)[:2]

    // For now, generate a placeholder v3 address
    std::string onion_base32;
    const char base32_chars[] = "abcdefghijklmnopqrstuvwxyz234567";

    for (size_t i = 0; i < protocol::V3_ONION_LEN; i++) {
        onion_base32 += base32_chars[rand() % 32];
    }

    onion_address_ = OnionAddress(onion_base32 + ".onion", config_.virtual_port);
    return true;
}

bool HiddenService::save_keys() {
    // Save hostname
    std::ofstream hostname_file(config_.hostname_file);
    if (!hostname_file.good()) {
        return false;
    }

    if (onion_address_) {
        hostname_file << onion_address_->address << std::endl;
    }
    hostname_file.close();

    // In a real implementation, would also save the private key securely

    return true;
}

// TORController implementation

TORController::TORController()
    : control_socket_(-1)
    , connected_(false)
    , authenticated_(false)
{
}

TORController::~TORController() {
    disconnect();
}

bool TORController::connect(const std::string& host, uint16_t port) {
    if (connected_) {
        return true;
    }

    control_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket_ < 0) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        close(control_socket_);
        control_socket_ = -1;
        return false;
    }

    if (::connect(control_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(control_socket_);
        control_socket_ = -1;
        return false;
    }

    connected_ = true;
    return true;
}

void TORController::disconnect() {
    if (control_socket_ >= 0) {
        close(control_socket_);
        control_socket_ = -1;
    }
    connected_ = false;
    authenticated_ = false;
}

bool TORController::authenticate(const std::string& password) {
    if (!connected_) {
        return false;
    }

    std::string command = "AUTHENTICATE";
    if (!password.empty()) {
        // Convert password to hex
        std::stringstream hex_password;
        for (unsigned char c : password) {
            hex_password << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        command += " " + hex_password.str();
    }

    std::string response;
    if (!send_command(command, response)) {
        return false;
    }

    authenticated_ = (response.find("250 OK") == 0);
    return authenticated_;
}

bool TORController::authenticate_cookie(const std::string& cookie_path) {
    if (!connected_) {
        return false;
    }

    // Read cookie file
    std::ifstream cookie_file(cookie_path, std::ios::binary);
    if (!cookie_file.good()) {
        return false;
    }

    std::vector<uint8_t> cookie((std::istreambuf_iterator<char>(cookie_file)),
                                std::istreambuf_iterator<char>());
    cookie_file.close();

    // Convert to hex
    std::stringstream hex_cookie;
    for (uint8_t byte : cookie) {
        hex_cookie << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }

    std::string command = "AUTHENTICATE " + hex_cookie.str();
    std::string response;

    if (!send_command(command, response)) {
        return false;
    }

    authenticated_ = (response.find("250 OK") == 0);
    return authenticated_;
}

bool TORController::send_command(const std::string& command, std::string& response) {
    if (!connected_) {
        return false;
    }

    if (!send_line(command)) {
        return false;
    }

    response = receive_response();
    return !response.empty();
}

bool TORController::get_info(const std::string& keyword, std::string& value) {
    std::string response;
    if (!send_command("GETINFO " + keyword, response)) {
        return false;
    }

    return parse_response(response, value);
}

bool TORController::set_config(const std::string& key, const std::string& value) {
    std::string response;
    return send_command("SETCONF " + key + "=" + value, response);
}

bool TORController::new_circuit() {
    std::string response;
    return send_command("SIGNAL NEWNYM", response);
}

bool TORController::close_circuit(const std::string& circuit_id) {
    std::string response;
    return send_command("CLOSECIRCUIT " + circuit_id, response);
}

bool TORController::add_onion(const std::string& private_key, uint16_t port,
                              std::string& onion_address) {
    std::string command = "ADD_ONION " + private_key + " Port=" +
                         std::to_string(port) + "," + std::to_string(port);
    std::string response;

    if (!send_command(command, response)) {
        return false;
    }

    // Parse onion address from response
    size_t pos = response.find("250-ServiceID=");
    if (pos != std::string::npos) {
        size_t start = pos + 14;
        size_t end = response.find("\n", start);
        onion_address = response.substr(start, end - start) + ".onion";
        return true;
    }

    return false;
}

bool TORController::del_onion(const std::string& onion_address) {
    std::string addr = onion_address;
    // Remove .onion suffix if present
    if (addr.size() > 6 && addr.substr(addr.size() - 6) == ".onion") {
        addr = addr.substr(0, addr.size() - 6);
    }

    std::string response;
    return send_command("DEL_ONION " + addr, response);
}

bool TORController::send_line(const std::string& line) {
    std::string data = line + "\r\n";
    ssize_t sent = send(control_socket_, data.c_str(), data.length(), 0);
    return sent == static_cast<ssize_t>(data.length());
}

std::string TORController::receive_response() {
    std::string response;
    char buffer[1024];

    while (true) {
        ssize_t received = recv(control_socket_, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            break;
        }

        buffer[received] = '\0';
        response += buffer;

        // Check if we have a complete response (ends with "250 OK\r\n" or similar)
        if (response.find("\r\n") != std::string::npos) {
            // Check if it's a final response
            if (response.find("250 ") == 0 || response.find("250-") != std::string::npos) {
                if (response.find("250 OK") != std::string::npos) {
                    break;
                }
            }
        }
    }

    return response;
}

bool TORController::parse_response(const std::string& response, std::string& data) {
    // Simple parser - extract data after "250-" or "250 "
    size_t pos = response.find("250");
    if (pos == std::string::npos) {
        return false;
    }

    size_t data_start = response.find("=", pos);
    if (data_start == std::string::npos) {
        return false;
    }

    size_t data_end = response.find("\r\n", data_start);
    data = response.substr(data_start + 1, data_end - data_start - 1);
    return true;
}

// TORNetwork implementation

TORNetwork::TORNetwork()
    : onion_only_(false)
    , initialized_(false)
    , running_(false)
    , connections_through_tor_(0)
{
}

TORNetwork::~TORNetwork() {
    stop();
}

void TORNetwork::set_socks5_config(const SOCKS5Config& config) {
    socks5_config_ = config;
}

void TORNetwork::set_hidden_service_config(const HiddenServiceConfig& config) {
    hs_config_ = config;
}

bool TORNetwork::initialize() {
    if (initialized_) {
        return true;
    }

    // Check if TOR is available
    if (!check_tor_available()) {
        return false;
    }

    // Create SOCKS5 proxy
    proxy_ = std::make_unique<SOCKS5Proxy>(socks5_config_);

    // Test proxy connection
    if (!proxy_->test_connection()) {
        return false;
    }

    // Initialize hidden service if configured
    if (hs_config_.enabled && !hs_config_.data_dir.empty()) {
        hidden_service_ = std::make_unique<HiddenService>(hs_config_);
        if (!hidden_service_->initialize()) {
            // Hidden service initialization failed, but continue without it
            hidden_service_.reset();
        }
    }

    initialized_ = true;
    return true;
}

bool TORNetwork::start() {
    if (!initialized_) {
        if (!initialize()) {
            return false;
        }
    }

    if (running_) {
        return true;
    }

    // Start hidden service if available
    if (hidden_service_) {
        if (!hidden_service_->start()) {
            // Continue even if hidden service fails
        }
    }

    running_ = true;
    return true;
}

void TORNetwork::stop() {
    if (hidden_service_) {
        hidden_service_->stop();
    }

    running_ = false;
}

int TORNetwork::connect_through_tor(const std::string& host, uint16_t port) {
    if (!proxy_) {
        return -1;
    }

    int socket_fd = proxy_->connect(host, port);
    if (socket_fd >= 0) {
        connections_through_tor_++;
    }

    return socket_fd;
}

int TORNetwork::connect_to_onion(const OnionAddress& addr) {
    if (!proxy_) {
        return -1;
    }

    if (!addr.is_valid()) {
        return -1;
    }

    int socket_fd = proxy_->connect(addr);
    if (socket_fd >= 0) {
        connections_through_tor_++;
    }

    return socket_fd;
}

bool TORNetwork::start_hidden_service() {
    if (!hidden_service_) {
        return false;
    }

    return hidden_service_->start();
}

void TORNetwork::stop_hidden_service() {
    if (hidden_service_) {
        hidden_service_->stop();
    }
}

std::optional<OnionAddress> TORNetwork::get_our_onion_address() const {
    if (!hidden_service_) {
        return std::nullopt;
    }

    return hidden_service_->get_onion_address();
}

void TORNetwork::add_onion_peer(const OnionAddress& addr) {
    if (addr.is_valid()) {
        onion_peers_.push_back(addr);
    }
}

std::vector<OnionAddress> TORNetwork::get_onion_peers() const {
    return onion_peers_;
}

p2p::PeerAddress TORNetwork::onion_to_peer_address(const OnionAddress& onion) const {
    p2p::PeerAddress peer;
    peer.ip = onion.address;
    peer.port = onion.port;
    peer.services = 1;  // NODE_NETWORK
    return peer;
}

std::optional<OnionAddress> TORNetwork::peer_address_to_onion(const p2p::PeerAddress& addr) const {
    if (OnionAddress::is_onion_address(addr.ip)) {
        return OnionAddress(addr.ip, addr.port);
    }
    return std::nullopt;
}

bool TORNetwork::is_tor_available() const {
    return check_tor_available();
}

TORNetwork::TORStats TORNetwork::get_stats() const {
    TORStats stats;
    stats.onion_peers = onion_peers_.size();
    stats.clearnet_peers = 0;  // Would need integration with main network
    stats.connections_through_tor = connections_through_tor_;
    stats.hidden_service_active = is_hidden_service_running();

    if (auto addr = get_our_onion_address()) {
        stats.our_onion_address = addr->to_string();
    }

    return stats;
}

bool TORNetwork::check_tor_available() const {
    return util::is_tor_running(socks5_config_.host, socks5_config_.port);
}

bool TORNetwork::validate_onion_address(const std::string& addr) const {
    return OnionAddress::is_onion_address(addr);
}

// Utility functions

namespace util {

std::string generate_v3_onion_address(const std::vector<uint8_t>& pubkey) {
    if (pubkey.size() != 32) {
        return "";
    }

    // v3 onion address format:
    // onion_address = base32(PUBKEY | CHECKSUM | VERSION) + ".onion"
    // CHECKSUM = SHA3(".onion checksum" | PUBKEY | VERSION)[:2]
    // VERSION = 0x03

    std::vector<uint8_t> data;
    const char* checksum_prefix = ".onion checksum";
    data.insert(data.end(), checksum_prefix, checksum_prefix + strlen(checksum_prefix));
    data.insert(data.end(), pubkey.begin(), pubkey.end());
    data.push_back(0x03);  // Version

    // Calculate checksum
    auto hash = crypto::SHA3_256::hash(data.data(), data.size());

    // Build address data: PUBKEY | CHECKSUM[:2] | VERSION
    std::vector<uint8_t> addr_data;
    addr_data.insert(addr_data.end(), pubkey.begin(), pubkey.end());
    addr_data.push_back(hash[0]);
    addr_data.push_back(hash[1]);
    addr_data.push_back(0x03);

    // Base32 encode (simplified - would need proper base32 implementation)
    std::string base32_result;
    const char base32_chars[] = "abcdefghijklmnopqrstuvwxyz234567";

    // Simplified base32 encoding (placeholder)
    for (size_t i = 0; i < protocol::V3_ONION_LEN; i++) {
        base32_result += base32_chars[addr_data[i % addr_data.size()] % 32];
    }

    return base32_result + ".onion";
}

bool parse_onion_address(const std::string& addr, std::string& onion, uint16_t& port) {
    size_t colon_pos = addr.find(':');
    if (colon_pos == std::string::npos) {
        // No port specified
        onion = addr;
        port = 0;
        return OnionAddress::is_onion_address(onion);
    }

    onion = addr.substr(0, colon_pos);
    try {
        port = static_cast<uint16_t>(std::stoi(addr.substr(colon_pos + 1)));
    } catch (...) {
        return false;
    }

    return OnionAddress::is_onion_address(onion);
}

bool is_tor_running(const std::string& host, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    // Set short timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    bool result = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sock);

    return result;
}

std::string get_default_tor_datadir() {
    const char* home = getenv("HOME");
    if (!home) {
        return "";
    }

    return std::string(home) + "/.tor";
}

std::string generate_circuit_id() {
    std::stringstream ss;
    ss << std::hex << (rand() % 0xFFFFFFFF);
    return ss.str();
}

} // namespace util

} // namespace tor
} // namespace intcoin
