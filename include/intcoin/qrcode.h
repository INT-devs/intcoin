// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QRCODE_H
#define INTCOIN_QRCODE_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace intcoin {

/**
 * QR Code Generation Utility
 *
 * Wrapper around libqrencode for generating QR codes from text data.
 * Used for addresses, Lightning invoices, payment requests, etc.
 */
class QRCode {
public:
    /**
     * Error correction level
     */
    enum class ECLevel {
        LOW = 0,      // 7% recovery capacity
        MEDIUM = 1,   // 15% recovery capacity
        QUARTILE = 2, // 25% recovery capacity
        HIGH = 3      // 30% recovery capacity
    };

    /**
     * QR Code data structure
     */
    struct Data {
        int version;              // QR code version (1-40)
        int width;                // Width/height in modules
        std::vector<uint8_t> modules; // Module data (1 = black, 0 = white)

        /**
         * Get module at position (x, y)
         * @param x X coordinate (0 to width-1)
         * @param y Y coordinate (0 to width-1)
         * @return true if module is black, false if white
         */
        bool GetModule(int x, int y) const {
            if (x < 0 || x >= width || y < 0 || y >= width) {
                return false;
            }
            return (modules[y * width + x] & 1) != 0;
        }
    };

    /**
     * Generate QR code from text
     *
     * @param text Text to encode (address, invoice, URL, etc.)
     * @param ec_level Error correction level
     * @return QR code data or std::nullopt on error
     */
    static std::optional<Data> Generate(
        const std::string& text,
        ECLevel ec_level = ECLevel::MEDIUM
    );

    /**
     * Generate QR code for INTcoin address
     *
     * Creates QR code with "intcoin:" URI scheme
     * Example: intcoin:int1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080?amount=1.5
     *
     * @param address INTcoin address (int1...)
     * @param amount Optional amount in INT (e.g., 1.5)
     * @param label Optional payment label
     * @return QR code data or std::nullopt on error
     */
    static std::optional<Data> GenerateAddress(
        const std::string& address,
        double amount = 0.0,
        const std::string& label = ""
    );

    /**
     * Generate QR code for Lightning Network invoice
     *
     * @param invoice Lightning invoice (ln...)
     * @return QR code data or std::nullopt on error
     */
    static std::optional<Data> GenerateLightningInvoice(
        const std::string& invoice
    );

    /**
     * Generate QR code as SVG string
     *
     * Useful for web interfaces and high-quality printing
     *
     * @param text Text to encode
     * @param module_size Size of each module in pixels
     * @param border Border size in modules (default = 4)
     * @param ec_level Error correction level
     * @return SVG string or std::nullopt on error
     */
    static std::optional<std::string> GenerateSVG(
        const std::string& text,
        int module_size = 4,
        int border = 4,
        ECLevel ec_level = ECLevel::MEDIUM
    );

    /**
     * Generate QR code as PNG data
     *
     * @param text Text to encode
     * @param module_size Size of each module in pixels
     * @param border Border size in modules (default = 4)
     * @param ec_level Error correction level
     * @return PNG data bytes or std::nullopt on error
     */
    static std::optional<std::vector<uint8_t>> GeneratePNG(
        const std::string& text,
        int module_size = 4,
        int border = 4,
        ECLevel ec_level = ECLevel::MEDIUM
    );

    /**
     * Get QR code capacity for given version and EC level
     *
     * @param version QR code version (1-40)
     * @param ec_level Error correction level
     * @return Maximum data capacity in bytes
     */
    static int GetCapacity(int version, ECLevel ec_level);

    /**
     * Get recommended EC level for data size
     *
     * For small amounts of data (like addresses), use HIGH for maximum reliability
     * For larger data (like long invoices), use MEDIUM or LOW
     *
     * @param data_size Size of data to encode in bytes
     * @return Recommended error correction level
     */
    static ECLevel GetRecommendedECLevel(size_t data_size);
};

} // namespace intcoin

#endif // INTCOIN_QRCODE_H
