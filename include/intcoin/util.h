/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Utility Functions and Helpers
 */

#ifndef INTCOIN_UTIL_H
#define INTCOIN_UTIL_H

#include "types.h"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <optional>

namespace intcoin {

// ============================================================================
// String Utilities
// ============================================================================

/// Convert bytes to hex string
std::string BytesToHex(const std::vector<uint8_t>& bytes);

/// Convert hex string to bytes
Result<std::vector<uint8_t>> HexToBytes(const std::string& hex);

/// Convert uint256 to hex string
std::string Uint256ToHex(const uint256& hash);

/// Convert hex string to uint256
Result<uint256> HexToUint256(const std::string& hex);

/// Trim whitespace from string
std::string Trim(const std::string& str);

/// Split string by delimiter
std::vector<std::string> Split(const std::string& str, char delimiter);

/// Join strings with delimiter
std::string Join(const std::vector<std::string>& strings,
                const std::string& delimiter);

/// Convert to lowercase
std::string ToLower(const std::string& str);

/// Convert to uppercase
std::string ToUpper(const std::string& str);

// ============================================================================
// Encoding Utilities
// ============================================================================

/// Base64 encode
std::string Base64Encode(const std::vector<uint8_t>& data);

/// Base64 decode
Result<std::vector<uint8_t>> Base64Decode(const std::string& encoded);

/// Bech32 encode
std::string Bech32Encode(const std::string& hrp,
                        const std::vector<uint8_t>& data);

/// Bech32 decode
Result<std::pair<std::string, std::vector<uint8_t>>> Bech32Decode(
    const std::string& encoded);

/// URL encode
std::string URLEncode(const std::string& str);

/// URL decode
std::string URLDecode(const std::string& str);

// ============================================================================
// Numeric Utilities
// ============================================================================

/// Convert INTS to INT (display amount)
double IntsToInt(uint64_t ints);

/// Convert INT to INTS (internal amount)
uint64_t IntToInts(double int_amount);

/// Format amount for display
std::string FormatAmount(uint64_t ints);

/// Parse amount from string
Result<uint64_t> ParseAmount(const std::string& str);

/// Safe addition (check overflow)
Result<uint64_t> SafeAdd(uint64_t a, uint64_t b);

/// Safe subtraction (check underflow)
Result<uint64_t> SafeSubtract(uint64_t a, uint64_t b);

/// Safe multiplication (check overflow)
Result<uint64_t> SafeMultiply(uint64_t a, uint64_t b);

// ============================================================================
// Time Utilities
// ============================================================================

/// Get current Unix timestamp
uint64_t GetCurrentTime();

/// Get current time with milliseconds
uint64_t GetCurrentTimeMillis();

/// Get current time with microseconds
uint64_t GetCurrentTimeMicros();

/// Format timestamp for display
std::string FormatTime(uint64_t timestamp);

/// Parse time string
Result<uint64_t> ParseTime(const std::string& str);

/// Convert duration to string
std::string FormatDuration(std::chrono::seconds duration);

// ============================================================================
// File System Utilities
// ============================================================================

/// Get default data directory
std::string GetDefaultDataDir();

/// Get config file path
std::string GetConfigFilePath();

/// Create directory (recursive)
Result<void> CreateDirectory(const std::string& path);

/// Check if file exists
bool FileExists(const std::string& path);

/// Check if directory exists
bool DirectoryExists(const std::string& path);

/// Get file size
Result<uint64_t> GetFileSize(const std::string& path);

/// Read file to bytes
Result<std::vector<uint8_t>> ReadFile(const std::string& path);

/// Write bytes to file
Result<void> WriteFile(const std::string& path,
                      const std::vector<uint8_t>& data);

/// Delete file
Result<void> DeleteFile(const std::string& path);

/// List files in directory
Result<std::vector<std::string>> ListFiles(const std::string& path);

// ============================================================================
// Serialization Utilities
// ============================================================================

/// Serialize uint8
void SerializeUint8(std::vector<uint8_t>& out, uint8_t value);

/// Serialize uint16
void SerializeUint16(std::vector<uint8_t>& out, uint16_t value);

/// Serialize uint32
void SerializeUint32(std::vector<uint8_t>& out, uint32_t value);

/// Serialize uint64
void SerializeUint64(std::vector<uint8_t>& out, uint64_t value);

/// Serialize uint256
void SerializeUint256(std::vector<uint8_t>& out, const uint256& value);

/// Serialize string
void SerializeString(std::vector<uint8_t>& out, const std::string& value);

/// Serialize vector
template<typename T>
void SerializeVector(std::vector<uint8_t>& out, const std::vector<T>& vec);

/// Deserialize uint8
Result<uint8_t> DeserializeUint8(const std::vector<uint8_t>& data, size_t& pos);

/// Deserialize uint16
Result<uint16_t> DeserializeUint16(const std::vector<uint8_t>& data, size_t& pos);

/// Deserialize uint32
Result<uint32_t> DeserializeUint32(const std::vector<uint8_t>& data, size_t& pos);

/// Deserialize uint64
Result<uint64_t> DeserializeUint64(const std::vector<uint8_t>& data, size_t& pos);

/// Deserialize uint256
Result<uint256> DeserializeUint256(const std::vector<uint8_t>& data, size_t& pos);

/// Deserialize string
Result<std::string> DeserializeString(const std::vector<uint8_t>& data, size_t& pos);

/// Deserialize vector
template<typename T>
Result<std::vector<T>> DeserializeVector(const std::vector<uint8_t>& data, size_t& pos);

// ============================================================================
// Logging
// ============================================================================

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

/// Log message
void Log(LogLevel level, const std::string& message);

/// Log with format
void LogF(LogLevel level, const char* format, ...);

/// Set log level
void SetLogLevel(LogLevel level);

/// Set log file
Result<void> SetLogFile(const std::string& path);

// Convenience macros
#define LOG_TRACE(msg) intcoin::Log(intcoin::LogLevel::TRACE, msg)
#define LOG_DEBUG(msg) intcoin::Log(intcoin::LogLevel::DEBUG, msg)
#define LOG_INFO(msg) intcoin::Log(intcoin::LogLevel::INFO, msg)
#define LOG_WARNING(msg) intcoin::Log(intcoin::LogLevel::WARNING, msg)
#define LOG_ERROR(msg) intcoin::Log(intcoin::LogLevel::ERROR, msg)
#define LOG_FATAL(msg) intcoin::Log(intcoin::LogLevel::FATAL, msg)

// ============================================================================
// Configuration
// ============================================================================

class Config {
public:
    /// Load configuration from file
    static Result<Config> Load(const std::string& path);

    /// Save configuration to file
    Result<void> Save(const std::string& path) const;

    /// Get string value
    std::optional<std::string> GetString(const std::string& key) const;

    /// Get integer value
    std::optional<int64_t> GetInt(const std::string& key) const;

    /// Get boolean value
    std::optional<bool> GetBool(const std::string& key) const;

    /// Set string value
    void SetString(const std::string& key, const std::string& value);

    /// Set integer value
    void SetInt(const std::string& key, int64_t value);

    /// Set boolean value
    void SetBool(const std::string& key, bool value);

    /// Check if key exists
    bool Has(const std::string& key) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Platform Utilities
// ============================================================================

/// Get number of CPU cores
size_t GetCPUCores();

/// Get total system memory (bytes)
uint64_t GetTotalMemory();

/// Get available memory (bytes)
uint64_t GetAvailableMemory();

/// Get platform name
std::string GetPlatform();

/// Get OS version
std::string GetOSVersion();

/// Check if running on 64-bit system
bool Is64Bit();

// ============================================================================
// Random Utilities
// ============================================================================

/// Generate random bytes
std::vector<uint8_t> GetRandomBytes(size_t count);

/// Generate random uint64
uint64_t GetRandomUint64();

/// Generate random uint256
uint256 GetRandomUint256();

// ============================================================================
// Validation Utilities
// ============================================================================

/// Validate address format
bool IsValidAddress(const std::string& address);

/// Validate transaction ID format
bool IsValidTxHash(const std::string& tx_hash);

/// Validate block hash format
bool IsValidBlockHash(const std::string& block_hash);

/// Validate amount
bool IsValidAmount(uint64_t amount);

// ============================================================================
// Hash Utilities
// ============================================================================

/// Calculate SHA3-256 hash
uint256 SHA3_256(const std::vector<uint8_t>& data);

/// Calculate double SHA3-256 hash
uint256 DoubleSHA3_256(const std::vector<uint8_t>& data);

/// Calculate HMAC-SHA3-256
uint256 HMAC_SHA3_256(const std::vector<uint8_t>& key,
                      const std::vector<uint8_t>& message);

} // namespace intcoin

#endif // INTCOIN_UTIL_H
