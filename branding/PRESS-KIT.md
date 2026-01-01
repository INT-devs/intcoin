# INTcoin Press Kit

**Version**: 1.0
**Last Updated**: January 2025
**Copyright**: © 2025 INTcoin Core (Neil Adamson)

---

## Quick Facts

| Attribute | Details |
|-----------|---------|
| **Name** | INTcoin |
| **Ticker** | INT |
| **Category** | Cryptocurrency, Blockchain, Post-Quantum Cryptography |
| **Founded** | 2025 |
| **License** | MIT (Open Source) |
| **Creator** | Neil Adamson |
| **Website** | https://international-coin.org |
| **Repository** | https://github.com/INT-devs/intcoin |

---

## Elevator Pitch

**30-Second Version:**
> INTcoin is the first quantum-resistant cryptocurrency using NIST-standardized post-quantum algorithms (CRYSTALS-Dilithium5 and Kyber1024). Unlike Bitcoin and Ethereum, which will be vulnerable to quantum computers within the next decade, INTcoin provides Level 5 security that protects your funds today and tomorrow.

**60-Second Version:**
> INTcoin is a next-generation cryptocurrency designed to resist attacks from quantum computers. While traditional cryptocurrencies like Bitcoin use ECDSA signatures that can be broken by Shor's algorithm on a quantum computer, INTcoin uses CRYSTALS-Dilithium5 and Kyber1024—NIST-standardized post-quantum algorithms with the highest security level (Level 5).
>
> Beyond quantum resistance, INTcoin maintains true decentralization through ASIC-resistant mining, ensuring that ordinary users can participate in network security. With Lightning Network support for instant payments, TOR integration for privacy, and a pure Proof-of-Work consensus (no staking or governance), INTcoin represents the future of secure, decentralized currency.

---

## The Problem

### Quantum Computing Threatens Cryptocurrency

- **Current State**: Bitcoin, Ethereum, and most cryptocurrencies use ECDSA (Elliptic Curve Digital Signature Algorithm) for transaction signing
- **The Threat**: Quantum computers running Shor's algorithm can break ECDSA in polynomial time
- **Timeline**: Cryptographically-relevant quantum computers expected by 2030-2035
- **Impact**: Trillions of dollars in cryptocurrency assets at risk
- **Industry Response**: Most projects have no quantum migration plan

### "Store now, decrypt later" attacks
Even if quantum computers aren't ready today, adversaries can:
1. Record encrypted blockchain data now
2. Decrypt it with quantum computers later
3. Steal private keys and funds retroactively

**INTcoin solves this problem from day one.**

---

## The Solution

### NIST-Standardized Post-Quantum Cryptography

INTcoin implements the highest level of post-quantum security:

| Component | Algorithm | Security Level | Status |
|-----------|-----------|----------------|--------|
| **Digital Signatures** | CRYSTALS-Dilithium5 | NIST Level 5 | ✅ Standardized 2024 |
| **Key Encapsulation** | CRYSTALS-Kyber1024 | NIST Level 5 | ✅ Standardized 2024 |
| **Hash Function** | SHA3-256 (Keccak) | 128-bit quantum | ✅ FIPS 202 |

### What is NIST Level 5?

NIST (National Institute of Standards and Technology) defines 5 security levels for post-quantum cryptography:
- **Level 1**: Equivalent to AES-128 (lowest)
- **Level 5**: Equivalent to AES-256 (highest)

**INTcoin uses Level 5 for maximum future-proofing.**

---

## Key Features

### 1. Quantum-Resistant Security
- CRYSTALS-Dilithium5 signatures (4595 bytes)
- CRYSTALS-Kyber1024 encryption (1568 bytes)
- SHA3-256 hashing (Keccak)
- Resistant to Shor's algorithm and Grover's algorithm

### 2. True Decentralization
- ASIC-resistant SHA-256 PoW mining (quantum era)
- No pre-mine, no ICO, no founder allocation
- No staking or on-chain governance
- Pure Proof of Work consensus

### 3. Lightning Network
- Instant payments (microsecond confirmation)
- Billions of transactions per second capacity
- Quantum-resistant payment channels
- HTLC support with onion routing

### 4. Privacy & Anonymity
- Pseudonymous transactions (like Bitcoin)
- TOR network integration
- Hidden service (.onion) support
- Optional Lightning Network privacy

### 5. Modern Technology
- C++23 codebase
- Cross-platform (Linux, macOS, FreeBSD, Windows)
- HD wallet with BIP39 mnemonic phrases
- Comprehensive test coverage (unit, functional, fuzz)

---

## Technical Specifications

### Network Parameters

```
Ticker:             INT
Unit:               INTcoin
Change Unit:        INTS
Max Supply:         221 Trillion INT
Block Time:         2 minutes (120 seconds)
Block Size:         4 MB
Consensus:          Proof of Work (PoW)
Mining Algorithm:   SHA-256 (ASIC-resistant in quantum era)
```

### Cryptography

```
Signatures:         CRYSTALS-Dilithium5 (NIST FIPS 204)
Key Exchange:       CRYSTALS-Kyber1024 (NIST FIPS 203)
Hash Function:      SHA3-256 (NIST FIPS 202)
Security Level:     NIST Level 5 (highest)
Address Format:     Base58Check
```

### Emission Schedule

```
Initial Reward:     105,113,636 INT
Halving Method:     50% every 4 years
Halving Interval:   1,051,200 blocks (~4 years)
Total Halvings:     64
Emission Period:    ~256 years
Final Supply:       ~221 Trillion INT
```

---

## Competitive Comparison

| Feature | INTcoin | Bitcoin | Ethereum | Monero |
|---------|---------|---------|----------|--------|
| **Quantum-Resistant** | ✅ Yes (Dilithium5) | ❌ No (ECDSA) | ❌ No (ECDSA) | ❌ No (EdDSA) |
| **NIST Standardized** | ✅ Yes (2024) | ❌ No | ❌ No | ❌ No |
| **ASIC-Resistant** | ✅ Yes | ❌ No | ⚠️ Partial | ✅ Yes (RandomX) |
| **Lightning Network** | ✅ Yes | ✅ Yes | ❌ No | ❌ No |
| **TOR Support** | ✅ Native | ⚠️ Optional | ⚠️ Optional | ✅ Native |
| **Pure PoW** | ✅ Yes | ✅ Yes | ❌ No (PoS) | ✅ Yes |
| **Security Level** | NIST L5 | Classical | Classical | Classical |

---

## Use Cases

### 1. Long-Term Store of Value
Perfect for individuals and institutions wanting cryptocurrency exposure with quantum-proof security for 50+ year time horizons.

### 2. Everyday Transactions
Lightning Network enables instant, low-cost payments for daily purchases—coffee, groceries, services.

### 3. International Remittances
Send money globally without intermediaries, high fees, or quantum vulnerability.

### 4. Privacy-Conscious Users
TOR integration and pseudonymous transactions for users valuing financial privacy.

### 5. Quantum-Era Businesses
Future-proof treasury holdings and payment systems for forward-thinking organizations.

---

## Timeline & Roadmap

### Phase 1: Foundation (Q4 2024 - Q1 2025) ✅
- ✅ Core blockchain implementation
- ✅ Quantum-resistant cryptography (Dilithium, Kyber)
- ✅ Wallet with HD key derivation
- ✅ P2P network protocol
- ✅ Mining implementation

### Phase 2: Advanced Features (Q1 - Q2 2025) 🔄
- ✅ Lightning Network integration
- ✅ TOR network support
- ✅ Comprehensive testing (unit, functional, fuzz)
- ✅ Mining pool support
- 🔄 Smart contract VM
- 🔄 Cross-chain bridges (Bitcoin, Ethereum)

### Phase 3: Testnet & Testing (Q2 - Q3 2025) 🎯
- 🎯 Public testnet launch
- 🎯 Community testing and feedback
- 🎯 Block explorer
- 🎯 Mobile wallet development

### Phase 4: Mainnet Launch (Q3 2025) 🚀
- 🚀 Mainnet genesis block
- 🚀 Exchange listings
- 🚀 Mining pool deployment
- 🚀 Mobile wallet release (iOS, Android)

### Phase 5: Ecosystem Growth (Q4 2025+) 🌱
- Hardware wallet support (Ledger, Trezor)
- Payment processor integrations
- Merchant adoption program
- Atomic swaps with major cryptocurrencies
- Developer grants program

---

## Media Assets

### Logos

All logos available in SVG, PNG (1x, 2x, 3x), and WebP formats.

- **Primary Logo** ([logo.svg](logo.svg)) - 200x200px square with quantum shield
- **Horizontal Logo** ([logo-horizontal.svg](logo-horizontal.svg)) - 400x120px with wordmark
- **White Monochrome** ([logo-white.svg](logo-white.svg)) - For dark backgrounds
- **Black Monochrome** ([logo-black.svg](logo-black.svg)) - For light backgrounds

### Favicons

- 16x16, 32x32, 64x64, 128x128, 256x256, 512x512, 1024x1024

### Social Media Assets

- **Twitter/X Card** ([social-twitter-card.svg](social-twitter-card.svg)) - 1200x628px
- **LinkedIn Banner** ([social-linkedin-banner.svg](social-linkedin-banner.svg)) - 1584x396px
- **Open Graph Image** ([social-og-image.svg](social-og-image.svg)) - 1200x630px

### Brand Colors

```
Quantum Blue:   #00d4ff  (Primary accent, quantum effects)
Deep Purple:    #7c3aed  (Secondary accent, security)
Dark Purple:    #4c1d95  (Dark accents, depth)
Electric Cyan:  #0084ff  (Gradients, interactive)
Violet:         #6b4ce6  (Gradients, decorative)
Dark Slate:     #1e293b  (Text, headings)
Slate Gray:     #64748b  (Body text)
```

---

## Quotes

### From the Creator

> "The quantum threat to cryptocurrency is not theoretical—it's inevitable. INTcoin represents a fundamental shift in how we think about long-term cryptographic security. We're not just building for today; we're building for the next century."
>
> **— Neil Adamson, Lead Developer, INTcoin Core**

### Technical

> "INTcoin is the only major cryptocurrency project implementing NIST Level 5 post-quantum cryptography from genesis. This isn't a migration plan—it's quantum security from day one."

> "While others talk about quantum resistance, INTcoin has already deployed CRYSTALS-Dilithium5 and Kyber1024, both freshly standardized by NIST in 2024."

---

## Frequently Asked Questions

### Q: Is INTcoin really quantum-resistant?
**A**: Yes. INTcoin uses CRYSTALS-Dilithium5 and Kyber1024, both NIST-standardized post-quantum algorithms achieving the highest security level (Level 5). These algorithms are designed to resist attacks from both classical and quantum computers.

### Q: When will quantum computers break Bitcoin?
**A**: Cryptanalytically-relevant quantum computers are expected between 2030-2035. A quantum computer with ~1 million qubits could break Bitcoin's ECDSA signatures in hours. INTcoin protects against this threat today.

### Q: Why not just upgrade Bitcoin to quantum-resistant crypto?
**A**: Migrating existing cryptocurrencies is extremely difficult:
- Signature size increases (65 bytes → 4595 bytes) impact blockchain size
- Address format changes break existing infrastructure
- Old addresses remain vulnerable forever (lost keys, early adopters)
- Consensus changes require network-wide coordination

INTcoin was designed quantum-resistant from the start, avoiding these migration issues.

### Q: Can I mine INTcoin?
**A**: Yes! INTcoin uses ASIC-resistant SHA-256 mining (in the quantum era), allowing anyone with a CPU or GPU to participate. No specialized hardware required.

### Q: What's the token distribution?
**A**: INTcoin has NO pre-mine, NO ICO, and NO founder allocation. 100% of coins are distributed through mining rewards over ~256 years. Completely fair launch.

### Q: Is INTcoin open source?
**A**: Yes. INTcoin is 100% open source under the MIT license. Anyone can review the code, contribute, or fork the project. No corporate control.

### Q: How does INTcoin compare to Bitcoin?
**A**: INTcoin builds on Bitcoin's proven PoW model but adds quantum resistance, ASIC resistance, Lightning Network, and TOR support. It's Bitcoin's security model, quantum-proofed for the next century.

---

## Statistics (As of January 2025)

- **Codebase**: 50,000+ lines of C++23
- **Test Coverage**: 400+ unit tests, fuzz testing, functional tests
- **Contributors**: Open to community contributions
- **Network Status**: Testnet in development

---

## Community & Social

- **Website**: https://international-coin.org
- **GitHub**: https://github.com/INT-devs/intcoin
- **Email**: team@international-coin.org
- **X (Twitter)**: https://x.com/INTcoin_team
- **Discord**: https://discord.gg/7p4VmS2z
- **Reddit**: r/INTcoin
- **Telegram**: https://t.me/INTcoin_official

---

## Media Contact

For press inquiries, interviews, or additional information:

**Email**: team@international-coin.org
**GPG Key**: `93C7 3090 AD8C 404D 4786  7CE7 D100 3736 1A73 8BE6`

**Response Time**: Within 24 hours for media inquiries

---

## Boilerplate

### Short (50 words)
INTcoin is a quantum-resistant cryptocurrency using NIST-standardized post-quantum cryptography (CRYSTALS-Dilithium5 and Kyber1024). Unlike Bitcoin and Ethereum, INTcoin provides Level 5 security against both classical and quantum computers, ensuring long-term protection for digital assets in the quantum era.

### Medium (100 words)
INTcoin is a next-generation cryptocurrency designed to resist attacks from quantum computers. Using CRYSTALS-Dilithium5 and Kyber1024—NIST-standardized post-quantum algorithms with the highest security level (Level 5)—INTcoin protects your funds today and tomorrow. Beyond quantum resistance, INTcoin maintains true decentralization through ASIC-resistant mining, supports instant Lightning Network payments, and integrates TOR for privacy. With a pure Proof-of-Work consensus, no pre-mine, and 100% open-source MIT license, INTcoin represents the future of secure, decentralized currency built for the quantum era.

### Long (200 words)
INTcoin is a quantum-resistant cryptocurrency that addresses the existential threat quantum computers pose to blockchain security. While traditional cryptocurrencies like Bitcoin and Ethereum use ECDSA signatures that can be broken by Shor's algorithm on a quantum computer, INTcoin employs CRYSTALS-Dilithium5 and Kyber1024—NIST-standardized post-quantum algorithms achieving the highest security level (Level 5).

Founded in 2025 by developer Neil Adamson, INTcoin was designed quantum-resistant from genesis, avoiding the complex migration challenges facing existing cryptocurrencies. The project implements ASIC-resistant SHA-256 proof-of-work mining to maintain decentralization, Lightning Network support for instant payments, and native TOR integration for privacy-conscious users.

INTcoin adheres to Bitcoin's proven security model: pure Proof of Work with no staking, governance, or pre-mine. The entire 221 trillion INT supply is distributed through mining rewards over approximately 256 years, with halvings every 4 years. Built with modern C++23 and comprehensive test coverage, INTcoin's open-source codebase (MIT license) ensures transparency and community-driven development.

With cryptographically-relevant quantum computers expected by 2030-2035, INTcoin provides the only proven path to long-term cryptographic security for digital assets. It's not just future-proof—it's quantum-proof.

---

## Technical Resources

### Documentation
- [Architecture Overview](../docs/architecture.md)
- [API Reference](../docs/api.md)
- [Mining Guide](../docs/mining.md)
- [Wallet Guide](../docs/wallet.md)
- [Lightning Network](../src/lightning/README.md)
- [TOR Integration](../docs/TOR-GUIDE.md)

### White Paper
- Coming Q2 2025

### Research Papers
- NIST FIPS 203: Module-Lattice-Based Key-Encapsulation Mechanism (Kyber)
- NIST FIPS 204: Module-Lattice-Based Digital Signature Algorithm (Dilithium)
- NIST FIPS 202: SHA-3 Standard (Keccak)

---

## Legal & Licensing

**Copyright**: © 2025 INTcoin Core (Neil Adamson)
**License**: MIT License
**Trademark**: INTcoin™ is a trademark of INTcoin Core

**Disclaimer**: INTcoin is experimental software. Use at your own risk. Cryptocurrency investments carry risk of loss.

---

## Version History

- **v1.0** (January 2025): Initial press kit release

---

**For the latest version of this press kit, visit**: https://international-coin.org/press-kit

**Last Updated**: January 2025
