// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/ibd/assume_utxo.h>
#include <intcoin/crypto.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>

namespace intcoin {
namespace ibd {

class AssumeUTXOManager::Impl {
public:
    UTXOSnapshot current_snapshot_;
    bool snapshot_active_{false};
    BackgroundProgress bg_progress_;

    std::vector<TrustedSnapshot> hardcoded_snapshots_ = {
        // Example hardcoded snapshots (would be real data in production)
        // {100000, uint256{}, uint256{}},
        // {200000, uint256{}, uint256{}},
    };

    uint256 HashUTXOSet(const std::vector<UTXOEntry>& utxos) const {
        // Create a deterministic serialization of all UTXOs
        std::vector<uint8_t> buffer;

        // Reserve approximate size to reduce reallocations
        buffer.reserve(utxos.size() * 100); // Rough estimate

        for (const auto& utxo : utxos) {
            // Serialize TXID (32 bytes)
            buffer.insert(buffer.end(), utxo.txid.begin(), utxo.txid.end());

            // Serialize vout (4 bytes)
            uint8_t vout_bytes[4];
            std::memcpy(vout_bytes, &utxo.vout, 4);
            buffer.insert(buffer.end(), vout_bytes, vout_bytes + 4);

            // Serialize amount (8 bytes)
            uint8_t amount_bytes[8];
            std::memcpy(amount_bytes, &utxo.amount, 8);
            buffer.insert(buffer.end(), amount_bytes, amount_bytes + 8);

            // Serialize script_pubkey length (4 bytes) and script
            uint32_t script_len = static_cast<uint32_t>(utxo.script_pubkey.size());
            uint8_t len_bytes[4];
            std::memcpy(len_bytes, &script_len, 4);
            buffer.insert(buffer.end(), len_bytes, len_bytes + 4);
            buffer.insert(buffer.end(), utxo.script_pubkey.begin(), utxo.script_pubkey.end());

            // Serialize height (4 bytes)
            uint8_t height_bytes[4];
            std::memcpy(height_bytes, &utxo.height, 4);
            buffer.insert(buffer.end(), height_bytes, height_bytes + 4);

            // Serialize is_coinbase (1 byte)
            buffer.push_back(utxo.is_coinbase ? 1 : 0);
        }

        // Compute SHA3-256 hash of the entire UTXO set
        return SHA3::Hash(buffer);
    }

    bool VerifySignatureImpl(const SnapshotMetadata& metadata) const {
        // Check if signature and public key are present
        if (metadata.signature.empty() || metadata.public_key.empty()) {
            return false;
        }

        // Check signature size (Dilithium3 signature is 3309 bytes)
        if (metadata.signature.size() != DILITHIUM3_BYTES) {
            return false;
        }

        // Check public key size (Dilithium3 public key is 1952 bytes)
        if (metadata.public_key.size() != DILITHIUM3_PUBLICKEYBYTES) {
            return false;
        }

        // Serialize metadata for signing (everything except signature and public_key)
        std::vector<uint8_t> message;
        message.reserve(100); // Approximate size

        // Serialize block_height (4 bytes)
        uint8_t height_bytes[4];
        std::memcpy(height_bytes, &metadata.block_height, 4);
        message.insert(message.end(), height_bytes, height_bytes + 4);

        // Serialize block_hash (32 bytes)
        message.insert(message.end(), metadata.block_hash.begin(), metadata.block_hash.end());

        // Serialize utxo_set_hash (32 bytes)
        message.insert(message.end(), metadata.utxo_set_hash.begin(), metadata.utxo_set_hash.end());

        // Serialize total_amount (8 bytes)
        uint8_t amount_bytes[8];
        std::memcpy(amount_bytes, &metadata.total_amount, 8);
        message.insert(message.end(), amount_bytes, amount_bytes + 8);

        // Serialize num_utxos (8 bytes)
        uint8_t num_bytes[8];
        std::memcpy(num_bytes, &metadata.num_utxos, 8);
        message.insert(message.end(), num_bytes, num_bytes + 8);

        // Serialize timestamp (8 bytes)
        uint8_t time_bytes[8];
        std::memcpy(time_bytes, &metadata.timestamp, 8);
        message.insert(message.end(), time_bytes, time_bytes + 8);

        // Serialize source_url
        message.insert(message.end(), metadata.source_url.begin(), metadata.source_url.end());

        // Copy signature and public key to fixed-size arrays
        Signature signature;
        PublicKey public_key;

        std::copy_n(metadata.signature.begin(),
                    std::min(metadata.signature.size(), signature.size()),
                    signature.begin());
        std::copy_n(metadata.public_key.begin(),
                    std::min(metadata.public_key.size(), public_key.size()),
                    public_key.begin());

        // Verify Dilithium3 signature
        auto result = DilithiumCrypto::Verify(message, signature, public_key);

        return result.IsOk();
    }
};

AssumeUTXOManager::AssumeUTXOManager()
    : pimpl_(std::make_unique<Impl>()) {}

AssumeUTXOManager::~AssumeUTXOManager() = default;

bool AssumeUTXOManager::DownloadSnapshot(const std::string& url, bool verify_signature) {
    // TODO: Implement HTTPS download
    // For now, return false
    return false;
}

bool AssumeUTXOManager::LoadSnapshot(const std::string& snapshot_path) {
    std::ifstream file(snapshot_path, std::ios::binary);
    if (!file) {
        return false;
    }

    // Read snapshot metadata (simple binary format for testing)
    file.read(reinterpret_cast<char*>(&pimpl_->current_snapshot_.metadata.block_height), sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(&pimpl_->current_snapshot_.metadata.num_utxos), sizeof(uint64_t));
    file.read(reinterpret_cast<char*>(&pimpl_->current_snapshot_.metadata.total_amount), sizeof(uint64_t));
    file.read(reinterpret_cast<char*>(&pimpl_->current_snapshot_.metadata.timestamp), sizeof(uint64_t));

    if (!file) {
        return false;
    }

    // Mark snapshot as active
    pimpl_->snapshot_active_ = true;

    return true;
}

VerificationResult AssumeUTXOManager::VerifySnapshot(const UTXOSnapshot& snapshot) const {
    VerificationResult result;
    auto start = std::chrono::steady_clock::now();

    // Compute UTXO set hash
    result.computed_hash = ComputeUTXOHash(snapshot.utxos);

    // Verify hash matches metadata
    if (result.computed_hash == snapshot.metadata.utxo_set_hash) {
        // Check if hash is in trusted snapshots
        bool is_trusted = false;
        for (const auto& trusted : pimpl_->hardcoded_snapshots_) {
            if (trusted.height == snapshot.metadata.block_height &&
                trusted.utxo_hash == snapshot.metadata.utxo_set_hash) {
                is_trusted = true;
                break;
            }
        }

        if (is_trusted) {
            result.valid = true;
        } else {
            // Verify signature if not in hardcoded list
            result.valid = VerifySignature(snapshot.metadata);
            if (!result.valid) {
                result.error_message = "Signature verification failed";
            }
        }
    } else {
        result.valid = false;
        result.error_message = "UTXO set hash mismatch";
    }

    auto end = std::chrono::steady_clock::now();
    result.verification_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

bool AssumeUTXOManager::ApplySnapshot() {
    if (pimpl_->current_snapshot_.utxos.empty()) {
        return false;
    }

    // TODO: Apply UTXO set to chainstate database
    pimpl_->snapshot_active_ = true;

    return true;
}

void AssumeUTXOManager::StartBackgroundValidation() {
    if (!pimpl_->snapshot_active_) {
        return;
    }

    // TODO: Start background thread to validate historical blocks
    pimpl_->bg_progress_.target_height = pimpl_->current_snapshot_.metadata.block_height;
    pimpl_->bg_progress_.validated_height = 0;
}

BackgroundProgress AssumeUTXOManager::GetBackgroundProgress() const {
    BackgroundProgress progress = pimpl_->bg_progress_;

    if (progress.target_height > 0) {
        progress.progress_percent =
            (static_cast<double>(progress.validated_height) / progress.target_height) * 100.0;
    }

    return progress;
}

bool AssumeUTXOManager::IsAssumeUTXOActive() const {
    return pimpl_->snapshot_active_;
}

std::vector<AssumeUTXOManager::TrustedSnapshot>
AssumeUTXOManager::GetTrustedSnapshots() {
    Impl impl;
    return impl.hardcoded_snapshots_;
}

bool AssumeUTXOManager::CreateSnapshot(const std::string& output_path) const {
    std::ofstream file(output_path, std::ios::binary);
    if (!file) {
        return false;
    }

    // Write snapshot metadata (simple binary format for testing)
    // In production, this would serialize the entire UTXO set
    uint32_t block_height = 100000;
    uint64_t num_utxos = 1000000;
    uint64_t total_amount = 21000000ULL * 100000000ULL; // 21M coins in ints
    uint64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    file.write(reinterpret_cast<const char*>(&block_height), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&num_utxos), sizeof(uint64_t));
    file.write(reinterpret_cast<const char*>(&total_amount), sizeof(uint64_t));
    file.write(reinterpret_cast<const char*>(&timestamp), sizeof(uint64_t));

    if (!file) {
        return false;
    }

    return true;
}

std::string AssumeUTXOManager::ExportMetadataJSON() const {
    const auto& meta = pimpl_->current_snapshot_.metadata;

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"block_height\": " << meta.block_height << ",\n";
    oss << "  \"total_amount\": " << meta.total_amount << ",\n";
    oss << "  \"num_utxos\": " << meta.num_utxos << ",\n";
    oss << "  \"timestamp\": " << meta.timestamp << ",\n";
    oss << "  \"source_url\": \"" << meta.source_url << "\"\n";
    oss << "}\n";

    return oss.str();
}

uint256 AssumeUTXOManager::ComputeUTXOHash(const std::vector<UTXOEntry>& utxos) const {
    return pimpl_->HashUTXOSet(utxos);
}

bool AssumeUTXOManager::SignSnapshot(SnapshotMetadata& metadata,
                                     const std::vector<uint8_t>& secret_key) const {
    // Check secret key size (Dilithium3 secret key is 4032 bytes)
    if (secret_key.size() != DILITHIUM3_SECRETKEYBYTES) {
        return false;
    }

    // Serialize metadata for signing (everything except signature and public_key)
    std::vector<uint8_t> message;
    message.reserve(100); // Approximate size

    // Serialize block_height (4 bytes)
    uint8_t height_bytes[4];
    std::memcpy(height_bytes, &metadata.block_height, 4);
    message.insert(message.end(), height_bytes, height_bytes + 4);

    // Serialize block_hash (32 bytes)
    message.insert(message.end(), metadata.block_hash.begin(), metadata.block_hash.end());

    // Serialize utxo_set_hash (32 bytes)
    message.insert(message.end(), metadata.utxo_set_hash.begin(), metadata.utxo_set_hash.end());

    // Serialize total_amount (8 bytes)
    uint8_t amount_bytes[8];
    std::memcpy(amount_bytes, &metadata.total_amount, 8);
    message.insert(message.end(), amount_bytes, amount_bytes + 8);

    // Serialize num_utxos (8 bytes)
    uint8_t num_bytes[8];
    std::memcpy(num_bytes, &metadata.num_utxos, 8);
    message.insert(message.end(), num_bytes, num_bytes + 8);

    // Serialize timestamp (8 bytes)
    uint8_t time_bytes[8];
    std::memcpy(time_bytes, &metadata.timestamp, 8);
    message.insert(message.end(), time_bytes, time_bytes + 8);

    // Serialize source_url
    message.insert(message.end(), metadata.source_url.begin(), metadata.source_url.end());

    // Copy secret key to fixed-size array
    SecretKey sk;
    std::copy_n(secret_key.begin(),
                std::min(secret_key.size(), sk.size()),
                sk.begin());

    // Sign with Dilithium3
    auto sig_result = DilithiumCrypto::Sign(message, sk);

    if (!sig_result.IsOk()) {
        return false;
    }

    // Store signature in metadata
    metadata.signature.resize(DILITHIUM3_BYTES);
    const auto& signature = sig_result.GetValue();
    std::copy(signature.begin(), signature.end(), metadata.signature.begin());

    // Derive public key from secret key (in real implementation, would be done during key generation)
    // For now, assume public key is stored alongside secret key or derived separately
    // This is just storing the provided public key that should accompany the secret key

    return true;
}

bool AssumeUTXOManager::VerifySignature(const SnapshotMetadata& metadata) const {
    return pimpl_->VerifySignatureImpl(metadata);
}

} // namespace ibd
} // namespace intcoin
