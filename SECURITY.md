# Security Policy

## Supported Versions

Currently supported versions of INTcoin:

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

---

## Reporting a Vulnerability

We take the security of INTcoin seriously. If you have discovered a security vulnerability, we appreciate your help in disclosing it to us in a responsible manner.

### Security Contact

**Neil Adamson**
- **Email**: security@international-coin.org
- **PGP Fingerprint**: `4E5A A9F8 A4C2 F245 E3FC  9381 BCFB 6274 A3AF EEE9`
- **GPG Public Key**: [Download](https://international-coin.org/security.asc)

### Reporting Process

1. **DO NOT** create a public GitHub/GitLab issue for security vulnerabilities
2. Email security@international-coin.org with details of the vulnerability
3. Encrypt your email using the PGP key above for sensitive information
4. Include the following information:
   - Type of vulnerability
   - Full paths of source file(s) related to the vulnerability
   - Location of the affected source code (tag/branch/commit or direct URL)
   - Step-by-step instructions to reproduce the issue
   - Proof-of-concept or exploit code (if possible)
   - Impact of the vulnerability, including how an attacker might exploit it

### Response Timeline

- **Initial Response**: Within 48 hours of report
- **Confirmation**: Within 5 business days
- **Fix Development**: Timeline varies based on severity
- **Public Disclosure**: After fix is deployed (coordinated with reporter)

### Recognition

Security researchers who responsibly disclose vulnerabilities will be:
- Acknowledged in our security advisories (unless anonymity is requested)
- Listed in our Hall of Fame (if desired)
- Considered for bug bounty rewards (when program is established)

---

## Security Best Practices

### For Users

1. **Wallet Security**
   - Always backup your 24-word recovery phrase on paper
   - Never share your recovery phrase with anyone
   - Store backups in secure, fireproof locations
   - Use wallet encryption with a strong passphrase
   - Never screenshot or digitally store your recovery phrase

2. **Node Security**
   - Keep your node software up to date
   - Use firewall rules to restrict RPC access
   - Enable RPC authentication (rpcuser/rpcpassword)
   - Use strong, unique passwords
   - Consider using Tor or I2P for network privacy

3. **Network Security**
   - Verify download signatures before installing
   - Use official sources for downloads
   - Keep your operating system updated
   - Use antivirus/antimalware software

### For Developers

1. **Code Security**
   - All code must pass security review before merge
   - Use modern C++23 features for memory safety
   - Enable compiler security flags (ASLR, stack protection)
   - Regular dependency audits
   - Static analysis with sanitizers

2. **Cryptography**
   - Never implement custom cryptography
   - Use established libraries (liboqs, OpenSSL)
   - Follow NIST post-quantum standards
   - Properly handle key material (zero memory after use)
   - Use secure random number generation

3. **Input Validation**
   - Validate all external inputs
   - Sanitize user-provided data
   - Use bounds checking
   - Prevent buffer overflows
   - Check for integer overflows

---

## Known Security Features

INTcoin implements multiple security layers:

### Post-Quantum Cryptography
- **Dilithium3**: NIST-standardized digital signatures
- **Kyber768**: Post-quantum key encapsulation
- **SHA3-256**: Quantum-resistant hashing

### Wallet Security
- **AES-256-GCM**: Wallet encryption with authenticated encryption
- **PBKDF2**: 100,000 iterations for key derivation
- **BIP32/BIP39**: HD wallet with mnemonic recovery
- **Bech32**: Error-detecting address format

### Network Security
- **Tor Support**: Onion routing for anonymity
- **I2P Support**: Garlic routing for privacy
- **TLS/SSL**: Encrypted RPC connections
- **Authentication**: HTTP Basic Auth for RPC

### Mining Security
- **RandomX**: ASIC-resistant PoW algorithm
- **Difficulty Adjustment**: Every 2016 blocks
- **Block Validation**: Comprehensive consensus rules

---

## Security Audit Status

| Component | Status | Last Audit |
|-----------|--------|------------|
| Core Consensus | Pending | - |
| Cryptography | Pending | - |
| Network Protocol | Pending | - |
| Wallet | Pending | - |
| RPC Interface | Pending | - |

Professional security audits are planned for Q1 2026.

---

## Vulnerability Disclosure Timeline

No vulnerabilities have been publicly disclosed at this time.

---

## PGP Public Key

To verify signatures or encrypt communications, use Neil Adamson's PGP public key:

**Fingerprint**: `4E5A A9F8 A4C2 F245 E3FC  9381 BCFB 6274 A3AF EEE9`

Download from:
- **Official Website**: https://international-coin.org/security.asc
- **Keyserver**: `gpg --recv-keys 4E5AA9F8A4C2F245E3FC9381BCFB6274A3AFEEE9`

---

## Contact

For security-related questions that are not vulnerability reports:
- **General Inquiries**: contact@international-coin.org
- **Security Team**: security@international-coin.org (PGP encrypted preferred)

---

*Last Updated: December 17, 2025*
