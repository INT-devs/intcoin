/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Utility Function Implementations
 */

#include "intcoin/util.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <iostream>

namespace intcoin {

// ============================================================================
// String Utilities
// ============================================================================

std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    for (auto byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(byte);
    }
    return ss.str();
}

Result<std::vector<uint8_t>> HexToBytes(const std::string& hex) {
    // Check for even length
    if (hex.length() % 2 != 0) {
        return Result<std::vector<uint8_t>>::Error("Hex string must have even length");
    }

    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);

    for (size_t i = 0; i < hex.length(); i += 2) {
        // Get two hex characters
        std::string byte_str = hex.substr(i, 2);

        // Convert to byte
        char* end;
        long value = std::strtol(byte_str.c_str(), &end, 16);

        // Check for conversion errors
        if (end != byte_str.c_str() + 2) {
            return Result<std::vector<uint8_t>>::Error(
                "Invalid hex character at position " + std::to_string(i));
        }

        bytes.push_back(static_cast<uint8_t>(value));
    }

    return Result<std::vector<uint8_t>>::Ok(std::move(bytes));
}

std::string Uint256ToHex(const uint256& hash) {
    return ToHex(hash);
}

Result<uint256> HexToUint256(const std::string& hex) {
    // TODO: Implement hex to uint256 conversion
    return Result<uint256>::Error("Not implemented");
}

std::string Trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    auto end = str.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

std::string Join(const std::vector<std::string>& strings,
                const std::string& delimiter) {
    std::stringstream ss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) ss << delimiter;
        ss << strings[i];
    }
    return ss.str();
}

std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// ============================================================================
// Numeric Utilities
// ============================================================================

// Note: IntsToInt and IntToInts are inline in types.h

std::string FormatAmount(uint64_t ints) {
    double int_amount = IntsToInt(ints);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6) << int_amount << " INT";
    return ss.str();
}

Result<uint64_t> ParseAmount(const std::string& str) {
    // TODO: Implement amount parsing
    return Result<uint64_t>::Error("Not implemented");
}

// ============================================================================
// Time Utilities
// ============================================================================

uint64_t GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t GetCurrentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t GetCurrentTimeMicros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string FormatTime(uint64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::gmtime(&time);
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

// ============================================================================
// File System Utilities
// ============================================================================

std::string GetDefaultDataDir() {
    // TODO: Implement platform-specific data directory
#ifdef __APPLE__
    return std::string(getenv("HOME")) + "/Library/Application Support/INTcoin";
#elif __linux__
    return std::string(getenv("HOME")) + "/.intcoin";
#elif _WIN32
    return std::string(getenv("APPDATA")) + "\\INTcoin";
#else
    return "./intcoin_data";
#endif
}

bool FileExists(const std::string& path) {
    // TODO: Implement file existence check
    return false;
}

bool DirectoryExists(const std::string& path) {
    // TODO: Implement directory existence check
    return false;
}

// ============================================================================
// Serialization Utilities
// ============================================================================

void SerializeUint8(std::vector<uint8_t>& out, uint8_t value) {
    out.push_back(value);
}

void SerializeUint16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(value & 0xFF);
    out.push_back((value >> 8) & 0xFF);
}

void SerializeUint32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(value & 0xFF);
    out.push_back((value >> 8) & 0xFF);
    out.push_back((value >> 16) & 0xFF);
    out.push_back((value >> 24) & 0xFF);
}

void SerializeUint64(std::vector<uint8_t>& out, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        out.push_back((value >> (i * 8)) & 0xFF);
    }
}

void SerializeUint256(std::vector<uint8_t>& out, const uint256& value) {
    out.insert(out.end(), value.begin(), value.end());
}

void SerializeString(std::vector<uint8_t>& out, const std::string& value) {
    SerializeUint64(out, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

// ============================================================================
// Deserialization Utilities
// ============================================================================

Result<uint8_t> DeserializeUint8(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 1 > data.size()) {
        return Result<uint8_t>::Error("Buffer underflow: not enough bytes for uint8");
    }
    uint8_t value = data[pos];
    pos += 1;
    return Result<uint8_t>::Ok(value);
}

Result<uint16_t> DeserializeUint16(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 2 > data.size()) {
        return Result<uint16_t>::Error("Buffer underflow: not enough bytes for uint16");
    }
    uint16_t value = data[pos] | (static_cast<uint16_t>(data[pos + 1]) << 8);
    pos += 2;
    return Result<uint16_t>::Ok(value);
}

Result<uint32_t> DeserializeUint32(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 4 > data.size()) {
        return Result<uint32_t>::Error("Buffer underflow: not enough bytes for uint32");
    }
    uint32_t value = data[pos] |
                    (static_cast<uint32_t>(data[pos + 1]) << 8) |
                    (static_cast<uint32_t>(data[pos + 2]) << 16) |
                    (static_cast<uint32_t>(data[pos + 3]) << 24);
    pos += 4;
    return Result<uint32_t>::Ok(value);
}

Result<uint64_t> DeserializeUint64(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 8 > data.size()) {
        return Result<uint64_t>::Error("Buffer underflow: not enough bytes for uint64");
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= (static_cast<uint64_t>(data[pos + i]) << (i * 8));
    }
    pos += 8;
    return Result<uint64_t>::Ok(value);
}

Result<uint256> DeserializeUint256(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 32 > data.size()) {
        return Result<uint256>::Error("Buffer underflow: not enough bytes for uint256");
    }
    uint256 value{};
    std::copy(data.begin() + pos, data.begin() + pos + 32, value.begin());
    pos += 32;
    return Result<uint256>::Ok(value);
}

Result<std::string> DeserializeString(const std::vector<uint8_t>& data, size_t& pos) {
    // Read string length
    auto len_result = DeserializeUint64(data, pos);
    if (len_result.IsError()) {
        return Result<std::string>::Error("Failed to deserialize string length: " + len_result.error);
    }
    uint64_t length = *len_result.value;

    // Check if we have enough bytes
    if (pos + length > data.size()) {
        return Result<std::string>::Error("Buffer underflow: not enough bytes for string");
    }

    // Read string data
    std::string value(data.begin() + pos, data.begin() + pos + length);
    pos += length;
    return Result<std::string>::Ok(value);
}

// ============================================================================
// Logging
// ============================================================================

void Log(LogLevel level, const std::string& message) {
    // TODO: Implement proper logging
    const char* level_str[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    std::cout << "[" << level_str[static_cast<int>(level)] << "] "
              << message << std::endl;
}

void SetLogLevel(LogLevel level) {
    // TODO: Implement log level setting
}

Result<void> SetLogFile(const std::string& path) {
    // TODO: Implement log file setting
    return Result<void>::Error("Not implemented");
}

} // namespace intcoin
