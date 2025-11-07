# INTcoin Mining Pool Implementation

## Overview

This directory contains the implementation of mining pool functionality for INTcoin, including both server-side pool software and client-side pool connectivity.

## Features

### Stratum Protocol Support

- **Stratum V1**: Traditional Stratum protocol for wide compatibility
- **Stratum V2**: Enhanced protocol with improved efficiency (future)
- JSON-RPC based communication
- Bi-directional messaging

### Core Functionality

1. **Pool Server** (`stratum.h/cpp`)
   - Accept miner connections
   - Distribute mining jobs
   - Validate share submissions
   - Track worker statistics
   - Variable difficulty (vardiff) support
   - Block found notifications

2. **Pool Client** (`stratum_client.cpp`)
   - Connect to external pools
   - Receive mining jobs
   - Submit shares
   - Handle difficulty adjustments
   - Automatic reconnection

3. **Share Management**
   - Share validation
   - Difficulty calculation
   - Hashrate estimation
   - Statistics tracking

## Architecture

```
┌─────────────────┐
│  Miners         │
│  (Workers)      │
└────────┬────────┘
         │
         │ Stratum Protocol
         │
┌────────▼────────┐
│  Stratum Server │
│  - Job Dispatch │
│  - Share Valid  │
│  - Vardiff      │
└────────┬────────┘
         │
         │
┌────────▼────────┐
│  Blockchain     │
│  - Block Submit │
│  - Validation   │
└─────────────────┘
```

## Usage

### Running a Pool Server

```cpp
#include <intcoin/pool/stratum.h>

using namespace intcoin::pool;

int main() {
    // Create Stratum server on port 3333
    StratumServer server(3333, StratumVersion::V1);

    // Set up handlers
    server.set_share_handler([](const MiningShare& share) {
        // Validate and process share
        std::cout << "Share from " << share.worker_name << std::endl;
        return true;
    });

    server.set_block_found_handler([](const Block& block) {
        // Submit block to blockchain
        std::cout << "Block found!" << std::endl;
    });

    // Enable variable difficulty
    server.enable_vardiff(true);

    // Start server
    if (server.start()) {
        std::cout << "Pool server running on port 3333" << std::endl;

        // Keep running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Update mining job periodically
            MiningJob job;
            // ... populate job from blockchain
            server.set_new_job(job);
        }
    }

    return 0;
}
```

### Connecting to a Pool

```cpp
#include <intcoin/pool/stratum.h>

using namespace intcoin::pool;

int main() {
    // Connect to pool
    StratumClient client("pool.example.com", 3333,
                        "username.worker1", "password");

    // Set up job callback
    client.set_job_callback([](const MiningJob& job) {
        std::cout << "New job: " << job.job_id << std::endl;
        // Start mining this job
    });

    // Set up difficulty callback
    client.set_difficulty_callback([](double difficulty) {
        std::cout << "Difficulty: " << difficulty << std::endl;
    });

    // Connect
    if (client.connect()) {
        std::cout << "Connected to pool" << std::endl;

        // Mining loop
        while (client.is_connected()) {
            MiningJob job = client.get_current_job();

            // Mine and find share
            MiningShare share;
            // ... perform mining work

            // Submit share
            client.submit_share(share);
        }
    }

    return 0;
}
```

## Stratum Protocol Messages

### Client -> Server

**mining.subscribe**
```json
{
  "id": 1,
  "method": "mining.subscribe",
  "params": []
}
```

**mining.authorize**
```json
{
  "id": 2,
  "method": "mining.authorize",
  "params": ["username", "password"]
}
```

**mining.submit**
```json
{
  "id": 4,
  "method": "mining.submit",
  "params": ["username", "job_id", "extranonce2", "ntime", "nonce"]
}
```

### Server -> Client

**mining.notify** (new job)
```json
{
  "method": "mining.notify",
  "params": [
    "job_id",
    "prevhash",
    "coinb1",
    "coinb2",
    ["merkle_branch"],
    "version",
    "nbits",
    "ntime",
    true
  ]
}
```

**mining.set_difficulty**
```json
{
  "method": "mining.set_difficulty",
  "params": [2.0]
}
```

## Variable Difficulty (Vardiff)

The pool automatically adjusts difficulty for each worker to maintain a target share submission rate (default: 1 share per 15 seconds).

**Algorithm:**
1. Track worker share submission times
2. Calculate average time between shares
3. Adjust difficulty if deviation exceeds threshold
4. Send new difficulty to worker

**Configuration:**
```cpp
DifficultyCalculator::VardiffConfig config;
config.target_share_time = 15.0;     // 15 seconds per share
config.min_difficulty = 1.0;
config.max_difficulty = 1000000.0;
config.retarget_time = 60.0;         // Adjust every minute
config.variance_percent = 10.0;      // ±10% before adjusting
```

## Share Validation

Shares are validated by:
1. Checking job ID matches current/recent jobs
2. Verifying extranonce and nonce values
3. Reconstructing block header
4. Validating hash meets difficulty target
5. Checking for duplicate submissions

## Statistics

### Worker Statistics
- Shares accepted/rejected/stale
- Blocks found
- Estimated hashrate
- Acceptance rate
- Last seen time

### Pool Statistics
- Total hashrate
- Connected workers
- Total blocks found
- Total shares submitted
- Network difficulty

## Future Enhancements

### Planned Features

1. **Stratum V2**
   - More efficient binary protocol
   - Reduced bandwidth usage
   - Job negotiation
   - Mining device authentication

2. **Payment Processing**
   - PPLNS (Pay Per Last N Shares)
   - PPS (Pay Per Share)
   - PROP (Proportional)
   - Payment thresholds
   - Automatic payouts

3. **Database Integration**
   - PostgreSQL/MySQL support
   - Share storage
   - Worker history
   - Payment records

4. **Web Interface**
   - Real-time statistics
   - Worker management
   - Payment history
   - Block explorer integration

5. **Security**
   - DDoS protection
   - Rate limiting
   - Worker authentication
   - SSL/TLS support

6. **Monitoring**
   - Prometheus metrics
   - Grafana dashboards
   - Alert system
   - Performance monitoring

## Building

The pool implementation requires:
- C++23 compiler
- nlohmann/json library
- POSIX sockets (Unix/Linux/macOS)

Add to CMakeLists.txt:
```cmake
add_library(pool
    src/pool/stratum.cpp
    src/pool/stratum_client.cpp
)

target_link_libraries(pool
    PRIVATE nlohmann_json::nlohmann_json
)
```

## Testing

Unit tests for pool functionality:
```bash
./test_pool
```

Test with real miners:
```bash
# Start pool server
./intcoin-pool --port 3333

# Connect miner
./intcoin-miner --pool localhost:3333 --user worker1
```

## References

- [Stratum V1 Protocol](https://en.bitcoin.it/wiki/Stratum_mining_protocol)
- [Stratum V2 Specification](https://stratumprotocol.org/)
- [Mining Pool Implementation Guide](https://github.com/bitcoin/bips/blob/master/bip-0023.mediawiki)

## License

MIT License - See LICENSE file for details

## Author

INTcoin Core Development Team
