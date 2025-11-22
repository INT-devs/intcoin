// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// OpenCL GPU miner for AMD and other GPUs

#ifndef INTCOIN_GPU_MINER_OPENCL_H
#define INTCOIN_GPU_MINER_OPENCL_H

#include "gpu_miner.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <vector>
#include <mutex>
#include <thread>

namespace intcoin {
namespace gpu {

/**
 * OpenCL GPU Miner for AMD, Intel, and other GPUs
 */
class OpenCLMiner : public GPUMiner {
public:
    OpenCLMiner();
    ~OpenCLMiner() override;

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
    std::string get_platform_name() const override { return "OpenCL"; }

private:
    struct OpenCLDevice {
        int device_id;
        cl_platform_id platform;
        cl_device_id device;
        cl_context context;
        cl_command_queue queue;
        cl_program program;
        cl_kernel kernel;
        cl_mem d_block_header;      // Device memory for block header
        cl_mem d_target;            // Device memory for difficulty target
        cl_mem d_found_nonce;       // Device memory for found nonce
        std::string name;
        std::string vendor;
        size_t global_memory;
        size_t local_memory;
        uint32_t compute_units;
        GPUMiningStats stats;
    };

    std::vector<OpenCLDevice> devices_;
    std::vector<std::thread> mining_threads_;
    std::atomic<bool> mining_;
    std::atomic<bool> initialized_;
    GPUConfig config_;
    BlockHeader current_template_;
    DilithiumPubKey reward_address_;
    BlockFoundCallback block_found_callback_;
    mutable std::mutex mutex_;

    // OpenCL helper functions
    bool initialize_device(OpenCLDevice& device, cl_platform_id platform,
                          cl_device_id cl_device, int device_id);
    void cleanup_device(OpenCLDevice& device);
    bool build_kernel(OpenCLDevice& device);
    bool allocate_device_memory(OpenCLDevice& device);
    void free_device_memory(OpenCLDevice& device);

    // Mining thread
    void mining_thread(int device_idx);
    bool check_for_solution(OpenCLDevice& device, Block& block);

    // Monitoring
    void update_device_stats(OpenCLDevice& device);

    // OpenCL kernel source
    static const char* get_kernel_source();

    // Error handling
    static std::string get_opencl_error_string(cl_int error);
    static bool check_cl_error(cl_int error, const char* operation);
};

} // namespace gpu
} // namespace intcoin

#endif // INTCOIN_GPU_MINER_OPENCL_H
