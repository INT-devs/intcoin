// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// CRYSTALS-Kyber quantum-resistant key encapsulation using liboqs.

#include "intcoin/crypto.h"
#include <oqs/oqs.h>
#include <cstring>
#include <stdexcept>

namespace intcoin {
namespace crypto {

namespace {
    // Kyber1024 is NIST Level 5 (highest security)
    constexpr const char* KYBER_ALGORITHM = "Kyber1024";

    // Verify sizes match
    static_assert(KYBER_PUBKEY_SIZE == 1568, "Kyber1024 public key size mismatch");
    static_assert(KYBER_CIPHERTEXT_SIZE == 1568, "Kyber1024 ciphertext size mismatch");
    static_assert(KYBER_SHARED_SECRET_SIZE == 32, "Kyber1024 shared secret size mismatch");
}

KyberKeyPair Kyber::generate_keypair() {
    KyberKeyPair keypair;

    OQS_KEM* kem = OQS_KEM_new(KYBER_ALGORITHM);
    if (!kem) {
        throw std::runtime_error("Failed to initialize Kyber1024");
    }

    // Ensure sizes match
    if (kem->length_public_key != KYBER_PUBKEY_SIZE ||
        kem->length_secret_key != keypair.private_key.size() ||
        kem->length_ciphertext != KYBER_CIPHERTEXT_SIZE ||
        kem->length_shared_secret != KYBER_SHARED_SECRET_SIZE) {
        OQS_KEM_free(kem);
        throw std::runtime_error("Kyber1024 size mismatch");
    }

    // Generate keypair
    if (OQS_KEM_keypair(kem, keypair.public_key.data(), keypair.private_key.data()) != OQS_SUCCESS) {
        OQS_KEM_free(kem);
        throw std::runtime_error("Failed to generate Kyber1024 keypair");
    }

    OQS_KEM_free(kem);
    return keypair;
}

std::pair<SharedSecret, KyberCiphertext> Kyber::encapsulate(
    const KyberPubKey& public_key
) {
    SharedSecret shared_secret;
    KyberCiphertext ciphertext;

    OQS_KEM* kem = OQS_KEM_new(KYBER_ALGORITHM);
    if (!kem) {
        throw std::runtime_error("Failed to initialize Kyber1024");
    }

    // Encapsulate
    if (OQS_KEM_encaps(
            kem,
            ciphertext.data(),
            shared_secret.data(),
            public_key.data()
        ) != OQS_SUCCESS) {
        OQS_KEM_free(kem);
        throw std::runtime_error("Failed to encapsulate with Kyber1024");
    }

    OQS_KEM_free(kem);
    return {shared_secret, ciphertext};
}

std::optional<SharedSecret> Kyber::decapsulate(
    const KyberCiphertext& ciphertext,
    const KyberKeyPair& keypair
) {
    SharedSecret shared_secret;

    OQS_KEM* kem = OQS_KEM_new(KYBER_ALGORITHM);
    if (!kem) {
        return std::nullopt;
    }

    // Decapsulate
    OQS_STATUS status = OQS_KEM_decaps(
        kem,
        shared_secret.data(),
        ciphertext.data(),
        keypair.private_key.data()
    );

    OQS_KEM_free(kem);

    if (status != OQS_SUCCESS) {
        return std::nullopt;
    }

    return shared_secret;
}

// KyberKeyPair methods

std::vector<uint8_t> KyberKeyPair::serialize_private() const {
    std::vector<uint8_t> result;
    result.reserve(KYBER_PUBKEY_SIZE + private_key.size());

    // Include public key for verification
    result.insert(result.end(), public_key.begin(), public_key.end());
    result.insert(result.end(), private_key.begin(), private_key.end());

    return result;
}

std::optional<KyberKeyPair> KyberKeyPair::deserialize_private(
    const std::vector<uint8_t>& data
) {
    const size_t expected_size = KYBER_PUBKEY_SIZE + 3168;  // pubkey + privkey

    if (data.size() != expected_size) {
        return std::nullopt;
    }

    KyberKeyPair keypair;

    // Extract public key
    std::copy_n(data.begin(), KYBER_PUBKEY_SIZE, keypair.public_key.begin());

    // Extract private key
    std::copy_n(data.begin() + KYBER_PUBKEY_SIZE, 3168, keypair.private_key.begin());

    return keypair;
}

void KyberKeyPair::clear_private() {
    // Securely zero out private key
    OQS_MEM_cleanse(private_key.data(), private_key.size());
}

} // namespace crypto
} // namespace intcoin
