// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_HARDWARE_WALLET_H
#define INTCOIN_QT_HARDWARE_WALLET_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace intcoin {
namespace qt {

/**
 * Hardware wallet device type
 */
enum class HWDeviceType {
    UNKNOWN,
    LEDGER_NANO_S,
    LEDGER_NANO_X,
    TREZOR_MODEL_T,
    TREZOR_ONE,
    COLDCARD_MK4
};

/**
 * Hardware wallet device info
 */
struct HWDeviceInfo {
    std::string device_id;
    HWDeviceType type{HWDeviceType::UNKNOWN};
    std::string model_name;
    std::string firmware_version;
    bool is_initialized{false};
    bool is_connected{false};
};

/**
 * HD derivation path (BIP44)
 */
struct DerivationPath {
    uint32_t purpose{44};            // BIP44 = 44, BIP49 = 49, BIP84 = 84
    uint32_t coin_type{0};           // Bitcoin = 0, Testnet = 1
    uint32_t account{0};
    uint32_t change{0};              // 0 = external, 1 = internal
    uint32_t address_index{0};

    std::string ToString() const {
        return "m/" + std::to_string(purpose) + "'/" +
               std::to_string(coin_type) + "'/" +
               std::to_string(account) + "'/" +
               std::to_string(change) + "/" +
               std::to_string(address_index);
    }
};

/**
 * Hardware wallet transaction signing request
 */
struct HWSigningRequest {
    std::vector<uint8_t> raw_transaction;
    std::vector<DerivationPath> input_paths;
    std::string change_address;
    bool verify_on_device{true};     // Show on device screen for verification
};

/**
 * Hardware wallet signing result
 */
struct HWSigningResult {
    bool success{false};
    std::vector<uint8_t> signed_transaction;
    std::string error_message;
    bool user_confirmed{false};
};

/**
 * Hardware Wallet Interface
 *
 * Unified interface for hardware wallet devices (Ledger, Trezor, Coldcard).
 * Supports device enumeration, connection, address derivation, and
 * transaction signing with on-device verification.
 */
class HardwareWalletInterface {
public:
    /**
     * Device discovery callback
     */
    using DeviceCallback = std::function<void(const HWDeviceInfo&)>;

    HardwareWalletInterface();
    ~HardwareWalletInterface();

    /**
     * Enumerate connected hardware wallet devices
     *
     * @return Vector of detected devices
     */
    std::vector<HWDeviceInfo> EnumerateDevices();

    /**
     * Connect to hardware wallet device
     *
     * @param device_id Device ID from enumeration
     * @return True if connected successfully
     */
    bool ConnectDevice(const std::string& device_id);

    /**
     * Disconnect from current device
     */
    void DisconnectDevice();

    /**
     * Check if device is connected
     */
    bool IsConnected() const;

    /**
     * Get current device info
     */
    HWDeviceInfo GetDeviceInfo() const;

    /**
     * Initialize device (first-time setup)
     *
     * @param device_name Name for the device
     * @return True if initialized successfully
     */
    bool InitializeDevice(const std::string& device_name);

    /**
     * Get address from device
     *
     * @param path Derivation path
     * @param verify_on_device Show on device screen
     * @return Address string
     */
    std::string GetAddress(const DerivationPath& path, bool verify_on_device = false);

    /**
     * Get multiple addresses
     *
     * @param start_index Starting address index
     * @param count Number of addresses to derive
     * @param account Account number
     * @param change Change address (0=external, 1=internal)
     * @return Vector of addresses
     */
    std::vector<std::string> GetAddresses(
        uint32_t start_index,
        uint32_t count,
        uint32_t account = 0,
        uint32_t change = 0
    );

    /**
     * Sign transaction
     *
     * @param request Signing request
     * @return Signing result
     */
    HWSigningResult SignTransaction(const HWSigningRequest& request);

    /**
     * Verify address on device screen
     *
     * @param address Address to verify
     * @param path Derivation path
     * @return True if user confirmed
     */
    bool VerifyAddressOnDevice(const std::string& address, const DerivationPath& path);

    /**
     * Get device public key
     *
     * @param path Derivation path
     * @return Public key bytes
     */
    std::vector<uint8_t> GetPublicKey(const DerivationPath& path);

    /**
     * Get extended public key (xpub)
     *
     * @param path Derivation path
     * @return Extended public key string
     */
    std::string GetExtendedPublicKey(const DerivationPath& path);

    /**
     * Wipe device (factory reset)
     *
     * WARNING: This will erase all data on the device!
     *
     * @return True if wiped successfully
     */
    bool WipeDevice();

    /**
     * Register device discovery callback
     *
     * @param callback Callback function
     */
    void SetDeviceCallback(DeviceCallback callback);

    /**
     * Start device monitoring (auto-connect on detection)
     */
    void StartMonitoring();

    /**
     * Stop device monitoring
     */
    void StopMonitoring();

    /**
     * Get supported device types
     */
    static std::vector<HWDeviceType> GetSupportedDevices();

    /**
     * Get device type name
     */
    static std::string GetDeviceTypeName(HWDeviceType type);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_HARDWARE_WALLET_H
