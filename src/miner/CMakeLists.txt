# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Miner module
set(MINER_SOURCES
    miner.cpp
    gpu_miner.cpp
)

# GPU Mining Support
if(ENABLE_GPU_CUDA)
    # Enable CUDA
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)

    list(APPEND MINER_SOURCES gpu_miner_cuda.cu)

    # CUDA-specific compiler flags
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -arch=sm_52")  # Minimum Maxwell architecture

    # Add CUDA definitions
    add_definitions(-DENABLE_CUDA)
endif()

if(ENABLE_GPU_OPENCL)
    # Find OpenCL
    find_package(OpenCL REQUIRED)

    list(APPEND MINER_SOURCES gpu_miner_opencl.cpp)

    # Add OpenCL definitions
    add_definitions(-DENABLE_OPENCL)
endif()

add_library(miner_lib OBJECT
    ${MINER_SOURCES}
)

target_include_directories(miner_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

find_package(RocksDB REQUIRED)

target_link_libraries(miner_lib
    PUBLIC
        RocksDB::rocksdb
)

# Link GPU libraries
if(ENABLE_GPU_CUDA)
    target_link_libraries(miner_lib
        PUBLIC
            ${CUDA_LIBRARIES}
    )
    target_include_directories(miner_lib
        PUBLIC
            ${CUDA_INCLUDE_DIRS}
    )
endif()

if(ENABLE_GPU_OPENCL)
    target_link_libraries(miner_lib
        PUBLIC
            OpenCL::OpenCL
    )
endif()
