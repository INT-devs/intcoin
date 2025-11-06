// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// CRYSTALS-Dilithium quantum-resistant signatures using liboqs.

#include "intcoin/crypto.h"
#include <oqs/oqs.h>
#include <cstring>
#include <stdexcept>
#include <memory>

namespace intcoin {
namespace crypto {

namespace {
    // ML-DSA-87 is the NIST-standardized version of Dilithium5 (FIPS 204)
    // Provides NIST Security Level 5 (highest)
    constexpr const char* DILITHIUM_ALGORITHM = "ML-DSA-87";

    // Verify sizes match ML-DSA-87 specifications
    static_assert(DILITHIUM_PUBKEY_SIZE == 2592, "ML-DSA-87 public key size");
    static_assert(DILITHIUM_SIGNATURE_SIZE == 4627, "ML-DSA-87 signature size");
}

DilithiumKeyPair Dilithium::generate_keypair() {
    DilithiumKeyPair keypair;

    OQS_SIG* sig = OQS_SIG_new(DILITHIUM_ALGORITHM);
    if (!sig) {
        throw std::runtime_error("Failed to initialize ML-DSA-87");
    }

    // Ensure sizes match
    if (sig->length_public_key != DILITHIUM_PUBKEY_SIZE ||
        sig->length_secret_key != keypair.private_key.size() ||
        sig->length_signature != DILITHIUM_SIGNATURE_SIZE) {
        OQS_SIG_free(sig);
        throw std::runtime_error("ML-DSA-87 size mismatch");
    }

    // Generate keypair
    if (OQS_SIG_keypair(sig, keypair.public_key.data(), keypair.private_key.data()) != OQS_SUCCESS) {
        OQS_SIG_free(sig);
        throw std::runtime_error("Failed to generate ML-DSA-87 keypair");
    }

    OQS_SIG_free(sig);
    return keypair;
}

DilithiumSignature Dilithium::sign(
    const std::vector<uint8_t>& message,
    const DilithiumKeyPair& keypair
) {
    DilithiumSignature signature;

    OQS_SIG* sig = OQS_SIG_new(DILITHIUM_ALGORITHM);
    if (!sig) {
        throw std::runtime_error("Failed to initialize ML-DSA-87");
    }

    size_t signature_len = DILITHIUM_SIGNATURE_SIZE;

    if (OQS_SIG_sign(
            sig,
            signature.data(),
            &signature_len,
            message.data(),
            message.size(),
            keypair.private_key.data()
        ) != OQS_SUCCESS) {
        OQS_SIG_free(sig);
        throw std::runtime_error("Failed to sign with ML-DSA-87");
    }

    if (signature_len != DILITHIUM_SIGNATURE_SIZE) {
        OQS_SIG_free(sig);
        throw std::runtime_error("ML-DSA-87 produced incorrect signature size");
    }

    OQS_SIG_free(sig);
    return signature;
}

bool Dilithium::verify(
    const std::vector<uint8_t>& message,
    const DilithiumSignature& signature,
    const DilithiumPubKey& public_key
) {
    OQS_SIG* sig = OQS_SIG_new(DILITHIUM_ALGORITHM);
    if (!sig) {
        return false;
    }

    OQS_STATUS status = OQS_SIG_verify(
        sig,
        message.data(),
        message.size(),
        signature.data(),
        DILITHIUM_SIGNATURE_SIZE,
        public_key.data()
    );

    OQS_SIG_free(sig);
    return status == OQS_SUCCESS;
}

// DilithiumKeyPair methods

std::vector<uint8_t> DilithiumKeyPair::serialize_private() const {
    std::vector<uint8_t> result;
    result.reserve(DILITHIUM_PUBKEY_SIZE + private_key.size());

    // Include public key for verification
    result.insert(result.end(), public_key.begin(), public_key.end());
    result.insert(result.end(), private_key.begin(), private_key.end());

    return result;
}

std::optional<DilithiumKeyPair> DilithiumKeyPair::deserialize_private(
    const std::vector<uint8_t>& data
) {
    const size_t expected_size = DILITHIUM_PUBKEY_SIZE + 4896;  // pubkey + privkey

    if (data.size() != expected_size) {
        return std::nullopt;
    }

    DilithiumKeyPair keypair;

    // Extract public key
    std::copy_n(data.begin(), DILITHIUM_PUBKEY_SIZE, keypair.public_key.begin());

    // Extract private key
    std::copy_n(data.begin() + DILITHIUM_PUBKEY_SIZE, 4896, keypair.private_key.begin());

    return keypair;
}

void DilithiumKeyPair::clear_private() {
    // Securely zero out private key
    OQS_MEM_cleanse(private_key.data(), private_key.size());
}

} // namespace crypto
} // namespace intcoin
