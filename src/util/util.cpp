/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Utility Function Implementations
 */

#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include "intcoin/consensus.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <iostream>
#include <fstream>
#include <mutex>
#include <cstdarg>
#include <cstring>
#include <filesystem>

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
    // Remove 0x prefix if present
    std::string hex_str = hex;
    if (hex_str.length() >= 2 && hex_str.substr(0, 2) == "0x") {
        hex_str = hex_str.substr(2);
    }

    // Validate hex string length (uint256 = 32 bytes = 64 hex chars)
    if (hex_str.length() != 64) {
        return Result<uint256>::Error("Invalid hex string length: expected 64 characters, got " + std::to_string(hex_str.length()));
    }

    // Validate hex characters
    for (char c : hex_str) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return Result<uint256>::Error(std::string("Invalid hex character: ") + c);
        }
    }

    // Convert hex string to bytes
    uint256 result;
    for (size_t i = 0; i < 32; ++i) {
        std::string byte_str = hex_str.substr(i * 2, 2);
        result.data()[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
    }

    return Result<uint256>::Ok(result);
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
    // Remove whitespace
    std::string amount_str = Trim(str);

    // Remove " INT" suffix if present
    if (amount_str.length() >= 4 && amount_str.substr(amount_str.length() - 4) == " INT") {
        amount_str = amount_str.substr(0, amount_str.length() - 4);
        amount_str = Trim(amount_str);
    }

    // Check for empty string
    if (amount_str.empty()) {
        return Result<uint64_t>::Error("Empty amount string");
    }

    // Check for negative amounts
    if (amount_str[0] == '-') {
        return Result<uint64_t>::Error("Negative amounts not allowed");
    }

    // Find decimal point
    size_t decimal_pos = amount_str.find('.');

    std::string int_part;
    std::string frac_part;

    if (decimal_pos == std::string::npos) {
        // No decimal point - whole INT amount
        int_part = amount_str;
        frac_part = "";
    } else {
        // Has decimal point - split into integer and fractional parts
        int_part = amount_str.substr(0, decimal_pos);
        frac_part = amount_str.substr(decimal_pos + 1);

        // Validate fractional part has max 6 digits (1 INT = 1,000,000 INTS)
        if (frac_part.length() > 6) {
            return Result<uint64_t>::Error("Too many decimal places (max 6)");
        }

        // Pad fractional part to 6 digits
        while (frac_part.length() < 6) {
            frac_part += "0";
        }
    }

    // Handle empty integer part (e.g., ".5")
    if (int_part.empty()) {
        int_part = "0";
    }

    // Validate numeric characters
    for (char c : int_part) {
        if (c < '0' || c > '9') {
            return Result<uint64_t>::Error(std::string("Invalid character in integer part: ") + c);
        }
    }
    for (char c : frac_part) {
        if (c < '0' || c > '9') {
            return Result<uint64_t>::Error(std::string("Invalid character in fractional part: ") + c);
        }
    }

    // Convert to INTS (satoshis)
    try {
        uint64_t int_value = int_part.empty() ? 0 : std::stoull(int_part);
        uint64_t frac_value = frac_part.empty() ? 0 : std::stoull(frac_part);

        // Check for overflow
        if (int_value > UINT64_MAX / INTS_PER_INT) {
            return Result<uint64_t>::Error("Amount too large");
        }

        uint64_t total_ints = int_value * INTS_PER_INT + frac_value;

        // Validate against max supply
        if (total_ints > consensus::MAX_SUPPLY) {
            return Result<uint64_t>::Error("Amount exceeds max supply");
        }

        return Result<uint64_t>::Ok(total_ints);

    } catch (const std::exception& e) {
        return Result<uint64_t>::Error(std::string("Failed to parse amount: ") + e.what());
    }
}

// ============================================================================
// Base64 Encoding/Decoding
// ============================================================================

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string Base64Encode(const std::vector<uint8_t>& data) {
    std::string encoded;
    int val = 0;
    int valb = -6;

    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (encoded.size() % 4) {
        encoded.push_back('=');
    }

    return encoded;
}

Result<std::vector<uint8_t>> Base64Decode(const std::string& encoded) {
    std::vector<uint8_t> decoded;
    std::vector<int> T(256, -1);

    // Build lookup table
    for (int i = 0; i < 64; i++) {
        T[static_cast<unsigned char>(base64_chars[i])] = i;
    }

    int val = 0;
    int valb = -8;

    for (char c : encoded) {
        if (c == '=') break;  // Padding
        if (T[static_cast<unsigned char>(c)] == -1) {
            return Result<std::vector<uint8_t>>::Error("Invalid base64 character");
        }

        val = (val << 6) + T[static_cast<unsigned char>(c)];
        valb += 6;

        if (valb >= 0) {
            decoded.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return Result<std::vector<uint8_t>>::Ok(decoded);
}

// ============================================================================
// Base58 Encoding/Decoding
// ============================================================================

// Base58 alphabet (Bitcoin-style)
static const char* base58_chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string Base58Encode(const std::vector<uint8_t>& data) {
    // Count leading zeros
    size_t leading_zeros = 0;
    for (size_t i = 0; i < data.size() && data[i] == 0; i++) {
        leading_zeros++;
    }

    // Allocate enough space in big-endian base58 representation
    size_t size = data.size() * 138 / 100 + 1; // log(256) / log(58), rounded up
    std::vector<uint8_t> b58(size);

    // Process the bytes
    for (size_t i = 0; i < data.size(); i++) {
        int carry = data[i];
        // Apply "b58 = b58 * 256 + ch"
        for (size_t j = size; j > 0; j--) {
            carry += 256 * b58[j - 1];
            b58[j - 1] = carry % 58;
            carry /= 58;
        }
    }

    // Skip leading zeros in base58 result
    size_t start_idx = 0;
    while (start_idx < size && b58[start_idx] == 0) {
        start_idx++;
    }

    // Translate to base58 characters
    std::string result;
    result.reserve(leading_zeros + (size - start_idx));

    // Add '1' for each leading zero byte
    for (size_t i = 0; i < leading_zeros; i++) {
        result += '1';
    }

    // Add base58 encoded characters
    for (size_t i = start_idx; i < size; i++) {
        result += base58_chars[b58[i]];
    }

    return result;
}

Result<std::vector<uint8_t>> Base58Decode(const std::string& encoded) {
    // Count leading '1's
    size_t leading_ones = 0;
    for (size_t i = 0; i < encoded.size() && encoded[i] == '1'; i++) {
        leading_ones++;
    }

    // Allocate enough space
    size_t size = encoded.size() * 733 / 1000 + 1; // log(58) / log(256), rounded up
    std::vector<uint8_t> b256(size);

    // Process the characters
    for (size_t i = 0; i < encoded.size(); i++) {
        // Find character in base58 alphabet
        const char* ch = strchr(base58_chars, encoded[i]);
        if (ch == nullptr) {
            return Result<std::vector<uint8_t>>::Error(
                std::string("Invalid base58 character: ") + encoded[i]);
        }

        int carry = static_cast<int>(ch - base58_chars);

        // Apply "b256 = b256 * 58 + carry"
        for (size_t j = size; j > 0; j--) {
            carry += 58 * b256[j - 1];
            b256[j - 1] = carry % 256;
            carry /= 256;
        }
    }

    // Skip leading zeros in result
    size_t start_idx = 0;
    while (start_idx < size && b256[start_idx] == 0) {
        start_idx++;
    }

    // Construct result with leading zeros
    std::vector<uint8_t> result;
    result.reserve(leading_ones + (size - start_idx));

    // Add zero bytes for leading '1's
    for (size_t i = 0; i < leading_ones; i++) {
        result.push_back(0);
    }

    // Add remaining bytes
    for (size_t i = start_idx; i < size; i++) {
        result.push_back(b256[i]);
    }

    return Result<std::vector<uint8_t>>::Ok(result);
}

std::string Base58CheckEncode(const std::vector<uint8_t>& data) {
    // Calculate double SHA3-256 checksum
    uint256 hash = DoubleSHA3_256(data);

    // Take first 4 bytes of hash as checksum
    std::vector<uint8_t> with_checksum = data;
    for (size_t i = 0; i < 4; i++) {
        with_checksum.push_back(hash.data[i]);
    }

    // Base58 encode the data + checksum
    return Base58Encode(with_checksum);
}

Result<std::vector<uint8_t>> Base58CheckDecode(const std::string& encoded) {
    // Base58 decode
    auto decode_result = Base58Decode(encoded);
    if (!decode_result.IsOk()) {
        return Result<std::vector<uint8_t>>::Error(decode_result.error);
    }

    std::vector<uint8_t> decoded = decode_result.value.value();

    // Check minimum length (data + 4 byte checksum)
    if (decoded.size() < 4) {
        return Result<std::vector<uint8_t>>::Error("Decoded data too short for checksum");
    }

    // Split data and checksum
    std::vector<uint8_t> data(decoded.begin(), decoded.end() - 4);
    std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());

    // Calculate expected checksum
    uint256 hash = DoubleSHA3_256(data);

    // Verify checksum
    for (size_t i = 0; i < 4; i++) {
        if (hash.data[i] != checksum[i]) {
            return Result<std::vector<uint8_t>>::Error("Invalid checksum");
        }
    }

    return Result<std::vector<uint8_t>>::Ok(data);
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
    // Platform-specific data directories
#ifdef __APPLE__
    // macOS: ~/Library/Application Support/INTcoin
    const char* home = getenv("HOME");
    if (!home) home = ".";
    return std::string(home) + "/Library/Application Support/INTcoin";
#elif defined(__linux__)
    // Linux: ~/.intcoin
    const char* home = getenv("HOME");
    if (!home) home = ".";
    return std::string(home) + "/.intcoin";
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    // BSD: ~/.intcoin
    const char* home = getenv("HOME");
    if (!home) home = ".";
    return std::string(home) + "/.intcoin";
#elif defined(_WIN32) || defined(_WIN64)
    // Windows: %APPDATA%\INTcoin
    const char* appdata = getenv("APPDATA");
    if (!appdata) appdata = ".";
    return std::string(appdata) + "\\INTcoin";
#else
    // Fallback: current directory
    return "./intcoin_data";
#endif
}

bool FileExists(const std::string& path) {
    // Check if file exists using C++17 filesystem
    try {
        std::filesystem::path p(path);
        return std::filesystem::exists(p) && std::filesystem::is_regular_file(p);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

bool DirectoryExists(const std::string& path) {
    // Check if directory exists using C++17 filesystem
    try {
        std::filesystem::path p(path);
        return std::filesystem::exists(p) && std::filesystem::is_directory(p);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

Result<void> CreateDirectory(const std::string& path) {
    // Create directory recursively using C++17 filesystem
    try {
        std::filesystem::path p(path);

        // Check if directory already exists
        if (std::filesystem::exists(p)) {
            if (std::filesystem::is_directory(p)) {
                return Result<void>::Ok();
            } else {
                return Result<void>::Error("Path exists but is not a directory: " + path);
            }
        }

        // Create all parent directories as needed
        if (!std::filesystem::create_directories(p)) {
            return Result<void>::Error("Failed to create directory: " + path);
        }

        return Result<void>::Ok();
    } catch (const std::filesystem::filesystem_error& e) {
        return Result<void>::Error("Filesystem error creating directory '" + path + "': " + e.what());
    }
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

namespace {
    // Global logging state
    struct LoggerState {
        LogLevel min_level{LogLevel::INFO};
        std::string log_file_path;
        std::ofstream log_file;
        std::mutex log_mutex;
        size_t max_file_size{10 * 1024 * 1024};  // 10 MB default
        size_t current_file_size{0};
        bool initialized{false};
    };

    LoggerState& GetLoggerState() {
        static LoggerState state;
        return state;
    }

    const char* LogLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    void RotateLogFile() {
        auto& state = GetLoggerState();

        if (state.log_file.is_open()) {
            state.log_file.close();
        }

        // Rotate old log files: debug.log.4 -> debug.log.5, etc.
        const int max_rotations = 5;
        for (int i = max_rotations - 1; i >= 0; i--) {
            std::string old_name = state.log_file_path + "." + std::to_string(i);
            std::string new_name = state.log_file_path + "." + std::to_string(i + 1);

            if (FileExists(old_name)) {
                std::rename(old_name.c_str(), new_name.c_str());
            }
        }

        // Rename current log file to .0
        if (FileExists(state.log_file_path)) {
            std::string backup = state.log_file_path + ".0";
            std::rename(state.log_file_path.c_str(), backup.c_str());
        }

        // Open new log file
        state.log_file.open(state.log_file_path, std::ios::out | std::ios::app);
        state.current_file_size = 0;
    }
}

void Log(LogLevel level, const std::string& message) {
    auto& state = GetLoggerState();

    // Check if this log level should be printed
    if (level < state.min_level) {
        return;
    }

    std::lock_guard<std::mutex> lock(state.log_mutex);

    // Format: [2025-12-10 14:30:45] [INFO] message
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_buf;
    std::tm* tm = nullptr;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_buf, &time_t);
    tm = &tm_buf;
#else
    tm = localtime_r(&time_t, &tm_buf);
#endif

    std::ostringstream oss;
    oss << "[" << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms.count()
        << "] [" << LogLevelToString(level) << "] "
        << message;

    std::string log_line = oss.str();

    // Write to console
    if (level >= LogLevel::WARNING) {
        std::cerr << log_line << std::endl;
    } else {
        std::cout << log_line << std::endl;
    }

    // Write to file if configured
    if (state.initialized && state.log_file.is_open()) {
        size_t line_size = log_line.size() + 1;  // +1 for newline

        // Check if rotation is needed
        if (state.current_file_size + line_size > state.max_file_size) {
            RotateLogFile();
        }

        if (state.log_file.is_open()) {
            state.log_file << log_line << std::endl;
            state.log_file.flush();  // Ensure immediate write for important logs
            state.current_file_size += line_size;
        }
    }
}

void SetLogLevel(LogLevel level) {
    auto& state = GetLoggerState();
    std::lock_guard<std::mutex> lock(state.log_mutex);
    state.min_level = level;
}

Result<void> SetLogFile(const std::string& path) {
    auto& state = GetLoggerState();
    std::lock_guard<std::mutex> lock(state.log_mutex);

    // Close existing file if open
    if (state.log_file.is_open()) {
        state.log_file.close();
    }

    state.log_file_path = path;

    // Open log file in append mode
    state.log_file.open(path, std::ios::out | std::ios::app);
    if (!state.log_file.is_open()) {
        state.initialized = false;
        return Result<void>::Error("Failed to open log file: " + path);
    }

    // Get current file size
    state.log_file.seekp(0, std::ios::end);
    state.current_file_size = static_cast<size_t>(state.log_file.tellp());
    state.initialized = true;

    return Result<void>::Ok();
}

void LogF(LogLevel level, const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Log(level, std::string(buffer));
}

// ============================================================================
// Random Number Generation
// ============================================================================

std::vector<uint8_t> GetRandomBytes(size_t count) {
    return RandomGenerator::GetRandomBytes(count);
}

uint64_t GetRandomUint64() {
    return RandomGenerator::GetRandomUint64();
}

uint256 GetRandomUint256() {
    return RandomGenerator::GetRandomUint256();
}

} // namespace intcoin
