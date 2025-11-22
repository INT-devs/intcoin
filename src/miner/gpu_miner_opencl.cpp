// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/gpu_miner_opencl.h"
#include "intcoin/crypto.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <sstream>

namespace intcoin {
namespace gpu {

// ============================================================================
// OpenCL Kernel Source Code
// ============================================================================

const char* OpenCLMiner::get_kernel_source() {
    return R"CLC(

// SHA-256 constants
__constant uint K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// SHA-256 helper functions
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

void sha256_transform(uint* state, const uint* data) {
    uint a, b, c, d, e, f, g, h, t1, t2, m[64];

    // Prepare message schedule
    for (int i = 0; i < 16; i++)
        m[i] = data[i];
    for (int i = 16; i < 64; i++)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    // Initialize working variables
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    // Compression function main loop
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    // Add compressed chunk to current hash value
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256_hash(__global const uchar* data, uint len, uchar* hash) {
    uint state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Process data in 512-bit chunks
    uint w[16];
    uint processed = 0;

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
    uchar final_block[64];
    uint remaining = len - processed;
    for (uint i = 0; i < remaining; i++)
        final_block[i] = data[processed + i];
    final_block[remaining] = 0x80;

    if (remaining >= 56) {
        for (uint i = remaining + 1; i < 64; i++)
            final_block[i] = 0;
        for (int i = 0; i < 16; i++) {
            w[i] = (final_block[i*4 + 0] << 24) |
                   (final_block[i*4 + 1] << 16) |
                   (final_block[i*4 + 2] << 8) |
                   (final_block[i*4 + 3]);
        }
        sha256_transform(state, w);
        for (uint i = 0; i < 56; i++)
            final_block[i] = 0;
    } else {
        for (uint i = remaining + 1; i < 56; i++)
            final_block[i] = 0;
    }

    // Append length in bits
    ulong bit_len = len * 8;
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

__kernel void mine_kernel(
    __global const uchar* block_header,
    uint header_size,
    __global const uchar* target,
    ulong start_nonce,
    __global ulong* found_nonce
) {
    ulong thread_id = get_global_id(0);
    ulong nonce = start_nonce + thread_id;

    // Check if solution already found
    if (*found_nonce != 0)
        return;

    // Copy block header to private memory and update nonce
    uchar local_header[128];
    for (uint i = 0; i < header_size; i++)
        local_header[i] = block_header[i];

    // Write nonce to header (assuming nonce is at fixed position)
    uint nonce_offset = 72;
    for (int i = 0; i < 8; i++)
        local_header[nonce_offset + i] = (nonce >> (i * 8)) & 0xFF;

    // Calculate double SHA-256 hash
    uchar hash1[32], hash2[32];
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
        atomic_cmpxchg(found_nonce, 0UL, nonce);
    }
}

)CLC";
}

// ============================================================================
// Helper Functions
// ============================================================================

std::string OpenCLMiner::get_opencl_error_string(cl_int error) {
    switch (error) {
        case CL_SUCCESS: return "Success";
        case CL_DEVICE_NOT_FOUND: return "Device not found";
        case CL_DEVICE_NOT_AVAILABLE: return "Device not available";
        case CL_COMPILER_NOT_AVAILABLE: return "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "Memory object allocation failure";
        case CL_OUT_OF_RESOURCES: return "Out of resources";
        case CL_OUT_OF_HOST_MEMORY: return "Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE: return "Profiling info not available";
        case CL_MEM_COPY_OVERLAP: return "Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH: return "Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED: return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE: return "Build program failure";
        case CL_MAP_FAILURE: return "Map failure";
        case CL_INVALID_VALUE: return "Invalid value";
        case CL_INVALID_DEVICE_TYPE: return "Invalid device type";
        case CL_INVALID_PLATFORM: return "Invalid platform";
        case CL_INVALID_DEVICE: return "Invalid device";
        case CL_INVALID_CONTEXT: return "Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES: return "Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE: return "Invalid command queue";
        case CL_INVALID_HOST_PTR: return "Invalid host pointer";
        case CL_INVALID_MEM_OBJECT: return "Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE: return "Invalid image size";
        case CL_INVALID_SAMPLER: return "Invalid sampler";
        case CL_INVALID_BINARY: return "Invalid binary";
        case CL_INVALID_BUILD_OPTIONS: return "Invalid build options";
        case CL_INVALID_PROGRAM: return "Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE: return "Invalid program executable";
        case CL_INVALID_KERNEL_NAME: return "Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION: return "Invalid kernel definition";
        case CL_INVALID_KERNEL: return "Invalid kernel";
        case CL_INVALID_ARG_INDEX: return "Invalid argument index";
        case CL_INVALID_ARG_VALUE: return "Invalid argument value";
        case CL_INVALID_ARG_SIZE: return "Invalid argument size";
        case CL_INVALID_KERNEL_ARGS: return "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION: return "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE: return "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE: return "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET: return "Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST: return "Invalid event wait list";
        case CL_INVALID_EVENT: return "Invalid event";
        case CL_INVALID_OPERATION: return "Invalid operation";
        case CL_INVALID_GL_OBJECT: return "Invalid GL object";
        case CL_INVALID_BUFFER_SIZE: return "Invalid buffer size";
        case CL_INVALID_MIP_LEVEL: return "Invalid mip level";
        case CL_INVALID_GLOBAL_WORK_SIZE: return "Invalid global work size";
        default: return "Unknown error (" + std::to_string(error) + ")";
    }
}

bool OpenCLMiner::check_cl_error(cl_int error, const char* operation) {
    if (error != CL_SUCCESS) {
        std::cerr << "OpenCL error in " << operation << ": "
                  << get_opencl_error_string(error) << std::endl;
        return false;
    }
    return true;
}

// ============================================================================
// OpenCLMiner Implementation
// ============================================================================

OpenCLMiner::OpenCLMiner()
    : mining_(false)
    , initialized_(false)
{
}

OpenCLMiner::~OpenCLMiner() {
    shutdown();
}

bool OpenCLMiner::initialize(int device_id) {
    if (initialized_) {
        std::cerr << "OpenCLMiner already initialized" << std::endl;
        return false;
    }

    cl_int err;
    cl_uint num_platforms;

    // Get platforms
    err = clGetPlatformIDs(0, nullptr, &num_platforms);
    if (!check_cl_error(err, "clGetPlatformIDs") || num_platforms == 0) {
        std::cerr << "No OpenCL platforms found" << std::endl;
        return false;
    }

    std::vector<cl_platform_id> platforms(num_platforms);
    err = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
    if (!check_cl_error(err, "clGetPlatformIDs")) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Enumerate all devices on all platforms
    int global_device_id = 0;
    for (cl_platform_id platform : platforms) {
        cl_uint num_devices;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
        if (err != CL_SUCCESS || num_devices == 0) {
            continue;
        }

        std::vector<cl_device_id> platform_devices(num_devices);
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, num_devices,
                            platform_devices.data(), nullptr);
        if (!check_cl_error(err, "clGetDeviceIDs")) {
            continue;
        }

        for (cl_device_id cl_device : platform_devices) {
            if (device_id == -1 || device_id == global_device_id) {
                OpenCLDevice device;
                if (initialize_device(device, platform, cl_device, global_device_id)) {
                    devices_.push_back(device);
                }
            }
            global_device_id++;
        }
    }

    if (devices_.empty()) {
        std::cerr << "Failed to initialize any OpenCL devices" << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "Initialized " << devices_.size() << " OpenCL device(s)" << std::endl;
    return true;
}

bool OpenCLMiner::initialize_device(OpenCLDevice& device, cl_platform_id platform,
                                    cl_device_id cl_device, int device_id) {
    cl_int err;

    device.device_id = device_id;
    device.platform = platform;
    device.device = cl_device;

    // Get device information
    char name[256], vendor[256];
    size_t name_size, vendor_size;

    clGetDeviceInfo(cl_device, CL_DEVICE_NAME, sizeof(name), name, &name_size);
    clGetDeviceInfo(cl_device, CL_DEVICE_VENDOR, sizeof(vendor), vendor, &vendor_size);
    clGetDeviceInfo(cl_device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(size_t),
                    &device.global_memory, nullptr);
    clGetDeviceInfo(cl_device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(size_t),
                    &device.local_memory, nullptr);
    clGetDeviceInfo(cl_device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(uint32_t),
                    &device.compute_units, nullptr);

    device.name = std::string(name, name_size - 1);
    device.vendor = std::string(vendor, vendor_size - 1);

    std::cout << "OpenCL Device " << device_id << ": " << device.name << std::endl;
    std::cout << "  Vendor: " << device.vendor << std::endl;
    std::cout << "  Global Memory: " << (device.global_memory / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Compute Units: " << device.compute_units << std::endl;

    // Create context
    device.context = clCreateContext(nullptr, 1, &cl_device, nullptr, nullptr, &err);
    if (!check_cl_error(err, "clCreateContext")) {
        return false;
    }

    // Create command queue
    device.queue = clCreateCommandQueue(device.context, cl_device, 0, &err);
    if (!check_cl_error(err, "clCreateCommandQueue")) {
        clReleaseContext(device.context);
        return false;
    }

    // Build kernel
    if (!build_kernel(device)) {
        clReleaseCommandQueue(device.queue);
        clReleaseContext(device.context);
        return false;
    }

    // Allocate device memory
    if (!allocate_device_memory(device)) {
        clReleaseKernel(device.kernel);
        clReleaseProgram(device.program);
        clReleaseCommandQueue(device.queue);
        clReleaseContext(device.context);
        return false;
    }

    return true;
}

bool OpenCLMiner::build_kernel(OpenCLDevice& device) {
    cl_int err;

    // Get kernel source
    const char* source = get_kernel_source();
    size_t source_size = std::strlen(source);

    // Create program
    device.program = clCreateProgramWithSource(device.context, 1, &source,
                                               &source_size, &err);
    if (!check_cl_error(err, "clCreateProgramWithSource")) {
        return false;
    }

    // Build program
    err = clBuildProgram(device.program, 1, &device.device, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // Get build log
        size_t log_size;
        clGetProgramBuildInfo(device.program, device.device, CL_PROGRAM_BUILD_LOG,
                             0, nullptr, &log_size);
        std::vector<char> log(log_size);
        clGetProgramBuildInfo(device.program, device.device, CL_PROGRAM_BUILD_LOG,
                             log_size, log.data(), nullptr);
        std::cerr << "OpenCL build error:\n" << log.data() << std::endl;
        clReleaseProgram(device.program);
        return false;
    }

    // Create kernel
    device.kernel = clCreateKernel(device.program, "mine_kernel", &err);
    if (!check_cl_error(err, "clCreateKernel")) {
        clReleaseProgram(device.program);
        return false;
    }

    return true;
}

bool OpenCLMiner::allocate_device_memory(OpenCLDevice& device) {
    cl_int err;

    // Allocate device memory for block header (128 bytes max)
    device.d_block_header = clCreateBuffer(device.context, CL_MEM_READ_ONLY,
                                          128, nullptr, &err);
    if (!check_cl_error(err, "clCreateBuffer (block_header)")) {
        return false;
    }

    // Allocate device memory for target (32 bytes)
    device.d_target = clCreateBuffer(device.context, CL_MEM_READ_ONLY,
                                    32, nullptr, &err);
    if (!check_cl_error(err, "clCreateBuffer (target)")) {
        clReleaseMemObject(device.d_block_header);
        return false;
    }

    // Allocate device memory for found nonce
    device.d_found_nonce = clCreateBuffer(device.context, CL_MEM_READ_WRITE,
                                         sizeof(uint64_t), nullptr, &err);
    if (!check_cl_error(err, "clCreateBuffer (found_nonce)")) {
        clReleaseMemObject(device.d_block_header);
        clReleaseMemObject(device.d_target);
        return false;
    }

    return true;
}

void OpenCLMiner::free_device_memory(OpenCLDevice& device) {
    if (device.d_block_header) {
        clReleaseMemObject(device.d_block_header);
        device.d_block_header = nullptr;
    }
    if (device.d_target) {
        clReleaseMemObject(device.d_target);
        device.d_target = nullptr;
    }
    if (device.d_found_nonce) {
        clReleaseMemObject(device.d_found_nonce);
        device.d_found_nonce = nullptr;
    }
}

void OpenCLMiner::cleanup_device(OpenCLDevice& device) {
    free_device_memory(device);
    if (device.kernel) {
        clReleaseKernel(device.kernel);
        device.kernel = nullptr;
    }
    if (device.program) {
        clReleaseProgram(device.program);
        device.program = nullptr;
    }
    if (device.queue) {
        clReleaseCommandQueue(device.queue);
        device.queue = nullptr;
    }
    if (device.context) {
        clReleaseContext(device.context);
        device.context = nullptr;
    }
}

void OpenCLMiner::shutdown() {
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

std::vector<GPUDevice> OpenCLMiner::list_devices() const {
    std::vector<GPUDevice> result;
    cl_int err;
    cl_uint num_platforms;

    err = clGetPlatformIDs(0, nullptr, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        return result;
    }

    std::vector<cl_platform_id> platforms(num_platforms);
    clGetPlatformIDs(num_platforms, platforms.data(), nullptr);

    int global_device_id = 0;
    for (cl_platform_id platform : platforms) {
        cl_uint num_devices;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
        if (err != CL_SUCCESS || num_devices == 0) {
            continue;
        }

        std::vector<cl_device_id> platform_devices(num_devices);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, num_devices,
                      platform_devices.data(), nullptr);

        for (cl_device_id cl_device : platform_devices) {
            GPUDevice dev;
            dev.device_id = global_device_id++;

            char name[256], vendor[256];
            size_t global_mem, local_mem;
            uint32_t compute_units;

            clGetDeviceInfo(cl_device, CL_DEVICE_NAME, sizeof(name), name, nullptr);
            clGetDeviceInfo(cl_device, CL_DEVICE_VENDOR, sizeof(vendor), vendor, nullptr);
            clGetDeviceInfo(cl_device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(size_t),
                           &global_mem, nullptr);
            clGetDeviceInfo(cl_device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(size_t),
                           &local_mem, nullptr);
            clGetDeviceInfo(cl_device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(uint32_t),
                           &compute_units, nullptr);

            dev.name = name;
            dev.vendor = vendor;
            dev.global_memory = global_mem;
            dev.local_memory = local_mem;
            dev.compute_units = compute_units;
            dev.clock_frequency = 0;  // Not easily available in OpenCL
            dev.is_available = true;
            dev.platform = "OpenCL";

            result.push_back(dev);
        }
    }

    return result;
}

bool OpenCLMiner::start_mining(const BlockHeader& block_template,
                               const DilithiumPubKey& reward_address) {
    if (!initialized_) {
        std::cerr << "OpenCLMiner not initialized" << std::endl;
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
    for (size_t i = 0; i < devices_.size(); i++) {
        mining_threads_.emplace_back(&OpenCLMiner::mining_thread, this, i);
    }

    return true;
}

void OpenCLMiner::stop_mining() {
    mining_ = false;

    // Wait for all mining threads to finish
    for (auto& thread : mining_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    mining_threads_.clear();
}

void OpenCLMiner::update_block_template(const BlockHeader& new_template) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_template_ = new_template;
}

void OpenCLMiner::mining_thread(int device_idx) {
    if (device_idx >= devices_.size()) {
        return;
    }

    OpenCLDevice& device = devices_[device_idx];
    cl_int err;

    // Prepare block header data
    uint8_t header_data[128];
    size_t header_size = 80;

    // Prepare difficulty target
    uint8_t target[32];
    // (Convert difficulty bits to target - implementation needed)

    // Copy to device
    err = clEnqueueWriteBuffer(device.queue, device.d_block_header, CL_FALSE,
                              0, header_size, header_data, 0, nullptr, nullptr);
    check_cl_error(err, "clEnqueueWriteBuffer (header)");

    err = clEnqueueWriteBuffer(device.queue, device.d_target, CL_FALSE,
                              0, 32, target, 0, nullptr, nullptr);
    check_cl_error(err, "clEnqueueWriteBuffer (target)");

    uint64_t start_nonce = 0;
    uint64_t found_nonce = 0;
    auto last_hash_time = std::chrono::steady_clock::now();
    uint64_t hashes_this_period = 0;

    while (mining_) {
        // Reset found nonce
        found_nonce = 0;
        err = clEnqueueWriteBuffer(device.queue, device.d_found_nonce, CL_FALSE,
                                  0, sizeof(uint64_t), &found_nonce, 0, nullptr, nullptr);
        check_cl_error(err, "clEnqueueWriteBuffer (found_nonce)");

        // Set kernel arguments
        err = clSetKernelArg(device.kernel, 0, sizeof(cl_mem), &device.d_block_header);
        err |= clSetKernelArg(device.kernel, 1, sizeof(uint32_t), &header_size);
        err |= clSetKernelArg(device.kernel, 2, sizeof(cl_mem), &device.d_target);
        err |= clSetKernelArg(device.kernel, 3, sizeof(uint64_t), &start_nonce);
        err |= clSetKernelArg(device.kernel, 4, sizeof(cl_mem), &device.d_found_nonce);
        if (!check_cl_error(err, "clSetKernelArg")) {
            break;
        }

        // Execute kernel
        size_t global_work_size = config_.global_work_size;
        size_t local_work_size = config_.work_group_size;

        err = clEnqueueNDRangeKernel(device.queue, device.kernel, 1, nullptr,
                                    &global_work_size, &local_work_size,
                                    0, nullptr, nullptr);
        if (!check_cl_error(err, "clEnqueueNDRangeKernel")) {
            break;
        }

        // Read result
        err = clEnqueueReadBuffer(device.queue, device.d_found_nonce, CL_TRUE,
                                 0, sizeof(uint64_t), &found_nonce,
                                 0, nullptr, nullptr);
        check_cl_error(err, "clEnqueueReadBuffer (found_nonce)");

        // Update statistics
        uint64_t hashes_done = global_work_size;
        hashes_this_period += hashes_done;
        device.stats.total_hashes += hashes_done;

        // Check if solution found
        if (found_nonce != 0) {
            // Solution found!
            Block block;
            block.header = current_template_;
            block.header.nonce = found_nonce;

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

            if (config_.enable_monitoring) {
                update_device_stats(device);
            }
        }

        // Next batch of nonces
        start_nonce += hashes_done;
    }
}

bool OpenCLMiner::check_for_solution(OpenCLDevice& device, Block& block) {
    // Handled in mining_thread
    return false;
}

void OpenCLMiner::update_device_stats(OpenCLDevice& device) {
    // OpenCL doesn't provide standard APIs for temperature/fan/power monitoring
    // Would require vendor-specific extensions (AMD ADL, etc.)
    // For now, leave as 0
}

GPUMiningStats OpenCLMiner::get_stats(int device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device_id >= 0 && device_id < devices_.size()) {
        return devices_[device_id].stats;
    }
    return GPUMiningStats();
}

uint64_t OpenCLMiner::get_hashrate(int device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device_id >= 0 && device_id < devices_.size()) {
        return devices_[device_id].stats.hashes_per_second;
    }
    return 0;
}

void OpenCLMiner::set_config(const GPUConfig& config) {
    config_ = config;
}

} // namespace gpu
} // namespace intcoin
