# INTcoin Genesis Block

## Mainnet Genesis Block

The INTcoin mainnet genesis block was successfully mined on November 14, 2025.

### Genesis Block Parameters

```
Message:      "BBC News 06/Nov/2025 Will quantum be bigger than AI?"
Timestamp:    1730851200 (November 6, 2025 00:00:00 UTC)
Nonce:        1233778760
Difficulty:   0x1d00ffff
Block Hash:   000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9
Merkle Root:  f98a5c8bc3eeadaee865ecbdf5ac955fef4614ca2551a250a962b42a000d36f5
```

### Mining Statistics

```
Total Hashes: 1,233,778,761
Mining Time:  478 seconds (~8 minutes)
Hash Rate:    2,581,127 H/s
```

### Block Details

The genesis block contains a reference to a BBC News article from November 6, 2025, discussing the significance of quantum computing in comparison to artificial intelligence. This timestamp marks the beginning of the INTcoin blockchain, a quantum-resistant cryptocurrency built with NIST-approved post-quantum cryptographic algorithms.

### Verification

To verify the genesis block, you can run:

```bash
./build/intcoind --verify-genesis
```

Or programmatically check the block hash matches the expected value in `src/core/block.cpp`.

### Technical Implementation

The genesis block is created in `src/core/block.cpp` using the `GenesisBlock::create_mainnet()` function with the parameters listed above. The block uses:

- **Signature Algorithm**: CRYSTALS-Dilithium5 (NIST FIPS 204 ML-DSA-87)
- **Key Exchange**: CRYSTALS-Kyber1024 (NIST FIPS 203 ML-KEM-1024)
- **Hash Function**: SHA3-256 (NIST FIPS 202)
- **Proof of Work**: SHA-256

### Launch Details

- **Network Launch**: January 1, 2026
- **Network Status**: Pre-launch (Mining in Progress)
- **Genesis Block Height**: 0
- **Initial Block Reward**: 105,113,636 INT
- **Halving Interval**: 1,051,200 blocks (~4 years)
- **Max Supply**: 221,000,000,000,000 INT

## Testnet Genesis Block

The testnet genesis block uses different parameters for testing purposes. See `src/core/block.cpp` for testnet configuration.
