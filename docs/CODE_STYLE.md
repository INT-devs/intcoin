# INTcoin C++ Code Style Guide
**Version**: 1.0.0-beta  
**Date**: December 26, 2025  
**Standard**: C++23 (ISO/IEC 14882:2023)

---

## Table of Contents

- [General Principles](#general-principles)
- [Naming Conventions](#naming-conventions)
- [Formatting](#formatting)
- [Code Organization](#code-organization)
- [Best Practices](#best-practices)
- [Modern C++ Features](#modern-c-features)
- [Error Handling](#error-handling)
- [Comments and Documentation](#comments-and-documentation)

---

## General Principles

### Philosophy

1. **Clarity over Cleverness**: Write code that is easy to understand, not code that shows off
2. **Consistency**: Follow existing patterns in the codebase
3. **Simplicity**: Prefer simple solutions over complex ones
4. **Safety**: Use modern C++ features to prevent bugs
5. **Performance**: Optimize where it matters, profile before optimizing

### Code Quality

- **Zero Warnings**: Code must compile without warnings (`-Wall -Wextra -Wpedantic`)
- **No Undefined Behavior**: Use sanitizers during development
- **Thread Safety**: Document thread-safety guarantees
- **const Correctness**: Use `const` wherever possible

---

## Naming Conventions

### Variables

Use `snake_case` for variables:

```cpp
// Good
int block_height;
std::string transaction_id;
uint64_t total_supply;
const double fee_rate = 0.001;

// Bad
int BlockHeight;
std::string transactionID;
uint64_t TotalSupply;
const double FeeRate = 0.001;
```

### Functions

Use `PascalCase` for public functions, `snake_case` for private:

```cpp
class BlockValidator {
public:
    // Public functions: PascalCase
    Result<bool> ValidateBlock(const Block& block);
    bool VerifyProofOfWork(const BlockHeader& header);
    
private:
    // Private functions: snake_case
    bool check_timestamp(uint64_t timestamp);
    Result<void> validate_transactions(const std::vector<Transaction>& txs);
};
```

### Classes and Structs

Use `PascalCase`:

```cpp
// Good
class BlockchainDB;
struct TransactionOutput;
class LightningChannel;

// Bad
class blockchain_db;
struct transaction_output;
class lightning_channel;
```

### Constants

Use `SCREAMING_SNAKE_CASE` for compile-time constants:

```cpp
// Good
constexpr uint64_t MAX_BLOCK_SIZE = 32 * 1024 * 1024;  // 32 MB
constexpr int DILITHIUM3_PUBLIC_KEY_SIZE = 1952;
constexpr double BLOCK_REWARD_HALVING = 0.5;

// Bad
const uint64_t maxBlockSize = 32 * 1024 * 1024;
constexpr int dilithium3PublicKeySize = 1952;
```

### Enums

Use `PascalCase` for enum class names, `PascalCase` for values:

```cpp
// Good
enum class BlockStatus {
    Valid,
    Invalid,
    Orphaned,
    PendingValidation
};

enum class NetworkMessageType {
    Version,
    VerAck,
    GetBlocks,
    Inventory,
    Transaction
};

// Bad
enum class block_status {
    VALID,
    INVALID
};
```

### Namespaces

Use `snake_case`:

```cpp
namespace intcoin {
namespace blockchain {
namespace validation {
    // ...
}
}
}
```

### File Names

Use `snake_case` matching the primary class:

```cpp
// Files
blockchain.h / blockchain.cpp     // For Blockchain class
transaction.h / transaction.cpp   // For Transaction class
lightning_channel.h / .cpp        // For LightningChannel class
```

---

## Formatting

### Indentation

- **Use 4 spaces** (no tabs)
- Indent namespaces and class contents

```cpp
namespace intcoin {

class Block {
public:
    Block();
    
    void AddTransaction(const Transaction& tx) {
        if (tx.IsValid()) {
            transactions_.push_back(tx);
        }
    }
    
private:
    std::vector<Transaction> transactions_;
};

}  // namespace intcoin
```

### Braces

Use **Allman style** (braces on new line) for functions, **K&R style** for control flow:

```cpp
// Functions: Allman style
void ProcessBlock(const Block& block)
{
    // Function body
}

// Control flow: K&R style
if (condition) {
    // ...
} else {
    // ...
}

for (const auto& tx : transactions) {
    // ...
}

while (running) {
    // ...
}
```

### Line Length

- **Maximum 100 characters** per line
- Break long lines at logical points

```cpp
// Good
Result<Block> GetBlockByHash(
    const Hash256& hash,
    bool include_transactions = true
);

// Break long function calls
auto result = blockchain.ValidateTransaction(
    transaction,
    current_height,
    utxo_set,
    ValidationFlags::CheckSignatures | ValidationFlags::CheckInputs
);

// Bad (too long)
auto result = blockchain.ValidateTransaction(transaction, current_height, utxo_set, ValidationFlags::CheckSignatures | ValidationFlags::CheckInputs);
```

### Spacing

```cpp
// Operators: spaces around binary operators
int sum = a + b;
bool valid = (x == y) && (z != 0);
amount *= multiplier;

// Function calls: no space before parenthesis
DoSomething(arg1, arg2);

// Control flow: space before parenthesis
if (condition) {
    // ...
}

for (int i = 0; i < count; ++i) {
    // ...
}

// Pointer/reference: attach to type
int* ptr = nullptr;
const std::string& name = get_name();

// Template: no space before angle bracket
std::vector<Transaction> transactions;
Result<std::optional<Block>> block;
```

---

## Code Organization

### Header Files

**Header Guard**: Use `#pragma once`

```cpp
#pragma once

#include <vector>
#include <string>

namespace intcoin {

class Blockchain {
public:
    // Public interface
    
private:
    // Private members
};

}  // namespace intcoin
```

**Include Order**:
1. Corresponding header (for .cpp files)
2. C system headers
3. C++ standard library headers
4. Third-party library headers
5. Project headers

```cpp
// blockchain.cpp
#include "blockchain.h"         // 1. Corresponding header

#include <cstdint>              // 2. C system headers
#include <cstring>

#include <vector>               // 3. C++ standard library
#include <string>
#include <memory>

#include <boost/filesystem.hpp> // 4. Third-party libraries
#include <rocksdb/db.h>

#include "transaction.h"        // 5. Project headers
#include "storage.h"
```

### Class Organization

**Order**: Public before private, grouped by functionality

```cpp
class Example {
public:
    // 1. Type aliases and nested types
    using TransactionList = std::vector<Transaction>;
    
    // 2. Constructors and destructor
    Example();
    explicit Example(int value);
    ~Example();
    
    // 3. Deleted/defaulted special members
    Example(const Example&) = delete;
    Example& operator=(const Example&) = delete;
    Example(Example&&) = default;
    Example& operator=(Example&&) = default;
    
    // 4. Main interface methods
    void ProcessTransaction(const Transaction& tx);
    Result<Block> GetBlock(uint64_t height);
    
    // 5. Getters and setters
    int GetValue() const { return value_; }
    void SetValue(int value) { value_ = value; }
    
protected:
    // Protected members (for inheritance)
    
private:
    // 6. Private methods
    void initialize();
    bool validate_state() const;
    
    // 7. Member variables (always last, with trailing underscore)
    int value_;
    std::shared_ptr<Database> db_;
    mutable std::mutex mutex_;
};
```

### Member Variables

**Always use trailing underscore** for private member variables:

```cpp
class Wallet {
private:
    std::vector<Address> addresses_;
    uint64_t balance_;
    std::shared_ptr<BlockchainDB> db_;
    mutable std::mutex mutex_;
};
```

---

## Best Practices

### Use RAII

Manage resources with constructors/destructors:

```cpp
// Good
class DatabaseConnection {
public:
    DatabaseConnection(const std::string& path) {
        db_ = rocksdb::DB::Open(options, path, &db_);
    }
    
    ~DatabaseConnection() {
        delete db_;  // Automatic cleanup
    }
    
private:
    rocksdb::DB* db_;
};

// Use
{
    DatabaseConnection conn("/path/to/db");
    conn.Query(...);
}  // Automatically closed
```

### Prefer Smart Pointers

```cpp
// Good
std::unique_ptr<Block> block = std::make_unique<Block>();
std::shared_ptr<Blockchain> blockchain = std::make_shared<Blockchain>();

// Bad
Block* block = new Block();  // Manual memory management
delete block;
```

### Use const Correctness

```cpp
// Good
class Transaction {
public:
    Hash256 GetHash() const;  // Doesn't modify object
    bool Verify(const PublicKey& key) const;
    
    void AddInput(const TxInput& input);  // Modifies object
    
private:
    Hash256 hash_;
    mutable std::optional<Hash256> cached_hash_;  // Can be modified in const methods
};
```

### Avoid Raw Loops

Use algorithms and range-based for:

```cpp
// Good
for (const auto& tx : transactions) {
    ProcessTransaction(tx);
}

auto it = std::find_if(blocks.begin(), blocks.end(),
                       [hash](const Block& b) { return b.GetHash() == hash; });

std::transform(inputs.begin(), inputs.end(),
               std::back_inserter(outputs),
               [](const Input& in) { return in.ToOutput(); });

// Acceptable for simple cases
for (size_t i = 0; i < transactions.size(); ++i) {
    ProcessTransaction(transactions[i], i);
}
```

### Initialize Members

Use member initializer lists:

```cpp
// Good
class Block {
public:
    Block(uint64_t height, const Hash256& prev_hash)
        : height_(height),
          previous_hash_(prev_hash),
          timestamp_(std::time(nullptr)),
          nonce_(0)
    {}
    
private:
    uint64_t height_;
    Hash256 previous_hash_;
    uint64_t timestamp_;
    uint64_t nonce_;
};
```

---

## Modern C++ Features

### Use auto Appropriately

```cpp
// Good: Type is obvious
auto blockchain = std::make_shared<Blockchain>();
auto it = blocks.begin();
for (const auto& [key, value] : map) {
    // Structured bindings
}

// Good: Avoid repeating long type names
auto transaction = CreateTransaction(inputs, outputs);

// Bad: Type unclear
auto x = GetValue();  // What type is x?

// Better: Explicit type
Transaction x = GetValue();
```

### Use Structured Bindings (C++17)

```cpp
// Good
for (const auto& [address, balance] : wallet.GetBalances()) {
    std::cout << address << ": " << balance << "\n";
}

auto [success, block] = blockchain.GetBlock(height);
if (success) {
    ProcessBlock(block);
}
```

### Use std::optional

```cpp
// Good
std::optional<Block> FindBlock(const Hash256& hash) {
    if (auto it = blocks.find(hash); it != blocks.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Usage
if (auto block = FindBlock(hash)) {
    ProcessBlock(*block);
}
```

### Use std::variant

```cpp
// Good for type-safe unions
using ScriptPubKey = std::variant<
    P2PKH,
    P2SH,
    P2WPKH,
    P2WSH
>;

std::visit([](auto&& script) {
    using T = std::decay_t<decltype(script)>;
    if constexpr (std::is_same_v<T, P2PKH>) {
        // Handle P2PKH
    }
}, script_pubkey);
```

### Use Concepts (C++20)

```cpp
// Define concepts
template<typename T>
concept Hashable = requires(T a) {
    { a.GetHash() } -> std::same_as<Hash256>;
};

template<typename T>
concept Serializable = requires(T a) {
    { a.Serialize() } -> std::same_as<std::vector<uint8_t>>;
    { T::Deserialize(std::vector<uint8_t>{}) } -> std::same_as<Result<T>>;
};

// Use concepts
template<Hashable T>
Hash256 ComputeMerkleRoot(const std::vector<T>& items);
```

---

## Error Handling

### Use Result<T> Monad

**Never use exceptions** in consensus-critical code:

```cpp
// Good
Result<Block> ParseBlock(const std::vector<uint8_t>& data) {
    if (data.size() < MIN_BLOCK_SIZE) {
        return Result<Block>::Error("Block too small");
    }
    
    Block block;
    // Parse block...
    
    if (!block.IsValid()) {
        return Result<Block>::Error("Invalid block");
    }
    
    return Result<Block>::Ok(block);
}

// Usage
auto result = ParseBlock(data);
if (result.IsOk()) {
    ProcessBlock(result.value);
} else {
    LogError(result.error);
}
```

### Check Return Values

```cpp
// Good
auto result = blockchain.AddBlock(block);
if (!result.IsOk()) {
    HandleError(result.error);
    return;
}

// Bad
blockchain.AddBlock(block);  // Ignoring return value
```

---

## Comments and Documentation

### Doxygen Comments

Use Doxygen style for public APIs:

```cpp
/**
 * @brief Validates a block according to consensus rules
 *
 * Performs comprehensive validation including:
 * - Proof-of-work verification
 * - Transaction validation
 * - Merkle root verification
 * - Timestamp checks
 *
 * @param block The block to validate
 * @param height The expected block height
 * @return Result<void> Ok if valid, Error with reason otherwise
 *
 * @note This function is consensus-critical. Changes require
 *       careful review and extensive testing.
 *
 * @warning Do not modify without understanding the full
 *          consensus implications.
 */
Result<void> ValidateBlock(const Block& block, uint64_t height);
```

### Inline Comments

```cpp
// Good: Explain WHY, not WHAT
// Use Dilithium3 for post-quantum security
constexpr int SIGNATURE_SIZE = 2420;

// Prevent time-warp attacks
if (timestamp > current_time + MAX_FUTURE_BLOCK_TIME) {
    return Result<void>::Error("Block timestamp too far in future");
}

// Bad: Stating the obvious
// Increment i by 1
i++;
```

### TODOs

```cpp
// TODO(username): Brief description of what needs to be done
// TODO(alice): Implement batch signature verification for performance
// FIXME(bob): This calculation overflows for large values
// NOTE(charlie): This must remain compatible with version 1.0
```

---

## File Templates

### Header File Template

```cpp
#pragma once

#include <vector>
#include <memory>

namespace intcoin {

/**
 * @brief Brief description of the class
 *
 * Detailed description explaining the purpose,
 * usage, and any important considerations.
 */
class MyClass {
public:
    /// Constructor
    MyClass();
    
    /// Destructor
    ~MyClass();
    
    // Public interface
    
private:
    // Private implementation
    
    // Member variables
};

}  // namespace intcoin
```

### Implementation File Template

```cpp
#include "my_class.h"

#include <algorithm>
#include <stdexcept>

namespace intcoin {

MyClass::MyClass()
    : member_var_(0)
{
    // Constructor implementation
}

MyClass::~MyClass() = default;

// Method implementations

}  // namespace intcoin
```

---

## Tools

### clang-format

Use the provided `.clang-format` configuration:

```bash
# Format single file
clang-format -i src/blockchain/blockchain.cpp

# Format all files
find src include -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

### clang-tidy

Static analysis:

```bash
clang-tidy src/blockchain/*.cpp -- -Iinclude
```

### Sanitizers

```bash
# Address Sanitizer (memory errors)
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=address"

# Thread Sanitizer (race conditions)
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=thread"

# Undefined Behavior Sanitizer
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
```

---

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Modern C++ Best Practices](https://github.com/cpp-best-practices)

---

**Maintainer**: INTcoin Development Team  
**Contact**: team@international-coin.org  
**Last Updated**: December 26, 2025
