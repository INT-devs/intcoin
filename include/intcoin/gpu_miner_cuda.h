// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// CUDA GPU miner for NVIDIA GPUs

#ifndef INTCOIN_GPU_MINER_CUDA_H
#define INTCOIN_GPU_MINER_CUDA_H

#include "gpu_miner.h"
#include <cuda_runtime.h>
#include <vector>
#include <mutex>

namespace intcoin {
namespace gpu {

/**
 * CUDA GPU Miner for NVIDIA GPUs
 */
class CUDAMiner : public GPUMiner {
public:
    CUDAMiner();
    ~CUDAMiner() override;

    // Device management
    bool initialize(int device_id = -1) override;
    void shutdown() override;
    std::vector<GPUDevice> list_devices() const override;

    // Mining control
    bool start_mining(const BlockHeader& block_template,
                     const DilithiumPubKey& reward_address) override;
    void stop_mining() override;
    bool is_mining() const override { return mining_; }

    // Configuration
    void set_config(const GPUConfig& config) override;
    GPUConfig get_config() const override { return config_; }

    // Statistics
    GPUMiningStats get_stats(int device_id = 0) const override;
    uint64_t get_hashrate(int device_id = 0) const override;

    // Callbacks
    void set_block_found_callback(BlockFoundCallback cb) override {
        block_found_callback_ = cb;
    }

    // Update block template
    void update_block_template(const BlockHeader& new_template) override;

    // Platform name
    std::string get_platform_name() const override { return "CUDA"; }

private:
    struct CUDADevice {
        int device_id;
        cudaDeviceProp properties;
        cudaStream_t stream;
        void* d_block_header;      // Device memory for block header
        void* d_target;            // Device memory for difficulty target
        void* d_found_nonce;       // Device memory for found nonce
        uint64_t* h_found_nonce;   // Host pinned memory for found nonce
        GPUMiningStats stats;
    };

    std::vector<CUDADevice> devices_;
    std::atomic<bool> mining_;
    std::atomic<bool> initialized_;
    GPUConfig config_;
    BlockHeader current_template_;
    DilithiumPubKey reward_address_;
    BlockFoundCallback block_found_callback_;
    mutable std::mutex mutex_;

    // CUDA helper functions
    bool initialize_device(CUDADevice& device);
    void cleanup_device(CUDADevice& device);
    bool allocate_device_memory(CUDADevice& device);
    void free_device_memory(CUDADevice& device);

    // Mining thread
    void mining_thread(int device_id);
    bool check_for_solution(CUDADevice& device, Block& block);

    // Monitoring
    void update_device_stats(CUDADevice& device);
    uint32_t get_device_temperature(int device_id) const;
    uint32_t get_device_fan_speed(int device_id) const;
    uint32_t get_device_power_usage(int device_id) const;
};

} // namespace gpu
} // namespace intcoin

#endif // INTCOIN_GPU_MINER_CUDA_H
