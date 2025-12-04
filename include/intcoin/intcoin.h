/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Core - Master Include Header
 */

#ifndef INTCOIN_H
#define INTCOIN_H

// Core types and utilities
#include "types.h"
#include "util.h"

// Cryptography
#include "crypto.h"

// Blockchain components
#include "script.h"
#include "transaction.h"
#include "block.h"
#include "blockchain.h"

// Consensus
#include "consensus.h"

// Network
#include "network.h"

// Storage
#include "storage.h"

// Machine Learning
#include "../ml/ml.h"

// RPC Server
#include "rpc.h"

// Wallet
#include "wallet.h"

// Mining
#include "mining.h"

// Mining Pool
#include "pool.h"

// Block Explorer
#include "explorer.h"

// Lightning Network
#include "lightning.h"

// Version information
#define INTCOIN_VERSION_MAJOR 1
#define INTCOIN_VERSION_MINOR 0
#define INTCOIN_VERSION_PATCH 0
#define INTCOIN_VERSION_SUFFIX "alpha"

#define INTCOIN_VERSION "1.0.0-alpha"
#define INTCOIN_COPYRIGHT "Copyright (c) 2025 INTcoin Team (Neil Adamson)"
#define INTCOIN_LICENSE "MIT License"

namespace intcoin {

/// Get version string
const char* GetVersion();

/// Get copyright string
const char* GetCopyright();

/// Get license string
const char* GetLicense();

/// Get build information
struct BuildInfo {
    const char* version;
    const char* git_commit;
    const char* build_date;
    const char* compiler;
    const char* platform;
};

BuildInfo GetBuildInfo();

} // namespace intcoin

#endif // INTCOIN_H
