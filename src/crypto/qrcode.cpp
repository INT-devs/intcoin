// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/qrcode.h>

#ifdef HAVE_QRENCODE
#include <qrencode.h>
#endif

#include <sstream>
#include <iomanip>
#include <cstring>

namespace intcoin {

std::optional<QRCode::Data> QRCode::Generate(
    const std::string& text,
    ECLevel ec_level
) {
#ifdef HAVE_QRENCODE
    if (text.empty()) {
        return std::nullopt;
    }

    // Map ECLevel to libqrencode QRecLevel
    QRecLevel qr_level;
    switch (ec_level) {
        case ECLevel::LOW:      qr_level = QR_ECLEVEL_L; break;
        case ECLevel::MEDIUM:   qr_level = QR_ECLEVEL_M; break;
        case ECLevel::QUARTILE: qr_level = QR_ECLEVEL_Q; break;
        case ECLevel::HIGH:     qr_level = QR_ECLEVEL_H; break;
        default:                qr_level = QR_ECLEVEL_M; break;
    }

    // Generate QR code
    QRcode* qr = QRcode_encodeString(
        text.c_str(),
        0,          // version (0 = auto)
        qr_level,   // error correction level
        QR_MODE_8,  // encoding mode (8-bit data)
        1           // case-sensitive
    );

    if (!qr) {
        return std::nullopt;
    }

    // Copy data to our structure
    Data result;
    result.version = qr->version;
    result.width = qr->width;
    result.modules.resize(qr->width * qr->width);

    std::memcpy(result.modules.data(), qr->data, qr->width * qr->width);

    // Free libqrencode data
    QRcode_free(qr);

    return result;
#else
    // QR code generation not available
    (void)text;
    (void)ec_level;
    return std::nullopt;
#endif
}

std::optional<QRCode::Data> QRCode::GenerateAddress(
    const std::string& address,
    double amount,
    const std::string& label
) {
    // Build URI: intcoin:address[?amount=X][&label=Y]
    std::ostringstream uri;
    uri << "intcoin:" << address;

    bool has_params = false;

    if (amount > 0.0) {
        uri << "?amount=" << std::fixed << std::setprecision(8) << amount;
        has_params = true;
    }

    if (!label.empty()) {
        uri << (has_params ? "&" : "?") << "label=" << label;
    }

    return Generate(uri.str(), ECLevel::HIGH);
}

std::optional<QRCode::Data> QRCode::GenerateLightningInvoice(
    const std::string& invoice
) {
    // Lightning invoices can be long, use MEDIUM EC level
    return Generate(invoice, ECLevel::MEDIUM);
}

std::optional<std::string> QRCode::GenerateSVG(
    const std::string& text,
    int module_size,
    int border,
    ECLevel ec_level
) {
    auto qr_data = Generate(text, ec_level);
    if (!qr_data) {
        return std::nullopt;
    }

    int width = qr_data->width;
    int img_size = (width + 2 * border) * module_size;

    // Build SVG
    std::ostringstream svg;
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "width=\"" << img_size << "\" "
        << "height=\"" << img_size << "\" "
        << "viewBox=\"0 0 " << img_size << " " << img_size << "\">\n";

    // White background
    svg << "  <rect width=\"" << img_size << "\" "
        << "height=\"" << img_size << "\" "
        << "fill=\"#ffffff\"/>\n";

    // Black modules
    svg << "  <g fill=\"#000000\">\n";
    for (int y = 0; y < width; y++) {
        for (int x = 0; x < width; x++) {
            if (qr_data->GetModule(x, y)) {
                int px = (border + x) * module_size;
                int py = (border + y) * module_size;
                svg << "    <rect x=\"" << px << "\" y=\"" << py << "\" "
                    << "width=\"" << module_size << "\" "
                    << "height=\"" << module_size << "\"/>\n";
            }
        }
    }
    svg << "  </g>\n";
    svg << "</svg>\n";

    return svg.str();
}

std::optional<std::vector<uint8_t>> QRCode::GeneratePNG(
    const std::string& text,
    int module_size,
    int border,
    ECLevel ec_level
) {
    // Note: Full PNG generation requires libpng or similar
    // For now, return empty - Qt can render QR codes directly
    // This is a placeholder for future implementation
    auto qr_data = Generate(text, ec_level);
    if (!qr_data) {
        return std::nullopt;
    }

    // TODO: Implement PNG encoding using libpng
    // For now, Qt applications should use the Data structure directly
    return std::nullopt;
}

int QRCode::GetCapacity(int version, ECLevel ec_level) {
    // Capacity table for numeric mode (simplified)
    // Full table would include alphanumeric, byte, and kanji modes
    // See: https://www.qrcode.com/en/about/version.html

    if (version < 1 || version > 40) {
        return 0;
    }

    // Approximate byte capacity for different EC levels
    // This is a simplified calculation
    static const int base_capacity[] = {
        // Versions 1-10 (approximate byte capacity at EC Medium)
        17, 32, 53, 78, 106, 134, 154, 192, 230, 271,
        // Versions 11-20
        321, 367, 425, 458, 520, 586, 644, 718, 792, 858,
        // Versions 21-30
        929, 1003, 1091, 1171, 1273, 1367, 1465, 1528, 1628, 1732,
        // Versions 31-40
        1840, 1952, 2068, 2188, 2303, 2431, 2563, 2699, 2809, 2953
    };

    int capacity = base_capacity[version - 1];

    // Adjust for EC level (approximate)
    switch (ec_level) {
        case ECLevel::LOW:      return capacity * 1.15; // +15%
        case ECLevel::MEDIUM:   return capacity;        // baseline
        case ECLevel::QUARTILE: return capacity * 0.85; // -15%
        case ECLevel::HIGH:     return capacity * 0.70; // -30%
        default:                return capacity;
    }
}

QRCode::ECLevel QRCode::GetRecommendedECLevel(size_t data_size) {
    // Addresses and short data: use HIGH for maximum reliability
    if (data_size <= 50) {
        return ECLevel::HIGH;
    }
    // Medium data: use QUARTILE
    else if (data_size <= 150) {
        return ECLevel::QUARTILE;
    }
    // Larger data: use MEDIUM
    else if (data_size <= 300) {
        return ECLevel::MEDIUM;
    }
    // Very large data: use LOW
    else {
        return ECLevel::LOW;
    }
}

} // namespace intcoin
