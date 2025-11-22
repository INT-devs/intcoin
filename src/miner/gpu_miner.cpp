// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/gpu_miner.h"

#ifdef ENABLE_CUDA
#include "intcoin/gpu_miner_cuda.h"
#endif

#ifdef ENABLE_OPENCL
#include "intcoin/gpu_miner_opencl.h"
#endif

#include <iostream>

namespace intcoin {
namespace gpu {

// ============================================================================
// GPUMinerFactory Implementation
// ============================================================================

std::unique_ptr<GPUMiner> GPUMinerFactory::create(Platform platform) {
    // If AUTO, determine the best platform
    if (platform == Platform::AUTO) {
        platform = get_recommended_platform();
    }

    switch (platform) {
        case Platform::CUDA:
#ifdef ENABLE_CUDA
            if (is_cuda_available()) {
                std::cout << "Creating CUDA GPU miner..." << std::endl;
                return std::make_unique<CUDAMiner>();
            } else {
                std::cerr << "CUDA requested but not available" << std::endl;
                return nullptr;
            }
#else
            std::cerr << "CUDA support not compiled in (use -DENABLE_CUDA=ON)" << std::endl;
            return nullptr;
#endif

        case Platform::OPENCL:
#ifdef ENABLE_OPENCL
            if (is_opencl_available()) {
                std::cout << "Creating OpenCL GPU miner..." << std::endl;
                return std::make_unique<OpenCLMiner>();
            } else {
                std::cerr << "OpenCL requested but not available" << std::endl;
                return nullptr;
            }
#else
            std::cerr << "OpenCL support not compiled in (use -DENABLE_OPENCL=ON)" << std::endl;
            return nullptr;
#endif

        default:
            std::cerr << "Unknown GPU platform" << std::endl;
            return nullptr;
    }
}

bool GPUMinerFactory::is_cuda_available() {
#ifdef ENABLE_CUDA
    int device_count = 0;
    cudaError_t error = cudaGetDeviceCount(&device_count);
    return (error == cudaSuccess && device_count > 0);
#else
    return false;
#endif
}

bool GPUMinerFactory::is_opencl_available() {
#ifdef ENABLE_OPENCL
    cl_uint num_platforms = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        return false;
    }

    // Check if any platform has GPU devices
    std::vector<cl_platform_id> platforms(num_platforms);
    clGetPlatformIDs(num_platforms, platforms.data(), nullptr);

    for (cl_platform_id platform : platforms) {
        cl_uint num_devices = 0;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
        if (err == CL_SUCCESS && num_devices > 0) {
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}

std::vector<GPUDevice> GPUMinerFactory::get_all_devices() {
    std::vector<GPUDevice> all_devices;

#ifdef ENABLE_CUDA
    if (is_cuda_available()) {
        auto cuda_miner = std::make_unique<CUDAMiner>();
        auto cuda_devices = cuda_miner->list_devices();
        all_devices.insert(all_devices.end(), cuda_devices.begin(), cuda_devices.end());
    }
#endif

#ifdef ENABLE_OPENCL
    if (is_opencl_available()) {
        auto opencl_miner = std::make_unique<OpenCLMiner>();
        auto opencl_devices = opencl_miner->list_devices();

        // Adjust device IDs to avoid conflicts with CUDA devices
        int id_offset = all_devices.size();
        for (auto& dev : opencl_devices) {
            dev.device_id += id_offset;
            all_devices.push_back(dev);
        }
    }
#endif

    return all_devices;
}

GPUMinerFactory::Platform GPUMinerFactory::get_recommended_platform() {
    // Priority: CUDA (NVIDIA) > OpenCL (AMD/other)
    // CUDA generally has better performance and tooling for NVIDIA GPUs

#ifdef ENABLE_CUDA
    if (is_cuda_available()) {
        std::cout << "CUDA available - using CUDA platform" << std::endl;
        return Platform::CUDA;
    }
#endif

#ifdef ENABLE_OPENCL
    if (is_opencl_available()) {
        std::cout << "OpenCL available - using OpenCL platform" << std::endl;
        return Platform::OPENCL;
    }
#endif

    std::cerr << "No GPU platforms available" << std::endl;
    return Platform::AUTO;  // Will fail later
}

} // namespace gpu
} // namespace intcoin
