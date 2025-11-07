// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_SERIALIZATION_H
#define INTCOIN_SERIALIZATION_H

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <optional>

namespace intcoin {
namespace serialization {

/**
 * Serialization format version
 * Increment when making breaking changes to serialization format
 */
constexpr uint32_t SERIALIZATION_VERSION = 1;

/**
 * Maximum serialized size limits (prevent DOS attacks)
 */
constexpr size_t MAX_BLOCK_SIZE = 4 * 1024 * 1024;  // 4 MB
constexpr size_t MAX_TX_SIZE = 1 * 1024 * 1024;      // 1 MB
constexpr size_t MAX_MESSAGE_SIZE = 32 * 1024 * 1024; // 32 MB

/**
 * Serialization error
 */
class SerializationError : public std::runtime_error {
public:
    explicit SerializationError(const std::string& msg)
        : std::runtime_error(msg) {}
};

/**
 * Binary serializer with bounds checking and versioning
 */
class Serializer {
private:
    std::vector<uint8_t> buffer_;
    size_t max_size_;

public:
    explicit Serializer(size_t max_size = MAX_MESSAGE_SIZE)
        : max_size_(max_size) {
        buffer_.reserve(1024); // Initial capacity
    }

    // Check if adding size would exceed limit
    bool check_size(size_t additional_size) const {
        return buffer_.size() + additional_size <= max_size_;
    }

    // Write uint8_t
    void write_uint8(uint8_t value) {
        if (!check_size(1)) {
            throw SerializationError("Size limit exceeded");
        }
        buffer_.push_back(value);
    }

    // Write uint16_t (little-endian)
    void write_uint16(uint16_t value) {
        if (!check_size(2)) {
            throw SerializationError("Size limit exceeded");
        }
        buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    // Write uint32_t (little-endian)
    void write_uint32(uint32_t value) {
        if (!check_size(4)) {
            throw SerializationError("Size limit exceeded");
        }
        buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    // Write uint64_t (little-endian)
    void write_uint64(uint64_t value) {
        if (!check_size(8)) {
            throw SerializationError("Size limit exceeded");
        }
        for (int i = 0; i < 8; ++i) {
            buffer_.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
        }
    }

    // Write variable-length integer (CompactSize)
    void write_varint(uint64_t value) {
        if (value < 0xFD) {
            write_uint8(static_cast<uint8_t>(value));
        } else if (value <= 0xFFFF) {
            write_uint8(0xFD);
            write_uint16(static_cast<uint16_t>(value));
        } else if (value <= 0xFFFFFFFF) {
            write_uint8(0xFE);
            write_uint32(static_cast<uint32_t>(value));
        } else {
            write_uint8(0xFF);
            write_uint64(value);
        }
    }

    // Write raw bytes
    void write_bytes(const uint8_t* data, size_t size) {
        if (!check_size(size)) {
            throw SerializationError("Size limit exceeded");
        }
        buffer_.insert(buffer_.end(), data, data + size);
    }

    // Write vector with length prefix
    void write_vector(const std::vector<uint8_t>& data) {
        write_varint(data.size());
        write_bytes(data.data(), data.size());
    }

    // Write string with length prefix
    void write_string(const std::string& str) {
        write_varint(str.size());
        write_bytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }

    // Get serialized data
    const std::vector<uint8_t>& data() const {
        return buffer_;
    }

    // Get size
    size_t size() const {
        return buffer_.size();
    }

    // Clear buffer
    void clear() {
        buffer_.clear();
    }
};

/**
 * Binary deserializer with bounds checking and validation
 */
class Deserializer {
private:
    const uint8_t* data_;
    size_t size_;
    size_t offset_;

public:
    Deserializer(const uint8_t* data, size_t size)
        : data_(data), size_(size), offset_(0) {}

    Deserializer(const std::vector<uint8_t>& data)
        : data_(data.data()), size_(data.size()), offset_(0) {}

    // Check if we can read size bytes
    bool can_read(size_t bytes) const {
        return offset_ + bytes <= size_;
    }

    // Get remaining bytes
    size_t remaining() const {
        return size_ - offset_;
    }

    // Read uint8_t
    std::optional<uint8_t> read_uint8() {
        if (!can_read(1)) {
            return std::nullopt;
        }
        return data_[offset_++];
    }

    // Read uint16_t (little-endian)
    std::optional<uint16_t> read_uint16() {
        if (!can_read(2)) {
            return std::nullopt;
        }
        uint16_t value = static_cast<uint16_t>(data_[offset_]) |
                        (static_cast<uint16_t>(data_[offset_ + 1]) << 8);
        offset_ += 2;
        return value;
    }

    // Read uint32_t (little-endian)
    std::optional<uint32_t> read_uint32() {
        if (!can_read(4)) {
            return std::nullopt;
        }
        uint32_t value = static_cast<uint32_t>(data_[offset_]) |
                        (static_cast<uint32_t>(data_[offset_ + 1]) << 8) |
                        (static_cast<uint32_t>(data_[offset_ + 2]) << 16) |
                        (static_cast<uint32_t>(data_[offset_ + 3]) << 24);
        offset_ += 4;
        return value;
    }

    // Read uint64_t (little-endian)
    std::optional<uint64_t> read_uint64() {
        if (!can_read(8)) {
            return std::nullopt;
        }
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(data_[offset_ + i]) << (i * 8);
        }
        offset_ += 8;
        return value;
    }

    // Read variable-length integer (CompactSize)
    std::optional<uint64_t> read_varint() {
        auto first = read_uint8();
        if (!first) return std::nullopt;

        if (*first < 0xFD) {
            return static_cast<uint64_t>(*first);
        } else if (*first == 0xFD) {
            auto value = read_uint16();
            return value ? std::optional<uint64_t>(*value) : std::nullopt;
        } else if (*first == 0xFE) {
            auto value = read_uint32();
            return value ? std::optional<uint64_t>(*value) : std::nullopt;
        } else {
            return read_uint64();
        }
    }

    // Read raw bytes
    std::optional<std::vector<uint8_t>> read_bytes(size_t count) {
        if (!can_read(count)) {
            return std::nullopt;
        }
        std::vector<uint8_t> result(data_ + offset_, data_ + offset_ + count);
        offset_ += count;
        return result;
    }

    // Read vector with length prefix
    std::optional<std::vector<uint8_t>> read_vector() {
        auto length = read_varint();
        if (!length) return std::nullopt;

        // Sanity check on length
        if (*length > remaining()) {
            return std::nullopt;
        }

        return read_bytes(static_cast<size_t>(*length));
    }

    // Read string with length prefix
    std::optional<std::string> read_string() {
        auto bytes = read_vector();
        if (!bytes) return std::nullopt;

        return std::string(bytes->begin(), bytes->end());
    }

    // Get current offset
    size_t offset() const {
        return offset_;
    }

    // Skip bytes
    bool skip(size_t count) {
        if (!can_read(count)) {
            return false;
        }
        offset_ += count;
        return true;
    }
};

/**
 * Serialization version header
 */
struct VersionHeader {
    uint32_t version;
    uint32_t type;  // Object type identifier

    static constexpr uint32_t TYPE_BLOCK = 1;
    static constexpr uint32_t TYPE_TRANSACTION = 2;
    static constexpr uint32_t TYPE_BLOCK_UNDO = 3;

    void serialize(Serializer& s) const {
        s.write_uint32(version);
        s.write_uint32(type);
    }

    static std::optional<VersionHeader> deserialize(Deserializer& d) {
        auto version = d.read_uint32();
        auto type = d.read_uint32();

        if (!version || !type) {
            return std::nullopt;
        }

        return VersionHeader{*version, *type};
    }
};

} // namespace serialization
} // namespace intcoin

#endif // INTCOIN_SERIALIZATION_H
