# INTcoin
## Quantum-Resistant Cryptocurrency

**Built for the Quantum Era**

---

## What is INTcoin?

INTcoin is a next-generation cryptocurrency designed to resist attacks from quantum computers. While other cryptocurrencies remain vulnerable to future quantum threats, INTcoin uses **NIST-standardized post-quantum cryptography** to ensure your funds remain secure—today and tomorrow.

---

## The Quantum Threat

Quantum computers pose an existential threat to traditional cryptocurrencies:

- **Bitcoin & Ethereum** use ECDSA signatures → **Vulnerable** to Shor's algorithm
- **Current encryption** can be broken in hours by future quantum computers
- **Your funds are at risk** once quantum computers become practical (est. 2030-2035)

**INTcoin solves this problem today.**

---

## Key Features

### 🛡️ Quantum-Resistant Cryptography

- **CRYSTALS-Dilithium5** digital signatures (NIST Level 5 security)
- **CRYSTALS-Kyber1024** encryption for quantum-safe communication
- **SHA3-256** hashing based on Keccak
- Resistant to both classical and quantum attacks

### ⚡ Lightning Network Support

- Instant payments with microsecond confirmation
- Low fees for everyday transactions
- Scalable to billions of transactions per second
- Quantum-resistant payment channels

### 🌐 True Decentralization

- **SHA-256 Proof of Work** (battle-tested Bitcoin algorithm)
- No pre-mine, no ICO, no founder allocation
- No staking or governance (pure Proof of Work)
- Fair launch for all participants

### 🔒 Privacy & Security

- Pseudonymous transactions (like Bitcoin)
- TOR network support for anonymity
- Optional Lightning Network privacy
- Post-quantum secure end-to-end

### 🚀 Modern Technology

- C++23 codebase with cutting-edge performance
- Cross-platform (Linux, macOS, FreeBSD, Windows)
- Full node implementation with wallet
- Comprehensive test coverage

---

## Technical Specifications

| Feature | Specification |
|---------|---------------|
| **Consensus** | Proof of Work (SHA-256 double hash) |
| **Block Time** | 2 minutes (120 seconds) |
| **Block Size** | 4 MB |
| **Supply** | 221 trillion INT (halving every 2,016 blocks) |
| **Initial Reward** | 50 INT per block |
| **Difficulty Adjustment** | Every 2,016 blocks (~4.67 days) |
| **Signatures** | CRYSTALS-Dilithium5 (ML-DSA-87, 4595 bytes) |
| **Encryption** | CRYSTALS-Kyber1024 (ML-KEM-1024, 3168 bytes) |
| **Hash Function** | SHA3-256 (FIPS 202) |
| **P2P Port** | 9333 (mainnet), 18333 (testnet) |
| **RPC Port** | 9332 (mainnet), 18332 (testnet) |
| **Lightning** | Quantum-resistant payment channels |

---

## Why INTcoin?

### Future-Proof Security
INTcoin is the **only major cryptocurrency** with NIST Level 5 post-quantum security from day one.

### Decentralized Forever
ASIC resistance ensures mining remains accessible to everyone, preventing centralization.

### Fast & Scalable
Lightning Network enables billions of transactions per second with instant finality.

### Battle-Tested Cryptography
Uses **NIST-standardized** algorithms that have undergone years of public scrutiny.

---

## Use Cases

### 💰 Store of Value
Protect your wealth against both current and future threats.

### 🛒 Everyday Payments
Lightning Network enables instant, low-cost purchases.

### 🏦 Remittances
Send money globally without intermediaries or quantum risk.

### 🔐 Long-Term Savings
The only crypto proven safe for 50+ year time horizons.

---

## Comparison

|  | INTcoin | Bitcoin | Ethereum |
|--|---------|---------|----------|
| **Quantum-Resistant** | ✅ Yes (Dilithium5) | ❌ No (ECDSA) | ❌ No (ECDSA) |
| **Block Time** | ✅ 2 min | ⚠️ 10 min | ⚠️ 12 sec |
| **Lightning Network** | ✅ Yes | ✅ Yes | ❌ No |
| **NIST Standardized** | ✅ Yes (2024) | ❌ No | ❌ No |
| **True PoW** | ✅ Yes | ✅ Yes | ❌ No (PoS) |

---

## Roadmap

### Phase 1: Foundation ✅
- Core blockchain implementation
- Quantum-resistant crypto (Dilithium, Kyber)
- Wallet with HD key derivation
- P2P network protocol

### Phase 2: Current 🔄
- Lightning Network integration
- TOR network support
- Comprehensive testing (unit, functional, fuzz)
- Mining pool support

### Phase 3: Launch 🎯
- Mainnet launch
- Exchange listings
- Block explorer
- Mobile wallets (iOS, Android)

### Phase 4: Growth 🚀
- Hardware wallet support (Ledger, Trezor)
- Atomic swaps with other cryptocurrencies
- Payment processor integrations
- Merchant adoption

---

## Get Started

### For Users
```bash
# Install wallet
curl -sSL https://international-coin.org/install.sh | bash

# Create wallet
intcoin-wallet create

# Start node
intcoind --daemon
```

### For Miners
```bash
# Start mining
intcoin-miner --address=<your-address> --threads=4
```

### For Developers
```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git

# Build from source
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## Community

- **Website**: [international-coin.org](https://international-coin.org)
- **Repository**: [gitlab.com/intcoin/crypto](https://gitlab.com/intcoin/crypto)
- **Email**: team@international-coin.org
- **Discord**: discord.gg/intcoin
- **Twitter/X**: @intcoin_crypto
- **Reddit**: r/INTcoin

---

## Open Source

INTcoin is **100% open source** under the MIT License.

- No corporate control
- No venture capital funding
- No pre-mine or founder rewards
- Community-driven development

**View the code**: [gitlab.com/intcoin/crypto](https://gitlab.com/intcoin/crypto)

---

## Security

INTcoin has undergone extensive security testing:

- ✅ Comprehensive unit tests (400+ test cases)
- ✅ Functional test framework
- ✅ Fuzz testing with libFuzzer and AFL
- ✅ Memory safety (AddressSanitizer, MemorySanitizer)
- ✅ Undefined behavior detection (UBSan)

**Report vulnerabilities**: security@international-coin.org (PGP key available)

---

## FAQ

**Q: Is INTcoin really quantum-resistant?**
A: Yes. INTcoin uses CRYSTALS-Dilithium5 and Kyber1024, both NIST-standardized post-quantum algorithms with the highest security level (Level 5).

**Q: When will quantum computers break Bitcoin?**
A: Estimates range from 2030-2035. INTcoin protects you today.

**Q: Can I mine INTcoin?**
A: Yes! INTcoin uses standard SHA-256 Proof of Work, the same algorithm as Bitcoin, allowing anyone to participate with mining hardware.

**Q: Is INTcoin compatible with existing crypto tools?**
A: INTcoin uses new post-quantum algorithms, so it requires its own tools and wallets. However, we support standard features like HD wallets and Lightning Network.

**Q: What's the token symbol?**
A: **INT** (INTcoin)

---

## License

Copyright (c) 2025 INTcoin Core (Maddison Lane)
Distributed under the MIT software license.

---

**INTcoin** • Quantum-Resistant • Decentralized • The Future is Secure

*Version 1.1 - November 2025*
