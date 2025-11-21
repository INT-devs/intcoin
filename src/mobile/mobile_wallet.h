// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Mobile Wallet Core - Cross-platform mobile wallet backend
// Supports iOS (Swift) and Android (Kotlin) native bindings

#ifndef INTCOIN_MOBILE_WALLET_H
#define INTCOIN_MOBILE_WALLET_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <optional>
#include <functional>

namespace intcoin {
namespace mobile {

// Forward declarations
class SecureKeyStore;
class QRCodeGenerator;
class QRCodeScanner;

/**
 * Transaction info for mobile display
 */
struct MobileTransaction {
    std::string tx_hash;
    std::string from_address;
    std::string to_address;
    uint64_t amount;           // In satoshis
    uint64_t fee;
    uint64_t timestamp;
    int32_t confirmations;
    bool is_incoming;
    std::string memo;
};

/**
 * Address info with balance
 */
struct AddressInfo {
    std::string address;
    std::string label;
    uint64_t balance;
    uint64_t pending_balance;
    bool is_change;
};

/**
 * QR Code payment request
 */
struct PaymentRequest {
    std::string address;
    std::optional<uint64_t> amount;
    std::optional<std::string> label;
    std::optional<std::string> message;

    // Generate INT URI: intcoin:address?amount=X&label=Y&message=Z
    std::string to_uri() const;

    // Parse INT URI
    static std::optional<PaymentRequest> from_uri(const std::string& uri);
};

/**
 * Biometric authentication types
 */
enum class BiometricType {
    NONE,
    FINGERPRINT,      // Touch ID / Fingerprint
    FACE,             // Face ID / Face Unlock
    IRIS,             // Iris scan (Samsung)
    PIN,              // Fallback PIN
    PATTERN           // Android pattern
};

/**
 * Secure key storage abstraction
 * Platform-specific implementations:
 * - iOS: Keychain Services with Secure Enclave
 * - Android: Android Keystore with StrongBox
 */
class SecureKeyStore {
public:
    virtual ~SecureKeyStore() = default;

    // Store encrypted seed phrase
    virtual bool store_seed(const std::vector<uint8_t>& encrypted_seed,
                           const std::string& wallet_id) = 0;

    // Retrieve encrypted seed phrase
    virtual std::optional<std::vector<uint8_t>> retrieve_seed(
        const std::string& wallet_id) = 0;

    // Delete seed phrase
    virtual bool delete_seed(const std::string& wallet_id) = 0;

    // Check if biometric authentication is available
    virtual BiometricType available_biometric() const = 0;

    // Authenticate with biometrics
    virtual bool authenticate_biometric(
        const std::string& reason,
        std::function<void(bool success, const std::string& error)> callback) = 0;

    // Check if hardware security module is available
    virtual bool has_hardware_security() const = 0;

    // Platform-specific factory
    static std::unique_ptr<SecureKeyStore> create();
};

/**
 * QR Code generation for payment requests
 */
class QRCodeGenerator {
public:
    struct QRConfig {
        int size = 256;              // Pixels
        int margin = 4;              // Quiet zone modules
        int error_correction = 2;    // 0=L, 1=M, 2=Q, 3=H
        uint32_t foreground = 0xFF000000;  // ARGB
        uint32_t background = 0xFFFFFFFF;  // ARGB
    };

    // Generate QR code as PNG data
    static std::vector<uint8_t> generate_png(const std::string& data,
                                              const QRConfig& config = {});

    // Generate QR code as SVG string
    static std::string generate_svg(const std::string& data,
                                    const QRConfig& config = {});

    // Generate payment request QR
    static std::vector<uint8_t> generate_payment_qr(const PaymentRequest& request,
                                                     const QRConfig& config = {});
};

/**
 * QR Code scanner interface
 * Implemented by platform-specific camera code
 */
class QRCodeScanner {
public:
    using ScanCallback = std::function<void(const std::string& data)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    virtual ~QRCodeScanner() = default;

    // Start scanning
    virtual void start_scanning(ScanCallback on_scan, ErrorCallback on_error) = 0;

    // Stop scanning
    virtual void stop_scanning() = 0;

    // Check camera permission
    virtual bool has_camera_permission() const = 0;

    // Request camera permission
    virtual void request_camera_permission(
        std::function<void(bool granted)> callback) = 0;

    // Platform-specific factory
    static std::unique_ptr<QRCodeScanner> create();
};

/**
 * Mobile Wallet Core
 * Cross-platform wallet functionality for mobile apps
 */
class MobileWallet {
public:
    MobileWallet();
    ~MobileWallet();

    // ========== Wallet Management ==========

    // Create new wallet with optional password
    bool create_wallet(const std::string& password = "");

    // Import wallet from mnemonic
    bool import_wallet(const std::vector<std::string>& mnemonic,
                       const std::string& password = "");

    // Export mnemonic (requires authentication)
    std::optional<std::vector<std::string>> export_mnemonic(
        const std::string& password);

    // Check if wallet exists
    bool wallet_exists() const;

    // Lock wallet
    void lock();

    // Unlock wallet
    bool unlock(const std::string& password);

    // Check if wallet is locked
    bool is_locked() const;

    // Delete wallet (requires password)
    bool delete_wallet(const std::string& password);

    // ========== Biometric Authentication ==========

    // Enable biometric authentication
    bool enable_biometric(const std::string& password);

    // Disable biometric authentication
    bool disable_biometric();

    // Check if biometric is enabled
    bool is_biometric_enabled() const;

    // Unlock with biometrics
    void unlock_biometric(std::function<void(bool success)> callback);

    // ========== Address Management ==========

    // Get current receive address
    std::string get_receive_address();

    // Generate new receive address
    std::string generate_new_address(const std::string& label = "");

    // Get all addresses with balances
    std::vector<AddressInfo> get_addresses() const;

    // Set address label
    bool set_address_label(const std::string& address, const std::string& label);

    // ========== Balance & Transactions ==========

    // Get total balance (confirmed)
    uint64_t get_balance() const;

    // Get pending balance (unconfirmed)
    uint64_t get_pending_balance() const;

    // Get transaction history
    std::vector<MobileTransaction> get_transactions(
        size_t offset = 0, size_t limit = 50) const;

    // Get transaction by hash
    std::optional<MobileTransaction> get_transaction(
        const std::string& tx_hash) const;

    // ========== Sending ==========

    // Estimate fee for transaction
    uint64_t estimate_fee(const std::string& to_address, uint64_t amount) const;

    // Create unsigned transaction
    struct UnsignedTx {
        std::string tx_hex;
        uint64_t fee;
        std::vector<std::string> signing_addresses;
    };
    std::optional<UnsignedTx> create_transaction(
        const std::string& to_address,
        uint64_t amount,
        uint64_t fee_rate = 0);  // 0 = auto

    // Sign and broadcast transaction
    struct SendResult {
        bool success;
        std::string tx_hash;
        std::string error;
    };
    SendResult send_transaction(const std::string& to_address,
                                uint64_t amount,
                                const std::string& password);

    // ========== QR Code Support ==========

    // Generate payment request QR
    std::vector<uint8_t> generate_receive_qr(
        std::optional<uint64_t> amount = std::nullopt,
        const std::string& label = "",
        const std::string& message = "");

    // Parse scanned QR code
    std::optional<PaymentRequest> parse_qr_data(const std::string& data);

    // ========== Network ==========

    // Connect to network
    bool connect(const std::string& node_url = "");

    // Disconnect from network
    void disconnect();

    // Check connection status
    bool is_connected() const;

    // Get sync progress (0.0 - 1.0)
    double get_sync_progress() const;

    // Force sync
    void sync();

    // ========== Notifications ==========

    using TransactionCallback = std::function<void(const MobileTransaction&)>;
    using BalanceCallback = std::function<void(uint64_t confirmed, uint64_t pending)>;
    using SyncCallback = std::function<void(double progress)>;

    // Set callback for incoming transactions
    void on_transaction(TransactionCallback callback);

    // Set callback for balance changes
    void on_balance_change(BalanceCallback callback);

    // Set callback for sync progress
    void on_sync_progress(SyncCallback callback);

    // ========== Settings ==========

    // Get/set preferred currency for display
    std::string get_display_currency() const;
    void set_display_currency(const std::string& currency);

    // Get/set preferred denomination (INT, mINT, uINT, satoshi)
    std::string get_denomination() const;
    void set_denomination(const std::string& denomination);

    // Enable/disable push notifications
    void set_push_notifications(bool enabled);
    bool get_push_notifications() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * Mobile wallet utilities
 */
namespace utils {
    // Format amount for display
    std::string format_amount(uint64_t satoshis, const std::string& denomination);

    // Parse amount from user input
    std::optional<uint64_t> parse_amount(const std::string& input,
                                         const std::string& denomination);

    // Validate address format
    bool is_valid_address(const std::string& address);

    // Get fiat value (requires exchange rate)
    double to_fiat(uint64_t satoshis, const std::string& currency);

    // Format timestamp for display
    std::string format_timestamp(uint64_t timestamp, bool relative = false);
}

} // namespace mobile
} // namespace intcoin

#endif // INTCOIN_MOBILE_WALLET_H
