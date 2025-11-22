// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/gpu_miner_cuda.h"
#include "intcoin/crypto.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

// CUDA error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": " \
                      << cudaGetErrorString(error) << std::endl; \
            return false; \
        } \
    } while(0)

#define CUDA_CHECK_VOID(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": " \
                      << cudaGetErrorString(error) << std::endl; \
        } \
    } while(0)

namespace intcoin {
namespace gpu {

// ============================================================================
// CUDA Kernels
// ============================================================================

/**
 * SHA-256 constants
 */
__constant__ uint32_t d_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/**
 * SHA-256 helper macros
 */
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/**
 * Device function: SHA-256 compression
 */
__device__ void sha256_transform(uint32_t state[8], const uint32_t data[16]) {
    uint32_t a, b, c, d, e, f, g, h, t1, t2, m[64];

    // Prepare message schedule
    for (int i = 0; i < 16; i++)
        m[i] = data[i];
    for (int i = 16; i < 64; i++)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    // Initialize working variables
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    // Compression function main loop
    #pragma unroll
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + d_K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    // Add compressed chunk to current hash value
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/**
 * Device function: Full SHA-256 hash
 */
__device__ void sha256_hash(const uint8_t* data, size_t len, uint8_t* hash) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Process data in 512-bit chunks
    uint32_t w[16];
    size_t processed = 0;

    while (processed + 64 <= len) {
        // Convert bytes to words (big-endian)
        for (int i = 0; i < 16; i++) {
            w[i] = (data[processed + i*4 + 0] << 24) |
                   (data[processed + i*4 + 1] << 16) |
                   (data[processed + i*4 + 2] << 8) |
                   (data[processed + i*4 + 3]);
        }
        sha256_transform(state, w);
        processed += 64;
    }

    // Padding and final block
    uint8_t final_block[64];
    size_t remaining = len - processed;
    memcpy(final_block, data + processed, remaining);
    final_block[remaining] = 0x80;

    if (remaining >= 56) {
        memset(final_block + remaining + 1, 0, 63 - remaining);
        for (int i = 0; i < 16; i++) {
            w[i] = (final_block[i*4 + 0] << 24) |
                   (final_block[i*4 + 1] << 16) |
                   (final_block[i*4 + 2] << 8) |
                   (final_block[i*4 + 3]);
        }
        sha256_transform(state, w);
        memset(final_block, 0, 56);
    } else {
        memset(final_block + remaining + 1, 0, 55 - remaining);
    }

    // Append length in bits
    uint64_t bit_len = len * 8;
    for (int i = 0; i < 8; i++)
        final_block[56 + i] = (bit_len >> (56 - i*8)) & 0xFF;

    for (int i = 0; i < 16; i++) {
        w[i] = (final_block[i*4 + 0] << 24) |
               (final_block[i*4 + 1] << 16) |
               (final_block[i*4 + 2] << 8) |
               (final_block[i*4 + 3]);
    }
    sha256_transform(state, w);

    // Convert state to bytes (big-endian)
    for (int i = 0; i < 8; i++) {
        hash[i*4 + 0] = (state[i] >> 24) & 0xFF;
        hash[i*4 + 1] = (state[i] >> 16) & 0xFF;
        hash[i*4 + 2] = (state[i] >> 8) & 0xFF;
        hash[i*4 + 3] = state[i] & 0xFF;
    }
}

/**
 * CUDA Mining Kernel
 *
 * Each thread tests a different nonce value
 */
__global__ void mine_kernel(
    const uint8_t* block_header,
    size_t header_size,
    const uint8_t* target,
    uint64_t start_nonce,
    uint64_t* found_nonce
) {
    uint64_t thread_id = blockIdx.x * blockDim.x + threadIdx.x;
    uint64_t nonce = start_nonce + thread_id;

    // Check if solution already found
    if (*found_nonce != 0)
        return;

    // Copy block header to local memory and update nonce
    uint8_t local_header[128];  // Max header size
    for (size_t i = 0; i < header_size; i++)
        local_header[i] = block_header[i];

    // Write nonce to header (assuming nonce is at fixed position)
    // Position depends on BlockHeader structure
    size_t nonce_offset = 72;  // Adjust based on actual header structure
    for (int i = 0; i < 8; i++)
        local_header[nonce_offset + i] = (nonce >> (i * 8)) & 0xFF;

    // Calculate double SHA-256 hash
    uint8_t hash1[32], hash2[32];
    sha256_hash(local_header, header_size, hash1);
    sha256_hash(hash1, 32, hash2);

    // Check if hash meets target (little-endian comparison)
    bool solution_found = true;
    for (int i = 31; i >= 0; i--) {
        if (hash2[i] < target[i])
            break;
        if (hash2[i] > target[i]) {
            solution_found = false;
            break;
        }
    }

    // Atomically set found nonce if solution is valid
    if (solution_found) {
        atomicExch((unsigned long long*)found_nonce, nonce);
    }
}

// ============================================================================
// CUDAMiner Implementation
// ============================================================================

CUDAMiner::CUDAMiner()
    : mining_(false)
    , initialized_(false)
{
}

CUDAMiner::~CUDAMiner() {
    shutdown();
}

bool CUDAMiner::initialize(int device_id) {
    if (initialized_) {
        std::cerr << "CUDAMiner already initialized" << std::endl;
        return false;
    }

    int device_count;
    CUDA_CHECK(cudaGetDeviceCount(&device_count));

    if (device_count == 0) {
        std::cerr << "No CUDA devices found" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (device_id == -1) {
        // Initialize all devices
        for (int i = 0; i < device_count; i++) {
            CUDADevice dev;
            dev.device_id = i;
            if (initialize_device(dev)) {
                devices_.push_back(dev);
            }
        }
    } else {
        // Initialize specific device
        if (device_id >= device_count) {
            std::cerr << "Invalid device ID: " << device_id << std::endl;
            return false;
        }
        CUDADevice dev;
        dev.device_id = device_id;
        if (initialize_device(dev)) {
            devices_.push_back(dev);
        }
    }

    if (devices_.empty()) {
        std::cerr << "Failed to initialize any CUDA devices" << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "Initialized " << devices_.size() << " CUDA device(s)" << std::endl;
    return true;
}

bool CUDAMiner::initialize_device(CUDADevice& device) {
    CUDA_CHECK(cudaSetDevice(device.device_id));
    CUDA_CHECK(cudaGetDeviceProperties(&device.properties, device.device_id));

    std::cout << "CUDA Device " << device.device_id << ": "
              << device.properties.name << std::endl;
    std::cout << "  Compute Capability: " << device.properties.major
              << "." << device.properties.minor << std::endl;
    std::cout << "  Global Memory: "
              << (device.properties.totalGlobalMem / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Multiprocessors: " << device.properties.multiProcessorCount << std::endl;

    // Create CUDA stream for this device
    CUDA_CHECK(cudaStreamCreate(&device.stream));

    // Allocate device memory
    if (!allocate_device_memory(device)) {
        return false;
    }

    return true;
}

bool CUDAMiner::allocate_device_memory(CUDADevice& device) {
    CUDA_CHECK(cudaSetDevice(device.device_id));

    // Allocate device memory for block header (128 bytes max)
    CUDA_CHECK(cudaMalloc(&device.d_block_header, 128));

    // Allocate device memory for target (32 bytes)
    CUDA_CHECK(cudaMalloc(&device.d_target, 32));

    // Allocate device memory for found nonce
    CUDA_CHECK(cudaMalloc(&device.d_found_nonce, sizeof(uint64_t)));

    // Allocate pinned host memory for found nonce
    CUDA_CHECK(cudaHostAlloc(&device.h_found_nonce, sizeof(uint64_t),
                             cudaHostAllocDefault));

    return true;
}

void CUDAMiner::free_device_memory(CUDADevice& device) {
    if (device.d_block_header) {
        cudaFree(device.d_block_header);
        device.d_block_header = nullptr;
    }
    if (device.d_target) {
        cudaFree(device.d_target);
        device.d_target = nullptr;
    }
    if (device.d_found_nonce) {
        cudaFree(device.d_found_nonce);
        device.d_found_nonce = nullptr;
    }
    if (device.h_found_nonce) {
        cudaFreeHost(device.h_found_nonce);
        device.h_found_nonce = nullptr;
    }
}

void CUDAMiner::cleanup_device(CUDADevice& device) {
    cudaSetDevice(device.device_id);
    free_device_memory(device);
    if (device.stream) {
        cudaStreamDestroy(device.stream);
        device.stream = nullptr;
    }
}

void CUDAMiner::shutdown() {
    if (!initialized_) {
        return;
    }

    stop_mining();

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& device : devices_) {
        cleanup_device(device);
    }
    devices_.clear();
    initialized_ = false;
}

std::vector<GPUDevice> CUDAMiner::list_devices() const {
    std::vector<GPUDevice> result;

    int device_count;
    if (cudaGetDeviceCount(&device_count) != cudaSuccess) {
        return result;
    }

    for (int i = 0; i < device_count; i++) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, i) == cudaSuccess) {
            GPUDevice dev;
            dev.device_id = i;
            dev.name = prop.name;
            dev.vendor = "NVIDIA";
            dev.global_memory = prop.totalGlobalMem;
            dev.local_memory = prop.sharedMemPerBlock;
            dev.compute_units = prop.multiProcessorCount;
            dev.clock_frequency = prop.clockRate / 1000;  // Convert kHz to MHz
            dev.is_available = true;
            dev.platform = "CUDA";
            result.push_back(dev);
        }
    }

    return result;
}

bool CUDAMiner::start_mining(const BlockHeader& block_template,
                             const DilithiumPubKey& reward_address) {
    if (!initialized_) {
        std::cerr << "CUDAMiner not initialized" << std::endl;
        return false;
    }

    if (mining_) {
        std::cerr << "Mining already in progress" << std::endl;
        return false;
    }

    current_template_ = block_template;
    reward_address_ = reward_address;
    mining_ = true;

    // Start mining thread for each device
    for (auto& device : devices_) {
        std::thread(&CUDAMiner::mining_thread, this, device.device_id).detach();
    }

    return true;
}

void CUDAMiner::stop_mining() {
    mining_ = false;
    // Wait a bit for threads to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void CUDAMiner::update_block_template(const BlockHeader& new_template) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_template_ = new_template;
}

void CUDAMiner::mining_thread(int device_idx) {
    if (device_idx >= devices_.size()) {
        return;
    }

    CUDADevice& device = devices_[device_idx];
    CUDA_CHECK_VOID(cudaSetDevice(device.device_id));

    // Prepare block header data
    // (This is simplified - actual implementation needs proper serialization)
    uint8_t header_data[128];
    size_t header_size = 80;  // Bitcoin-style 80-byte header

    // Prepare difficulty target
    uint8_t target[32];
    // (Convert difficulty bits to target - implementation needed)

    // Copy to device
    CUDA_CHECK_VOID(cudaMemcpyAsync(device.d_block_header, header_data, header_size,
                                    cudaMemcpyHostToDevice, device.stream));
    CUDA_CHECK_VOID(cudaMemcpyAsync(device.d_target, target, 32,
                                    cudaMemcpyHostToDevice, device.stream));

    uint64_t start_nonce = 0;
    auto last_hash_time = std::chrono::steady_clock::now();
    uint64_t hashes_this_period = 0;

    while (mining_) {
        // Reset found nonce
        *device.h_found_nonce = 0;
        CUDA_CHECK_VOID(cudaMemcpyAsync(device.d_found_nonce, device.h_found_nonce,
                                        sizeof(uint64_t), cudaMemcpyHostToDevice,
                                        device.stream));

        // Launch mining kernel
        uint32_t threads = config_.threads_per_block;
        uint32_t blocks = config_.blocks_per_grid;

        mine_kernel<<<blocks, threads, 0, device.stream>>>(
            (uint8_t*)device.d_block_header,
            header_size,
            (uint8_t*)device.d_target,
            start_nonce,
            (uint64_t*)device.d_found_nonce
        );

        // Copy result back
        CUDA_CHECK_VOID(cudaMemcpyAsync(device.h_found_nonce, device.d_found_nonce,
                                        sizeof(uint64_t), cudaMemcpyDeviceToHost,
                                        device.stream));
        cudaStreamSynchronize(device.stream);

        // Update statistics
        uint64_t hashes_done = (uint64_t)threads * blocks;
        hashes_this_period += hashes_done;
        device.stats.total_hashes += hashes_done;

        // Check if solution found
        if (*device.h_found_nonce != 0) {
            // Solution found!
            Block block;
            block.header = current_template_;
            block.header.nonce = *device.h_found_nonce;

            device.stats.blocks_found++;

            if (block_found_callback_) {
                block_found_callback_(block, device.device_id);
            }
        }

        // Update hashrate every second
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_hash_time).count();
        if (elapsed >= 1) {
            device.stats.hashes_per_second = hashes_this_period / elapsed;
            hashes_this_period = 0;
            last_hash_time = now;

            // Update monitoring stats if enabled
            if (config_.enable_monitoring) {
                update_device_stats(device);
            }
        }

        // Next batch of nonces
        start_nonce += hashes_done;
    }
}

bool CUDAMiner::check_for_solution(CUDADevice& device, Block& block) {
    // This is handled in mining_thread
    return false;
}

void CUDAMiner::update_device_stats(CUDADevice& device) {
    // Update temperature, fan speed, power usage using NVML if available
    // This would require linking against NVIDIA Management Library (libnvidia-ml)
    // For now, we'll leave these as 0
    device.stats.temperature = get_device_temperature(device.device_id);
    device.stats.fan_speed = get_device_fan_speed(device.device_id);
    device.stats.power_usage = get_device_power_usage(device.device_id);

    if (device.stats.power_usage > 0) {
        device.stats.efficiency =
            (double)device.stats.hashes_per_second / device.stats.power_usage;
    }
}

uint32_t CUDAMiner::get_device_temperature(int device_id) const {
    // Would require NVML integration
    return 0;
}

uint32_t CUDAMiner::get_device_fan_speed(int device_id) const {
    // Would require NVML integration
    return 0;
}

uint32_t CUDAMiner::get_device_power_usage(int device_id) const {
    // Would require NVML integration
    return 0;
}

GPUMiningStats CUDAMiner::get_stats(int device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device_id >= 0 && device_id < devices_.size()) {
        return devices_[device_id].stats;
    }
    return GPUMiningStats();
}

uint64_t CUDAMiner::get_hashrate(int device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device_id >= 0 && device_id < devices_.size()) {
        return devices_[device_id].stats.hashes_per_second;
    }
    return 0;
}

void CUDAMiner::set_config(const GPUConfig& config) {
    config_ = config;
}

} // namespace gpu
} // namespace intcoin
