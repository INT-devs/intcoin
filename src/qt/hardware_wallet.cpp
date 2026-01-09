// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/qt/hardware_wallet.h>
#include <hidapi/hidapi.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace qt {

// USB Vendor IDs
constexpr uint16_t VID_LEDGER = 0x2c97;
constexpr uint16_t VID_TREZOR = 0x534c;
constexpr uint16_t VID_COLDCARD = 0xd13e;

// USB Product IDs
constexpr uint16_t PID_LEDGER_NANO_S = 0x0001;
constexpr uint16_t PID_LEDGER_NANO_X = 0x0004;
constexpr uint16_t PID_TREZOR_ONE = 0x0001;
constexpr uint16_t PID_TREZOR_MODEL_T = 0x0002;
constexpr uint16_t PID_COLDCARD_MK4 = 0xcc10;

// APDU Constants (for Ledger)
constexpr uint8_t CLA_BITCOIN = 0xE0;
constexpr uint8_t INS_GET_VERSION = 0x01;
constexpr uint8_t INS_GET_ADDRESS = 0x40;
[[maybe_unused]] constexpr uint8_t INS_SIGN_TX = 0x44; // Reserved for future transaction signing
constexpr uint8_t INS_GET_PUBKEY = 0x42;

/**
 * USB HID Device Handle
 */
struct HIDDevice {
    hid_device* handle{nullptr};
    uint16_t vendor_id{0};
    uint16_t product_id{0};
    std::string path;
    std::string serial_number;
    HWDeviceType type{HWDeviceType::UNKNOWN};

    ~HIDDevice() {
        if (handle) {
            hid_close(handle);
            handle = nullptr;
        }
    }
};

/**
 * APDU Command (for Ledger)
 */
struct APDU {
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    std::vector<uint8_t> data;

    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> result;
        result.push_back(cla);
        result.push_back(ins);
        result.push_back(p1);
        result.push_back(p2);
        result.push_back(static_cast<uint8_t>(data.size()));
        result.insert(result.end(), data.begin(), data.end());
        return result;
    }
};

/**
 * APDU Response
 */
struct APDUResponse {
    std::vector<uint8_t> data;
    uint16_t status_word{0};

    bool IsSuccess() const {
        return status_word == 0x9000;
    }

    std::string GetError() const {
        switch (status_word) {
            case 0x9000: return "Success";
            case 0x6985: return "User denied";
            case 0x6a80: return "Invalid data";
            case 0x6d00: return "Instruction not supported";
            case 0x6e00: return "Class not supported";
            default: return "Unknown error: 0x" +
                std::to_string(status_word);
        }
    }
};

/**
 * Implementation class (Pimpl)
 */
class HardwareWalletInterface::Impl {
public:
    Impl() {
        hid_init();
    }

    ~Impl() {
        StopMonitoring();
        DisconnectDevice();
        hid_exit();
    }

    std::vector<HWDeviceInfo> EnumerateDevices() {
        std::vector<HWDeviceInfo> devices;

        // Enumerate HID devices
        hid_device_info* devs = hid_enumerate(0, 0);
        hid_device_info* cur_dev = devs;

        while (cur_dev) {
            HWDeviceType type = IdentifyDevice(cur_dev->vendor_id, cur_dev->product_id);

            if (type != HWDeviceType::UNKNOWN) {
                HWDeviceInfo info;
                info.device_id = cur_dev->path;
                info.type = type;
                info.model_name = HardwareWalletInterface::GetDeviceTypeName(type);

                if (cur_dev->serial_number) {
                    // Convert wide string to regular string
                    std::wstring ws(cur_dev->serial_number);
                    info.firmware_version = std::string(ws.begin(), ws.end());
                }

                info.is_connected = false;
                info.is_initialized = true; // Assume initialized if detected

                devices.push_back(info);
            }

            cur_dev = cur_dev->next;
        }

        hid_free_enumeration(devs);
        return devices;
    }

    bool ConnectDevice(const std::string& device_id) {
        std::lock_guard<std::mutex> lock(device_mutex_);

        // Disconnect existing device
        if (current_device_) {
            DisconnectDevice();
        }

        // Try to open device by path
        hid_device* handle = hid_open_path(device_id.c_str());
        if (!handle) {
            return false;
        }

        // Get device info
        hid_device_info* info = hid_enumerate(0, 0);
        hid_device_info* cur_dev = info;

        while (cur_dev) {
            if (device_id == cur_dev->path) {
                current_device_ = std::make_unique<HIDDevice>();
                current_device_->handle = handle;
                current_device_->vendor_id = cur_dev->vendor_id;
                current_device_->product_id = cur_dev->product_id;
                current_device_->path = device_id;
                current_device_->type = IdentifyDevice(cur_dev->vendor_id, cur_dev->product_id);

                if (cur_dev->serial_number) {
                    std::wstring ws(cur_dev->serial_number);
                    current_device_->serial_number = std::string(ws.begin(), ws.end());
                }

                break;
            }
            cur_dev = cur_dev->next;
        }

        hid_free_enumeration(info);

        if (!current_device_) {
            hid_close(handle);
            return false;
        }

        // Set non-blocking mode
        hid_set_nonblocking(current_device_->handle, 1);

        // Get firmware version
        if (current_device_->type == HWDeviceType::LEDGER_NANO_S ||
            current_device_->type == HWDeviceType::LEDGER_NANO_X) {
            GetLedgerVersion();
        }

        return true;
    }

    void DisconnectDevice() {
        std::lock_guard<std::mutex> lock(device_mutex_);
        current_device_.reset();
    }

    bool IsConnected() const {
        std::lock_guard<std::mutex> lock(device_mutex_);
        return current_device_ && current_device_->handle;
    }

    HWDeviceInfo GetDeviceInfo() const {
        std::lock_guard<std::mutex> lock(device_mutex_);

        HWDeviceInfo info;
        if (current_device_) {
            info.device_id = current_device_->path;
            info.type = current_device_->type;
            info.model_name = HardwareWalletInterface::GetDeviceTypeName(current_device_->type);
            info.firmware_version = current_device_->serial_number;
            info.is_connected = true;
            info.is_initialized = true;
        }

        return info;
    }

    std::string GetAddress(const DerivationPath& path, bool verify_on_device) {
        if (!IsConnected()) {
            return "";
        }

        std::lock_guard<std::mutex> lock(device_mutex_);

        switch (current_device_->type) {
            case HWDeviceType::LEDGER_NANO_S:
            case HWDeviceType::LEDGER_NANO_X:
                return GetLedgerAddress(path, verify_on_device);

            case HWDeviceType::TREZOR_ONE:
            case HWDeviceType::TREZOR_MODEL_T:
                return GetTrezorAddress(path, verify_on_device);

            case HWDeviceType::COLDCARD_MK4:
                return GetColdcardAddress(path, verify_on_device);

            default:
                return "";
        }
    }

    std::vector<std::string> GetAddresses(
        uint32_t start_index,
        uint32_t count,
        uint32_t account,
        uint32_t change
    ) {
        std::vector<std::string> addresses;
        addresses.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            DerivationPath path;
            path.account = account;
            path.change = change;
            path.address_index = start_index + i;

            std::string address = GetAddress(path, false);
            if (!address.empty()) {
                addresses.push_back(address);
            }
        }

        return addresses;
    }

    HWSigningResult SignTransaction(const HWSigningRequest& request) {
        HWSigningResult result;
        result.success = false;

        if (!IsConnected()) {
            result.error_message = "Device not connected";
            return result;
        }

        std::lock_guard<std::mutex> lock(device_mutex_);

        switch (current_device_->type) {
            case HWDeviceType::LEDGER_NANO_S:
            case HWDeviceType::LEDGER_NANO_X:
                return SignLedgerTransaction(request);

            case HWDeviceType::TREZOR_ONE:
            case HWDeviceType::TREZOR_MODEL_T:
                return SignTrezorTransaction(request);

            case HWDeviceType::COLDCARD_MK4:
                return SignColdcardTransaction(request);

            default:
                result.error_message = "Device type not supported";
                return result;
        }
    }

    std::vector<uint8_t> GetPublicKey(const DerivationPath& path) {
        if (!IsConnected()) {
            return {};
        }

        std::lock_guard<std::mutex> lock(device_mutex_);

        switch (current_device_->type) {
            case HWDeviceType::LEDGER_NANO_S:
            case HWDeviceType::LEDGER_NANO_X:
                return GetLedgerPublicKey(path);

            // Other devices would be implemented similarly
            default:
                return {};
        }
    }

    void StartMonitoring() {
        if (monitoring_active_) {
            return;
        }

        monitoring_active_ = true;
        monitor_thread_ = std::thread([this]() {
            while (monitoring_active_) {
                auto devices = EnumerateDevices();

                for (const auto& device : devices) {
                    if (device_callback_) {
                        device_callback_(device);
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }

    void StopMonitoring() {
        if (!monitoring_active_) {
            return;
        }

        monitoring_active_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

    void SetDeviceCallback(DeviceCallback callback) {
        device_callback_ = callback;
    }

private:
    // Device identification
    HWDeviceType IdentifyDevice(uint16_t vid, uint16_t pid) const {
        if (vid == VID_LEDGER) {
            if (pid == PID_LEDGER_NANO_S) return HWDeviceType::LEDGER_NANO_S;
            if (pid == PID_LEDGER_NANO_X) return HWDeviceType::LEDGER_NANO_X;
        }
        else if (vid == VID_TREZOR) {
            if (pid == PID_TREZOR_ONE) return HWDeviceType::TREZOR_ONE;
            if (pid == PID_TREZOR_MODEL_T) return HWDeviceType::TREZOR_MODEL_T;
        }
        else if (vid == VID_COLDCARD && pid == PID_COLDCARD_MK4) {
            return HWDeviceType::COLDCARD_MK4;
        }
        return HWDeviceType::UNKNOWN;
    }

    // Ledger-specific implementations
    APDUResponse SendAPDU(const APDU& apdu) {
        APDUResponse response;

        if (!current_device_ || !current_device_->handle) {
            return response;
        }

        std::vector<uint8_t> command = apdu.Serialize();

        // Send command
        int written = hid_write(current_device_->handle, command.data(), command.size());
        if (written < 0) {
            return response;
        }

        // Read response (may require multiple reads)
        uint8_t buffer[64];
        int read_bytes = hid_read_timeout(current_device_->handle, buffer, sizeof(buffer), 5000);

        if (read_bytes > 2) {
            response.data.assign(buffer, buffer + read_bytes - 2);
            response.status_word = (static_cast<uint16_t>(buffer[read_bytes - 2]) << 8) |
                                   buffer[read_bytes - 1];
        }

        return response;
    }

    void GetLedgerVersion() {
        APDU apdu;
        apdu.cla = CLA_BITCOIN;
        apdu.ins = INS_GET_VERSION;
        apdu.p1 = 0;
        apdu.p2 = 0;

        auto response = SendAPDU(apdu);
        if (response.IsSuccess() && !response.data.empty()) {
            // Parse version from response
            if (response.data.size() >= 3) {
                std::ostringstream oss;
                oss << static_cast<int>(response.data[0]) << "."
                    << static_cast<int>(response.data[1]) << "."
                    << static_cast<int>(response.data[2]);
                current_device_->serial_number = oss.str();
            }
        }
    }

    std::string GetLedgerAddress(const DerivationPath& path, bool verify) {
        APDU apdu;
        apdu.cla = CLA_BITCOIN;
        apdu.ins = INS_GET_ADDRESS;
        apdu.p1 = verify ? 0x01 : 0x00;
        apdu.p2 = 0x00;

        // Encode derivation path
        apdu.data.push_back(5); // Number of derivation levels

        auto append_uint32 = [&apdu](uint32_t value) {
            apdu.data.push_back((value >> 24) & 0xFF);
            apdu.data.push_back((value >> 16) & 0xFF);
            apdu.data.push_back((value >> 8) & 0xFF);
            apdu.data.push_back(value & 0xFF);
        };

        append_uint32(0x8000002C); // 44' (hardened)
        append_uint32(0x80000000 + path.coin_type); // coin_type' (hardened)
        append_uint32(0x80000000 + path.account); // account' (hardened)
        append_uint32(path.change);
        append_uint32(path.address_index);

        auto response = SendAPDU(apdu);

        if (response.IsSuccess() && !response.data.empty()) {
            // Parse address from response (simplified)
            // Real implementation would decode based on address format
            size_t addr_len = response.data[0];
            if (addr_len > 0 && response.data.size() >= addr_len + 1) {
                return std::string(response.data.begin() + 1,
                                 response.data.begin() + 1 + addr_len);
            }
        }

        return "";
    }

    std::vector<uint8_t> GetLedgerPublicKey(const DerivationPath& path) {
        APDU apdu;
        apdu.cla = CLA_BITCOIN;
        apdu.ins = INS_GET_PUBKEY;
        apdu.p1 = 0x00;
        apdu.p2 = 0x00;

        // Encode derivation path (same as GetLedgerAddress)
        apdu.data.push_back(5);

        auto append_uint32 = [&apdu](uint32_t value) {
            apdu.data.push_back((value >> 24) & 0xFF);
            apdu.data.push_back((value >> 16) & 0xFF);
            apdu.data.push_back((value >> 8) & 0xFF);
            apdu.data.push_back(value & 0xFF);
        };

        append_uint32(0x8000002C);
        append_uint32(0x80000000 + path.coin_type);
        append_uint32(0x80000000 + path.account);
        append_uint32(path.change);
        append_uint32(path.address_index);

        auto response = SendAPDU(apdu);

        if (response.IsSuccess()) {
            return response.data;
        }

        return {};
    }

    HWSigningResult SignLedgerTransaction(const HWSigningRequest& request) {
        HWSigningResult result;
        result.success = false;
        result.error_message = "Ledger transaction signing not yet fully implemented";

        // TODO: Implement full Ledger transaction signing protocol
        // This requires chunking the transaction and sending multiple APDUs

        return result;
    }

    // Trezor-specific implementations (placeholder)
    std::string GetTrezorAddress(const DerivationPath& path, bool verify) {
        // TODO: Implement Trezor protocol using Protobuf messages
        return "";
    }

    HWSigningResult SignTrezorTransaction(const HWSigningRequest& request) {
        HWSigningResult result;
        result.success = false;
        result.error_message = "Trezor support not yet implemented";
        return result;
    }

    // Coldcard-specific implementations (placeholder)
    std::string GetColdcardAddress(const DerivationPath& path, bool verify) {
        // TODO: Implement Coldcard protocol
        return "";
    }

    HWSigningResult SignColdcardTransaction(const HWSigningRequest& request) {
        HWSigningResult result;
        result.success = false;
        result.error_message = "Coldcard support not yet implemented";
        return result;
    }

private:
    mutable std::mutex device_mutex_;
    std::unique_ptr<HIDDevice> current_device_;

    // Device monitoring
    std::atomic<bool> monitoring_active_{false};
    std::thread monitor_thread_;
    DeviceCallback device_callback_;
};

// ============================================================================
// HardwareWalletInterface Implementation
// ============================================================================

HardwareWalletInterface::HardwareWalletInterface()
    : pimpl_(std::make_unique<Impl>())
{}

HardwareWalletInterface::~HardwareWalletInterface() = default;

std::vector<HWDeviceInfo> HardwareWalletInterface::EnumerateDevices() {
    return pimpl_->EnumerateDevices();
}

bool HardwareWalletInterface::ConnectDevice(const std::string& device_id) {
    return pimpl_->ConnectDevice(device_id);
}

void HardwareWalletInterface::DisconnectDevice() {
    pimpl_->DisconnectDevice();
}

bool HardwareWalletInterface::IsConnected() const {
    return pimpl_->IsConnected();
}

HWDeviceInfo HardwareWalletInterface::GetDeviceInfo() const {
    return pimpl_->GetDeviceInfo();
}

bool HardwareWalletInterface::InitializeDevice(const std::string& device_name) {
    // Device initialization is typically done through the device's own interface
    return false;
}

std::string HardwareWalletInterface::GetAddress(const DerivationPath& path, bool verify_on_device) {
    return pimpl_->GetAddress(path, verify_on_device);
}

std::vector<std::string> HardwareWalletInterface::GetAddresses(
    uint32_t start_index,
    uint32_t count,
    uint32_t account,
    uint32_t change
) {
    return pimpl_->GetAddresses(start_index, count, account, change);
}

HWSigningResult HardwareWalletInterface::SignTransaction(const HWSigningRequest& request) {
    return pimpl_->SignTransaction(request);
}

bool HardwareWalletInterface::VerifyAddressOnDevice(const std::string& address, const DerivationPath& path) {
    // Get address with verification flag set
    std::string verified_address = GetAddress(path, true);
    return verified_address == address;
}

std::vector<uint8_t> HardwareWalletInterface::GetPublicKey(const DerivationPath& path) {
    return pimpl_->GetPublicKey(path);
}

std::string HardwareWalletInterface::GetExtendedPublicKey(const DerivationPath& path) {
    // TODO: Implement xpub derivation
    return "";
}

bool HardwareWalletInterface::WipeDevice() {
    // This is a dangerous operation and should be implemented carefully
    return false;
}

void HardwareWalletInterface::SetDeviceCallback(DeviceCallback callback) {
    pimpl_->SetDeviceCallback(callback);
}

void HardwareWalletInterface::StartMonitoring() {
    pimpl_->StartMonitoring();
}

void HardwareWalletInterface::StopMonitoring() {
    pimpl_->StopMonitoring();
}

std::vector<HWDeviceType> HardwareWalletInterface::GetSupportedDevices() {
    return {
        HWDeviceType::LEDGER_NANO_S,
        HWDeviceType::LEDGER_NANO_X,
        HWDeviceType::TREZOR_ONE,
        HWDeviceType::TREZOR_MODEL_T,
        HWDeviceType::COLDCARD_MK4
    };
}

std::string HardwareWalletInterface::GetDeviceTypeName(HWDeviceType type) {
    switch (type) {
        case HWDeviceType::LEDGER_NANO_S: return "Ledger Nano S";
        case HWDeviceType::LEDGER_NANO_X: return "Ledger Nano X";
        case HWDeviceType::TREZOR_ONE: return "Trezor One";
        case HWDeviceType::TREZOR_MODEL_T: return "Trezor Model T";
        case HWDeviceType::COLDCARD_MK4: return "Coldcard Mk4";
        default: return "Unknown Device";
    }
}

} // namespace qt
} // namespace intcoin
