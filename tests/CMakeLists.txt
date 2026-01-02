# Test executables

# Find RocksDB
find_library(ROCKSDB_LIB rocksdb REQUIRED)
find_path(ROCKSDB_INCLUDE_DIR rocksdb/db.h REQUIRED)

include_directories(${ROCKSDB_INCLUDE_DIR})

# Test: Crypto
add_executable(test_crypto test_crypto.cpp)
target_link_libraries(test_crypto intcoin_core)

# Test: RandomX
add_executable(test_randomx test_randomx.cpp)
target_link_libraries(test_randomx intcoin_core ${ROCKSDB_LIB})

# Test: Bech32
add_executable(test_bech32 test_bech32.cpp)
target_link_libraries(test_bech32 intcoin_core)

# Test: Serialization
add_executable(test_serialization test_serialization.cpp)
target_link_libraries(test_serialization intcoin_core ${ROCKSDB_LIB})

# Test: Storage (RocksDB)
add_executable(test_storage test_storage.cpp)
target_link_libraries(test_storage intcoin_core ${ROCKSDB_LIB})

# Test: Validation
add_executable(test_validation test_validation.cpp)
target_link_libraries(test_validation intcoin_core ${ROCKSDB_LIB})

# Test: Genesis Block
add_executable(test_genesis test_genesis.cpp)
target_link_libraries(test_genesis intcoin_core ${ROCKSDB_LIB})

# Test: Network Protocol
add_executable(test_network test_network.cpp)
target_link_libraries(test_network intcoin_core ${ROCKSDB_LIB})

# Test: Machine Learning
add_executable(test_ml test_ml.cpp)
target_link_libraries(test_ml intcoin_core ${ROCKSDB_LIB})

# Test: Wallet (HD wallet, BIP39, BIP44)
add_executable(test_wallet test_wallet.cpp)
target_link_libraries(test_wallet intcoin_core ${ROCKSDB_LIB})

# Test: Fuzzing (robustness and edge cases)
add_executable(test_fuzz test_fuzz.cpp)
target_link_libraries(test_fuzz intcoin_core ${ROCKSDB_LIB})

# Test: Integration (end-to-end component interaction)
add_executable(test_integration test_integration.cpp)
target_link_libraries(test_integration intcoin_core ${ROCKSDB_LIB})

# Test: Lightning Network (channels, HTLCs, routing, invoices, watchtower)
add_executable(test_lightning test_lightning.cpp)
target_link_libraries(test_lightning intcoin_core ${ROCKSDB_LIB})

# Test: Atomic Swaps (cross-chain HTLCs, Bitcoin/Litecoin monitoring)
add_executable(test_atomic_swap test_atomic_swap.cpp)
target_link_libraries(test_atomic_swap intcoin_core ${ROCKSDB_LIB})

# Test: Bridge (cross-chain bridges, wrapped tokens, federated validators)
add_executable(test_bridge test_bridge.cpp)
target_link_libraries(test_bridge intcoin_core ${ROCKSDB_LIB})

# Test: Mempool (priority queues, persistence, eviction)
add_executable(test_mempool test_mempool.cpp)
target_link_libraries(test_mempool intcoin_core ${ROCKSDB_LIB})

# Test: Bloom Filter (SPV transaction filtering, BIP37)
add_executable(test_bloom test_bloom.cpp)
target_link_libraries(test_bloom intcoin_core ${ROCKSDB_LIB})

# Test: Metrics (Prometheus monitoring, analytics)
add_executable(test_metrics test_metrics.cpp)
target_link_libraries(test_metrics intcoin_core ${ROCKSDB_LIB})

# Test: Metrics Server (Prometheus HTTP endpoint)
add_executable(test_metrics_server test_metrics_server.cpp)
target_link_libraries(test_metrics_server intcoin_core ${ROCKSDB_LIB})

# Test: Mempool Analytics (Phase 1-2: real-time statistics, historical snapshots)
add_executable(test_mempool_analytics test_mempool_analytics.cpp)
target_link_libraries(test_mempool_analytics intcoin_core ${ROCKSDB_LIB})

# Test: Parallel Validation (Phase 1-2: multi-threaded block validation)
add_executable(test_parallel_validation test_parallel_validation.cpp)
target_link_libraries(test_parallel_validation intcoin_core ${ROCKSDB_LIB})

# Test: AssumeUTXO (Phase 1-2: UTXO snapshots for fast IBD)
add_executable(test_assumeutxo test_assumeutxo.cpp)
target_link_libraries(test_assumeutxo intcoin_core ${ROCKSDB_LIB})

# Test: Multi-Path Payments (Phase 4: MPP/AMP Lightning payments)
add_executable(test_multipath_payments lightning/test_multipath_payments.cpp)
target_link_libraries(test_multipath_payments intcoin_core ${ROCKSDB_LIB})

# Test: Watchtower (Phase 4: watchtower protocol V2)
add_executable(test_watchtower lightning/test_watchtower.cpp)
target_link_libraries(test_watchtower intcoin_core ${ROCKSDB_LIB})

# Test: Submarine Swaps (Phase 4: Lightning ⟷ on-chain swaps)
add_executable(test_submarine_swaps lightning/test_submarine_swaps.cpp)
target_link_libraries(test_submarine_swaps intcoin_core ${ROCKSDB_LIB})

# Test: Channel Rebalancing (Phase 4: auto-rebalancing, 6 strategies)
add_executable(test_channel_rebalancing lightning/test_channel_rebalancing.cpp)
target_link_libraries(test_channel_rebalancing intcoin_core ${ROCKSDB_LIB})

# Test: Routing (Phase 4: advanced routing algorithms)
add_executable(test_routing lightning/test_routing.cpp)
target_link_libraries(test_routing intcoin_core ${ROCKSDB_LIB})

# Test: Network Explorer (Phase 4: Lightning network graph exploration)
add_executable(test_network_explorer lightning/test_network_explorer.cpp)
target_link_libraries(test_network_explorer intcoin_core ${ROCKSDB_LIB})

# Register tests with CTest
add_test(NAME CryptoTest COMMAND test_crypto)
add_test(NAME RandomXTest COMMAND test_randomx)
add_test(NAME Bech32Test COMMAND test_bech32)
add_test(NAME SerializationTest COMMAND test_serialization)
add_test(NAME StorageTest COMMAND test_storage)
add_test(NAME ValidationTest COMMAND test_validation)
add_test(NAME GenesisTest COMMAND test_genesis)
add_test(NAME NetworkTest COMMAND test_network)
add_test(NAME MLTest COMMAND test_ml)
add_test(NAME WalletTest COMMAND test_wallet)
add_test(NAME FuzzTest COMMAND test_fuzz)
add_test(NAME IntegrationTest COMMAND test_integration)
add_test(NAME LightningTest COMMAND test_lightning)
add_test(NAME AtomicSwapTest COMMAND test_atomic_swap)
add_test(NAME BridgeTest COMMAND test_bridge)
add_test(NAME MempoolTest COMMAND test_mempool)
add_test(NAME BloomTest COMMAND test_bloom)
add_test(NAME MetricsTest COMMAND test_metrics)
add_test(NAME MetricsServerTest COMMAND test_metrics_server)
add_test(NAME MempoolAnalyticsTest COMMAND test_mempool_analytics)
add_test(NAME ParallelValidationTest COMMAND test_parallel_validation)
add_test(NAME AssumeUTXOTest COMMAND test_assumeutxo)
add_test(NAME MultiPathPaymentsTest COMMAND test_multipath_payments)
add_test(NAME WatchtowerTest COMMAND test_watchtower)
add_test(NAME SubmarineSwapsTest COMMAND test_submarine_swaps)
add_test(NAME ChannelRebalancingTest COMMAND test_channel_rebalancing)
add_test(NAME RoutingTest COMMAND test_routing)
add_test(NAME NetworkExplorerTest COMMAND test_network_explorer)

# Install test executables (optional)
install(TARGETS
    test_crypto
    test_randomx
    test_bech32
    test_serialization
    test_storage
    test_validation
    test_genesis
    test_network
    test_ml
    test_wallet
    test_fuzz
    test_integration
    test_lightning
    test_atomic_swap
    test_bridge
    test_mempool
    test_bloom
    test_metrics
    test_metrics_server
    test_mempool_analytics
    test_parallel_validation
    test_assumeutxo
    test_multipath_payments
    test_watchtower
    test_submarine_swaps
    test_channel_rebalancing
    test_routing
    test_network_explorer
    DESTINATION bin/tests
)
