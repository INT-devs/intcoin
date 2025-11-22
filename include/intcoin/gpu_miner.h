// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// GPU miner interface for CUDA and OpenCL mining

#ifndef INTCOIN_GPU_MINER_H
#define INTCOIN_GPU_MINER_H

#include "block.h"
#include "primitives.h"
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace intcoin {
namespace gpu {

/**
 * GPU device information
 */
struct GPUDevice {
    int device_id;
    std::string name;
    std::string vendor;
    size_t global_memory;      // bytes
    size_t local_memory;       // bytes
    uint32_t compute_units;
    uint32_t clock_frequency;  // MHz
    bool is_available;
    std::string platform;      // "CUDA" or "OpenCL"

    GPUDevice()
        : device_id(-1)
        , global_memory(0)
        , local_memory(0)
        , compute_units(0)
        , clock_frequency(0)
        , is_available(false)
    {}
};

/**
 * GPU mining statistics
 */
struct GPUMiningStats {
    uint64_t hashes_per_second;
    uint64_t total_hashes;
    uint64_t blocks_found;
    uint32_t temperature;          // Celsius (if available)
    uint32_t fan_speed;            // Percentage (if available)
    uint32_t power_usage;          // Watts (if available)
    double efficiency;             // Hashes per watt

    GPUMiningStats()
        : hashes_per_second(0)
        , total_hashes(0)
        , blocks_found(0)
        , temperature(0)
        , fan_speed(0)
        , power_usage(0)
        , efficiency(0.0)
    {}
};

/**
 * GPU mining configuration
 */
struct GPUConfig {
    uint32_t threads_per_block;    // CUDA: threads per block
    uint32_t blocks_per_grid;      // CUDA: blocks per grid
    uint32_t work_group_size;      // OpenCL: work group size
    uint32_t global_work_size;     // OpenCL: global work size
    bool enable_monitoring;        // Enable temp/fan/power monitoring
    uint32_t intensity;            // Mining intensity (1-31)

    GPUConfig()
        : threads_per_block(256)
        : blocks_per_grid(8192)
        , work_group_size(256)
        , global_work_size(2097152)  // 2M
        , enable_monitoring(true)
        , intensity(20)  // Default intensity
    {}
};

/**
 * Abstract base class for GPU miners
 */
class GPUMiner {
public:
    using BlockFoundCallback = std::function<void(const Block&, int device_id)>;

    virtual ~GPUMiner() = default;

    // Device management
    virtual bool initialize(int device_id = -1) = 0;
    virtual void shutdown() = 0;
    virtual std::vector<GPUDevice> list_devices() const = 0;

    // Mining control
    virtual bool start_mining(const BlockHeader& block_template,
                            const DilithiumPubKey& reward_address) = 0;
    virtual void stop_mining() = 0;
    virtual bool is_mining() const = 0;

    // Configuration
    virtual void set_config(const GPUConfig& config) = 0;
    virtual GPUConfig get_config() const = 0;

    // Statistics
    virtual GPUMiningStats get_stats(int device_id = 0) const = 0;
    virtual uint64_t get_hashrate(int device_id = 0) const = 0;

    // Callbacks
    virtual void set_block_found_callback(BlockFoundCallback cb) = 0;

    // Update block template (for when new transactions arrive or difficulty changes)
    virtual void update_block_template(const BlockHeader& new_template) = 0;

    // Platform name
    virtual std::string get_platform_name() const = 0;
};

/**
 * GPU Miner Factory
 */
class GPUMinerFactory {
public:
    enum class Platform {
        CUDA,
        OPENCL,
        AUTO  // Auto-detect best platform
    };

    // Create a GPU miner for the specified platform
    static std::unique_ptr<GPUMiner> create(Platform platform = Platform::AUTO);

    // Check if CUDA is available
    static bool is_cuda_available();

    // Check if OpenCL is available
    static bool is_opencl_available();

    // Get all available devices (CUDA + OpenCL)
    static std::vector<GPUDevice> get_all_devices();

    // Get recommended platform based on available hardware
    static Platform get_recommended_platform();
};

} // namespace gpu
} // namespace intcoin

#endif // INTCOIN_GPU_MINER_H
