// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_UINT256_H
#define INTCOIN_UINT256_H

#include <cstdint>
#include <array>

namespace intcoin {

/**
 * @brief 256-bit unsigned integer
 *
 * Simple implementation for hash values and other 256-bit data.
 */
class uint256 {
public:
    std::array<uint64_t, 4> data{{0, 0, 0, 0}};

    uint256() = default;

    uint256(uint64_t val) {
        data[0] = val;
        data[1] = 0;
        data[2] = 0;
        data[3] = 0;
    }

    bool operator==(const uint256& other) const {
        return data == other.data;
    }

    bool operator!=(const uint256& other) const {
        return data != other.data;
    }

    bool operator<(const uint256& other) const {
        for (int i = 3; i >= 0; i--) {
            if (data[i] < other.data[i]) return true;
            if (data[i] > other.data[i]) return false;
        }
        return false;
    }
};

} // namespace intcoin

#endif // INTCOIN_UINT256_H
