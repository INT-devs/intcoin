# Security Policy

## Supported Versions

Currently supported versions of INTcoin:

| Version | Supported          |
| ------- | ------------------ |
| 1.4.x   | :white_check_mark: |
| 1.3.x   | :white_check_mark: |
| 1.2.x   | :white_check_mark: |
| 1.1.x   | :x:                |
| 1.0.x   | :x:                |
| < 1.0   | :x:                |

---

## Reporting a Vulnerability

We take the security of INTcoin seriously. If you have discovered a security vulnerability, we appreciate your help in disclosing it to us in a responsible manner.

### Security Contact

**Neil Adamson**
- **Email**: security@international-coin.org
- **PGP Fingerprint**: `93C7 3090 AD8C 404D 4786  7CE7 D100 3736 1A73 8BE6`
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
- Invited to participate in the community project development

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
| Smart Contracts | Pending | - |

Professional security audits are planned for Q1 2026.

---

## Vulnerability Disclosure Timeline

No vulnerabilities have been publicly disclosed at this time.

---

## PGP Public Key

To verify signatures or encrypt communications, use Neil Adamson's PGP public key:

**Fingerprint**: `93C7 3090 AD8C 404D 4786  7CE7 D100 3736 1A73 8BE6`

Download from:
- **Official Website**: https://international-coin.org/security.asc
- **Keyserver**: `gpg --recv-keys 93C73090AD8C404D47867CE7D10037361A738BE6`

---

## v1.2.0-beta Security Considerations

### Mobile Wallet Security

**SPV Security Model**:
- SPV clients trust that the longest proof-of-work chain is valid
- Merkle proofs verify transaction inclusion but not validity
- SPV clients rely on full nodes for transaction validity
- **Risk**: Potential for eclipse attacks if all connected peers are malicious
- **Mitigation**: Connect to multiple diverse peers, use trusted full node connections

**Bloom Filter Privacy**:
- Bloom filters leak some privacy information through false positive patterns
- **Risk**: Sophisticated attackers may correlate transactions to addresses
- **Mitigation**: Use larger filters (higher false positive rate), rotate filters periodically
- Note: Bloom filter use is optional; users can run full nodes for maximum privacy

**Mobile Key Storage**:
- **iOS**: Keys must be stored in iOS Keychain with `kSecAttrAccessibleWhenUnlockedThisDeviceOnly`
- **Android**: Use EncryptedSharedPreferences or Android Keystore for key storage
- **Never** store mnemonic or private keys in plain text or shared preferences
- Enable biometric authentication for additional security layer

**Mobile RPC Security**:
- Mobile RPC endpoints should be accessed over TLS/SSL
- Implement certificate pinning in production mobile apps
- Use authentication tokens for RPC access
- Rate limit mobile RPC to prevent abuse

### Atomic Swap Security

**HTLC Security**:
- HTLCs are cryptographically secure if preimage remains secret until claim
- **Risk**: Preimage revelation before both HTLCs are locked can lead to fund loss
- **Mitigation**: Protocol enforces strict state machine, automated monitoring

**Timeout Considerations**:
- Initiator timeout: 48 hours (must be longer than participant timeout)
- Participant timeout: 24 hours
- **Critical**: Always claim before timeout or funds may be lost to refund
- **Mitigation**: Automated monitoring alerts for expiring HTLCs

**Exchange Rate Volatility**:
- Exchange rates may change between swap initiation and completion
- **Risk**: Price slippage during swap execution
- **Mitigation**: Set acceptable price range, short swap windows, automated monitoring

**Chain Reorganization**:
- Deep reorgs on either chain could invalidate HTLCs
- **Mitigation**: Wait for sufficient confirmations before finalizing (BTC: 6, LTC: 24)

### Cross-Chain Bridge Security

**Multi-Sig Validation**:
- All bridge transactions require M-of-N validator signatures
- Current: 3-of-5 validators required for testnet
- Mainnet: Will increase to 7-of-11 or higher
- **Risk**: Validator collusion could compromise bridge
- **Mitigation**: Diverse, reputable validator set; ongoing reputation monitoring

**Emergency Pause Mechanism**:
- Circuit breaker can halt bridge operations if anomalies detected
- Activated by majority validator vote or automated detection
- **Risk**: Malicious validators could DOS bridge with false pause
- **Mitigation**: High threshold for emergency pause, transparent override process

**Supply Consistency**:
- Total wrapped token supply must always equal locked assets
- Automated verification every 100 blocks
- **Risk**: Minting bug could create unbacked tokens
- **Mitigation**: Multi-layer validation, public audit trail, insurance fund

**Validator Key Management**:
- Validators must use HSM (Hardware Security Module) or equivalent for signing keys
- Hot/cold wallet separation for large value operations
- Regular key rotation and backup procedures
- **Critical**: Validator key compromise could enable unauthorized minting

**Deposit/Withdrawal Limits**:
- Initial limits: 10 BTC / 100 ETH / 1000 LTC per transaction (testnet)
- Larger transactions require additional confirmation time
- **Risk**: Large value transactions increase attack incentive
- **Mitigation**: Graduated limits, time locks on large withdrawals

### Enhanced Mempool Security

**Priority System**:
- CRITICAL and BRIDGE priority levels reserved for protocol transactions
- **Risk**: Attackers could spam high-priority transactions
- **Mitigation**: Require higher fees for CRITICAL priority, rate limiting per address

**Mempool Persistence**:
- Mempool saved to `mempool.dat` on shutdown
- **Risk**: Malicious mempool.dat could crash node on restart
- **Mitigation**: Integrity checks on load, size limits, validation before acceptance

**DoS Protection**:
- Transaction size limits prevent memory exhaustion
- Low-fee transactions evicted when mempool full
- **Mitigation**: Min relay fee, max mempool size (300 MB), orphan tx limits

### Prometheus Metrics Security

**Metrics Endpoint Exposure**:
- Default configuration binds to 127.0.0.1:9090 (localhost only)
- **Risk**: Exposing metrics publicly reveals node information (peers, txs, etc.)
- **Mitigation**: Use firewall rules, reverse proxy with authentication, VPN access

**No Built-in Authentication**:
- Metrics HTTP endpoint has no authentication by default
- **Risk**: Unauthorized access if exposed
- **Mitigation**: Use reverse proxy (nginx/apache) with HTTP basic auth, or mTLS

**Information Disclosure**:
- Metrics reveal operational details (block height, peer count, mempool size)
- **Risk**: Information useful for targeted attacks
- **Mitigation**: Limit external access, monitor for unusual scraping patterns

---

## v1.4.0-beta Security Considerations

### Smart Contracts Security (IntSC VM)

**Quantum-Resistant Signatures**:
- All contract transactions (deployment and calls) require Dilithium3 signatures
- Signature size: 3,309 bytes (provides post-quantum security)
- **Protection**: Future-proof against quantum computer attacks
- **Validation**: 11,206 signature validations per second (measured on Apple Silicon)
- **Critical**: Never reuse contract deployment addresses; nonces ensure deterministic uniqueness

**Gas Limit Enforcement**:
- **Block Gas Limit**: 30,000,000 gas per block
- **Mempool Gas Limit**: 60,000,000 gas (2 blocks worth)
- **Transaction Gas Limit**: Maximum 30M gas per transaction
- **Minimum Gas**: 32,000 + 200 per byte (deployments), 21,000 (calls)
- **Risk**: Infinite loops or resource-intensive contracts could DoS validators
- **Mitigation**: Strict gas metering prevents infinite execution; gas limits enforced at mempool, block validation, and execution levels

**Nonce Validation and Replay Prevention**:
- Sequential nonce requirement per contract address
- Nonce tracking in mempool prevents replay attacks
- **Risk**: Nonce reuse could enable transaction replay
- **Mitigation**: Mempool rejects old nonces; future nonces held until sequential; atomic nonce increment on execution
- **Critical**: Users must manage nonces carefully when sending multiple transactions

**Contract Bytecode Size Limits**:
- Maximum contract size: 24 KB (24,576 bytes) - EVM standard
- **Risk**: Oversized contracts could consume excessive storage
- **Mitigation**: Validation rejects contracts exceeding size limit; encourages modular contract design

**Replace-By-Fee (RBF) Security**:
- RBF requires 10% minimum gas price increase for replacement
- **Risk**: Transaction spam through repeated low-cost replacements
- **Mitigation**: 10% threshold prevents spam; mempool tracks gas price per nonce
- **User Impact**: Allows transaction acceleration during network congestion

**Contract Execution Isolation**:
- Each contract executes in isolated VM context
- Gas tracking prevents runaway execution
- **Risk**: Malicious contracts could attempt to exploit VM
- **Mitigation**: EVM-compatible opcode validation; bounded execution; no access to host system
- **Critical**: IntSC VM implements 60+ standard opcodes + 4 PQC opcodes

**Event Log Storage**:
- Event logs indexed by block number and transaction hash
- Topic-based filtering (up to 4 indexed topics)
- **Risk**: Excessive event emission could consume storage
- **Mitigation**: Gas cost per log emission; database compaction; efficient indexing

**Contract Address Determinism**:
- Contract addresses generated via SHA3-256(deployer || nonce)
- Bech32 "int1" prefix with error detection
- **Risk**: Address collision (theoretical with SHA3-256)
- **Mitigation**: Cryptographically negligible collision probability; nonce ensures uniqueness per deployer
- **Benefit**: Deterministic addresses enable off-chain address prediction

**State Rollback During Reorgs**:
- Contract state (balance, nonce, storage) correctly rolls back during chain reorganizations
- Event logs managed during block disconnect/reconnect
- **Risk**: Deep reorgs could temporarily invalidate contract states
- **Mitigation**: Atomic batch operations ensure consistency; wait for confirmations on critical operations
- **Validation**: 6 reorg tests confirm correct rollback behavior

**Smart Contract Audit Recommendations**:

For developers deploying contracts on INTcoin:

1. **Audit Before Deployment**:
   - Review contract logic for common vulnerabilities (reentrancy, overflow, etc.)
   - Test extensively on testnet before mainnet deployment
   - Use established Solidity patterns and libraries

2. **Gas Optimization**:
   - Optimize gas usage to reduce transaction costs
   - Avoid unbounded loops
   - Use efficient data structures

3. **Event Logging**:
   - Emit events for all state changes
   - Index important parameters for efficient querying
   - Keep event data concise

4. **Access Control**:
   - Implement proper access control for administrative functions
   - Use multi-sig for critical operations
   - Consider time locks for upgrades

5. **Emergency Procedures**:
   - Implement pause mechanisms for critical bugs
   - Plan for contract upgrades or migrations
   - Maintain insurance or reserve funds

**RPC Security for Contracts**:
- 12 new contract RPC methods require authentication
- Parameter validation on all inputs
- Gas estimation prevents underestimation attacks
- **Risk**: Unauthorized contract deployment/execution
- **Mitigation**: Enable `rpcauth` for all production nodes; rate limit contract RPC calls
- **Recommendation**: Use dedicated RPC nodes for contract interaction; firewall port 8332

**Contract Database Security**:
- RocksDB backend with atomic batch operations
- Contract storage isolated per address
- **Performance**: 2.5M reads/sec, 242K writes/sec
- **Risk**: Database corruption could lose contract state
- **Mitigation**: Regular backups; database checksums; atomic commits prevent partial writes
- **Critical**: Ensure sufficient disk space for contract storage growth

**Known Attack Vectors**:

1. **Gas Griefing**: Attacker deploys contracts that consume maximum gas without useful work
   - **Mitigation**: High gas costs disincentivize; min gas price enforcement

2. **Storage Bloat**: Attacker creates many contracts to consume storage
   - **Mitigation**: Deployment requires transaction fees; 24 KB size limit per contract

3. **Nonce Manipulation**: Attacker attempts to replay old transactions
   - **Mitigation**: Sequential nonce validation; mempool rejects duplicate nonces

4. **Signature Malleability**: Attacker modifies signature to create new transaction hash
   - **Mitigation**: Dilithium3 signatures are non-malleable by design

**Recommended Security Practices for Contract Developers**:

1. Use latest Solidity compiler with optimizer enabled
2. Implement checks-effects-interactions pattern
3. Avoid delegatecall to untrusted contracts
4. Use SafeMath equivalents for arithmetic
5. Test with maximum gas limits to verify execution bounds
6. Monitor deployed contracts for unusual activity
7. Plan for emergency pause and upgrade mechanisms
8. Consider quantum-resistant key management for contract ownership

---

## Contact

For security-related questions that are not vulnerability reports:
- **General Inquiries**: admin@international-coin.org
- **Security Team**: security@international-coin.org (PGP encrypted preferred)

---

*Last Updated: January 9, 2026 - v1.4.0-beta*
