// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Mobile Wallet Core Implementation

#include "mobile_wallet.h"
#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include "intcoin/network.h"
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>
#include <mutex>

namespace intcoin {
namespace mobile {

// ========== PaymentRequest Implementation ==========

std::string PaymentRequest::to_uri() const {
    std::ostringstream uri;
    uri << "intcoin:" << address;

    bool first = true;
    auto add_param = [&](const std::string& key, const std::string& value) {
        uri << (first ? "?" : "&") << key << "=" << value;
        first = false;
    };

    if (amount.has_value()) {
        // Convert satoshis to INT with 8 decimal places
        double int_amount = static_cast<double>(*amount) / 100000000.0;
        std::ostringstream amount_str;
        amount_str << std::fixed << std::setprecision(8) << int_amount;
        add_param("amount", amount_str.str());
    }

    if (label.has_value() && !label->empty()) {
        // URL encode label
        std::string encoded;
        for (char c : *label) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else {
                std::ostringstream oss;
                oss << '%' << std::hex << std::uppercase << std::setw(2)
                    << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                encoded += oss.str();
            }
        }
        add_param("label", encoded);
    }

    if (message.has_value() && !message->empty()) {
        // URL encode message
        std::string encoded;
        for (char c : *message) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else {
                std::ostringstream oss;
                oss << '%' << std::hex << std::uppercase << std::setw(2)
                    << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                encoded += oss.str();
            }
        }
        add_param("message", encoded);
    }

    return uri.str();
}

std::optional<PaymentRequest> PaymentRequest::from_uri(const std::string& uri) {
    // Parse intcoin:address?params format
    std::regex uri_regex(R"(^intcoin:([a-zA-Z0-9]+)(\?.*)?$)");
    std::smatch match;

    if (!std::regex_match(uri, match, uri_regex)) {
        // Also try plain address
        if (utils::is_valid_address(uri)) {
            PaymentRequest req;
            req.address = uri;
            return req;
        }
        return std::nullopt;
    }

    PaymentRequest request;
    request.address = match[1].str();

    if (match[2].matched) {
        std::string params = match[2].str().substr(1);  // Remove '?'

        // Parse parameters
        std::regex param_regex(R"(([^&=]+)=([^&]*))");
        auto params_begin = std::sregex_iterator(params.begin(), params.end(), param_regex);
        auto params_end = std::sregex_iterator();

        for (auto it = params_begin; it != params_end; ++it) {
            std::string key = (*it)[1].str();
            std::string value = (*it)[2].str();

            // URL decode value
            std::string decoded;
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '%' && i + 2 < value.size()) {
                    int hex_val;
                    std::istringstream iss(value.substr(i + 1, 2));
                    if (iss >> std::hex >> hex_val) {
                        decoded += static_cast<char>(hex_val);
                        i += 2;
                        continue;
                    }
                }
                decoded += value[i];
            }

            if (key == "amount") {
                try {
                    double amount_double = std::stod(decoded);
                    request.amount = static_cast<uint64_t>(amount_double * 100000000.0);
                } catch (...) {}
            } else if (key == "label") {
                request.label = decoded;
            } else if (key == "message") {
                request.message = decoded;
            }
        }
    }

    return request;
}

// ========== QRCodeGenerator Implementation ==========

std::vector<uint8_t> QRCodeGenerator::generate_png(const std::string& data,
                                                    const QRConfig& config) {
    // QR code generation implementation
    // In production, use libqrencode or similar library
    std::vector<uint8_t> png_data;

    // Placeholder - actual implementation would generate QR code
    // This would use libqrencode to create QR matrix, then encode as PNG

    return png_data;
}

std::string QRCodeGenerator::generate_svg(const std::string& data,
                                          const QRConfig& config) {
    // Generate SVG QR code
    std::ostringstream svg;

    // Placeholder SVG structure
    svg << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
    svg << R"(<svg xmlns="http://www.w3.org/2000/svg" )";
    svg << R"(width=")" << config.size << R"(" height=")" << config.size << R"(">)" << "\n";
    svg << R"(<rect width="100%" height="100%" fill="#ffffff"/>)" << "\n";
    // QR modules would be rendered here
    svg << "</svg>";

    return svg.str();
}

std::vector<uint8_t> QRCodeGenerator::generate_payment_qr(const PaymentRequest& request,
                                                          const QRConfig& config) {
    return generate_png(request.to_uri(), config);
}

// ========== MobileWallet Implementation ==========

class MobileWallet::Impl {
public:
    std::unique_ptr<SecureKeyStore> key_store;
    std::unique_ptr<Wallet> wallet;
    std::mutex wallet_mutex;

    bool is_locked = true;
    bool biometric_enabled = false;
    std::string wallet_id = "default";

    // Callbacks
    TransactionCallback tx_callback;
    BalanceCallback balance_callback;
    SyncCallback sync_callback;

    // Settings
    std::string display_currency = "USD";
    std::string denomination = "INT";
    bool push_notifications = true;

    // Network
    bool connected = false;
    double sync_progress = 0.0;

    Impl() : key_store(SecureKeyStore::create()) {}
};

MobileWallet::MobileWallet() : impl_(std::make_unique<Impl>()) {}

MobileWallet::~MobileWallet() = default;

bool MobileWallet::create_wallet(const std::string& password) {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    // Generate new wallet with mnemonic
    impl_->wallet = std::make_unique<Wallet>();

    if (!impl_->wallet->create(password)) {
        return false;
    }

    // Store encrypted seed in secure storage
    auto encrypted_seed = impl_->wallet->get_encrypted_seed();
    if (!impl_->key_store->store_seed(encrypted_seed, impl_->wallet_id)) {
        return false;
    }

    impl_->is_locked = false;
    return true;
}

bool MobileWallet::import_wallet(const std::vector<std::string>& mnemonic,
                                  const std::string& password) {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    impl_->wallet = std::make_unique<Wallet>();

    if (!impl_->wallet->restore_from_mnemonic(mnemonic, password)) {
        return false;
    }

    auto encrypted_seed = impl_->wallet->get_encrypted_seed();
    if (!impl_->key_store->store_seed(encrypted_seed, impl_->wallet_id)) {
        return false;
    }

    impl_->is_locked = false;
    return true;
}

std::optional<std::vector<std::string>> MobileWallet::export_mnemonic(
    const std::string& password) {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (impl_->is_locked || !impl_->wallet) {
        return std::nullopt;
    }

    if (!impl_->wallet->verify_password(password)) {
        return std::nullopt;
    }

    return impl_->wallet->get_mnemonic();
}

bool MobileWallet::wallet_exists() const {
    auto seed = impl_->key_store->retrieve_seed(impl_->wallet_id);
    return seed.has_value();
}

void MobileWallet::lock() {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);
    impl_->is_locked = true;
    if (impl_->wallet) {
        impl_->wallet->lock();
    }
}

bool MobileWallet::unlock(const std::string& password) {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    auto encrypted_seed = impl_->key_store->retrieve_seed(impl_->wallet_id);
    if (!encrypted_seed) {
        return false;
    }

    if (!impl_->wallet) {
        impl_->wallet = std::make_unique<Wallet>();
    }

    if (!impl_->wallet->unlock(*encrypted_seed, password)) {
        return false;
    }

    impl_->is_locked = false;
    return true;
}

bool MobileWallet::is_locked() const {
    return impl_->is_locked;
}

bool MobileWallet::delete_wallet(const std::string& password) {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (!impl_->wallet || !impl_->wallet->verify_password(password)) {
        return false;
    }

    impl_->key_store->delete_seed(impl_->wallet_id);
    impl_->wallet.reset();
    impl_->is_locked = true;
    return true;
}

bool MobileWallet::enable_biometric(const std::string& password) {
    if (impl_->key_store->available_biometric() == BiometricType::NONE) {
        return false;
    }

    if (!impl_->wallet || !impl_->wallet->verify_password(password)) {
        return false;
    }

    impl_->biometric_enabled = true;
    return true;
}

bool MobileWallet::disable_biometric() {
    impl_->biometric_enabled = false;
    return true;
}

bool MobileWallet::is_biometric_enabled() const {
    return impl_->biometric_enabled;
}

void MobileWallet::unlock_biometric(std::function<void(bool success)> callback) {
    if (!impl_->biometric_enabled) {
        callback(false);
        return;
    }

    impl_->key_store->authenticate_biometric(
        "Unlock INTcoin Wallet",
        [this, callback](bool success, const std::string& error) {
            if (success) {
                impl_->is_locked = false;
            }
            callback(success);
        });
}

std::string MobileWallet::get_receive_address() {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (impl_->is_locked || !impl_->wallet) {
        return "";
    }

    return impl_->wallet->get_receive_address();
}

std::string MobileWallet::generate_new_address(const std::string& label) {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (impl_->is_locked || !impl_->wallet) {
        return "";
    }

    auto address = impl_->wallet->generate_new_address();
    if (!label.empty()) {
        impl_->wallet->set_address_label(address, label);
    }
    return address;
}

std::vector<AddressInfo> MobileWallet::get_addresses() const {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    std::vector<AddressInfo> result;
    if (impl_->is_locked || !impl_->wallet) {
        return result;
    }

    auto addresses = impl_->wallet->get_addresses();
    for (const auto& addr : addresses) {
        AddressInfo info;
        info.address = addr.address;
        info.label = addr.label;
        info.balance = addr.balance;
        info.pending_balance = addr.pending_balance;
        info.is_change = addr.is_change;
        result.push_back(info);
    }
    return result;
}

bool MobileWallet::set_address_label(const std::string& address,
                                      const std::string& label) {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (impl_->is_locked || !impl_->wallet) {
        return false;
    }

    return impl_->wallet->set_address_label(address, label);
}

uint64_t MobileWallet::get_balance() const {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (impl_->is_locked || !impl_->wallet) {
        return 0;
    }

    return impl_->wallet->get_balance();
}

uint64_t MobileWallet::get_pending_balance() const {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (impl_->is_locked || !impl_->wallet) {
        return 0;
    }

    return impl_->wallet->get_pending_balance();
}

std::vector<MobileTransaction> MobileWallet::get_transactions(
    size_t offset, size_t limit) const {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    std::vector<MobileTransaction> result;
    if (impl_->is_locked || !impl_->wallet) {
        return result;
    }

    auto txs = impl_->wallet->get_transactions(offset, limit);
    for (const auto& tx : txs) {
        MobileTransaction mtx;
        mtx.tx_hash = tx.hash;
        mtx.from_address = tx.from_address;
        mtx.to_address = tx.to_address;
        mtx.amount = tx.amount;
        mtx.fee = tx.fee;
        mtx.timestamp = tx.timestamp;
        mtx.confirmations = tx.confirmations;
        mtx.is_incoming = tx.is_incoming;
        mtx.memo = tx.memo;
        result.push_back(mtx);
    }
    return result;
}

uint64_t MobileWallet::estimate_fee(const std::string& to_address,
                                     uint64_t amount) const {
    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    if (impl_->is_locked || !impl_->wallet) {
        return 0;
    }

    return impl_->wallet->estimate_fee(to_address, amount);
}

MobileWallet::SendResult MobileWallet::send_transaction(
    const std::string& to_address,
    uint64_t amount,
    const std::string& password) {

    std::lock_guard<std::mutex> lock(impl_->wallet_mutex);

    SendResult result;
    result.success = false;

    if (impl_->is_locked || !impl_->wallet) {
        result.error = "Wallet is locked";
        return result;
    }

    if (!impl_->wallet->verify_password(password)) {
        result.error = "Invalid password";
        return result;
    }

    auto tx_result = impl_->wallet->send(to_address, amount);
    result.success = tx_result.success;
    result.tx_hash = tx_result.tx_hash;
    result.error = tx_result.error;

    return result;
}

std::vector<uint8_t> MobileWallet::generate_receive_qr(
    std::optional<uint64_t> amount,
    const std::string& label,
    const std::string& message) {

    PaymentRequest request;
    request.address = get_receive_address();
    request.amount = amount;
    if (!label.empty()) request.label = label;
    if (!message.empty()) request.message = message;

    return QRCodeGenerator::generate_payment_qr(request);
}

std::optional<PaymentRequest> MobileWallet::parse_qr_data(const std::string& data) {
    return PaymentRequest::from_uri(data);
}

bool MobileWallet::connect(const std::string& node_url) {
    impl_->connected = true;  // Simplified
    return true;
}

void MobileWallet::disconnect() {
    impl_->connected = false;
}

bool MobileWallet::is_connected() const {
    return impl_->connected;
}

double MobileWallet::get_sync_progress() const {
    return impl_->sync_progress;
}

void MobileWallet::sync() {
    // Trigger sync
}

void MobileWallet::on_transaction(TransactionCallback callback) {
    impl_->tx_callback = callback;
}

void MobileWallet::on_balance_change(BalanceCallback callback) {
    impl_->balance_callback = callback;
}

void MobileWallet::on_sync_progress(SyncCallback callback) {
    impl_->sync_callback = callback;
}

std::string MobileWallet::get_display_currency() const {
    return impl_->display_currency;
}

void MobileWallet::set_display_currency(const std::string& currency) {
    impl_->display_currency = currency;
}

std::string MobileWallet::get_denomination() const {
    return impl_->denomination;
}

void MobileWallet::set_denomination(const std::string& denomination) {
    impl_->denomination = denomination;
}

void MobileWallet::set_push_notifications(bool enabled) {
    impl_->push_notifications = enabled;
}

bool MobileWallet::get_push_notifications() const {
    return impl_->push_notifications;
}

// ========== Utilities ==========

namespace utils {

std::string format_amount(uint64_t satoshis, const std::string& denomination) {
    std::ostringstream oss;

    if (denomination == "INT") {
        double amount = static_cast<double>(satoshis) / 100000000.0;
        oss << std::fixed << std::setprecision(8) << amount << " INT";
    } else if (denomination == "mINT") {
        double amount = static_cast<double>(satoshis) / 100000.0;
        oss << std::fixed << std::setprecision(5) << amount << " mINT";
    } else if (denomination == "uINT") {
        double amount = static_cast<double>(satoshis) / 100.0;
        oss << std::fixed << std::setprecision(2) << amount << " uINT";
    } else {
        oss << satoshis << " sat";
    }

    return oss.str();
}

std::optional<uint64_t> parse_amount(const std::string& input,
                                     const std::string& denomination) {
    try {
        double value = std::stod(input);
        if (value < 0) return std::nullopt;

        uint64_t satoshis;
        if (denomination == "INT") {
            satoshis = static_cast<uint64_t>(value * 100000000.0);
        } else if (denomination == "mINT") {
            satoshis = static_cast<uint64_t>(value * 100000.0);
        } else if (denomination == "uINT") {
            satoshis = static_cast<uint64_t>(value * 100.0);
        } else {
            satoshis = static_cast<uint64_t>(value);
        }
        return satoshis;
    } catch (...) {
        return std::nullopt;
    }
}

bool is_valid_address(const std::string& address) {
    // INT addresses start with 'i' and are 34-42 characters
    if (address.empty() || address[0] != 'i') {
        return false;
    }
    if (address.length() < 34 || address.length() > 42) {
        return false;
    }
    // Check Base58 characters
    for (char c : address) {
        if (!isalnum(c)) return false;
        if (c == '0' || c == 'O' || c == 'I' || c == 'l') return false;
    }
    return true;
}

double to_fiat(uint64_t satoshis, const std::string& currency) {
    // Would fetch exchange rate from API
    // Placeholder implementation
    double int_amount = static_cast<double>(satoshis) / 100000000.0;
    return int_amount * 0.001;  // Placeholder rate
}

std::string format_timestamp(uint64_t timestamp, bool relative) {
    auto now = std::chrono::system_clock::now();
    auto tp = std::chrono::system_clock::from_time_t(static_cast<time_t>(timestamp));

    if (relative) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - tp).count();

        if (diff < 60) return "just now";
        if (diff < 3600) return std::to_string(diff / 60) + " min ago";
        if (diff < 86400) return std::to_string(diff / 3600) + " hr ago";
        if (diff < 604800) return std::to_string(diff / 86400) + " days ago";
    }

    auto time = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

} // namespace utils

} // namespace mobile
} // namespace intcoin
