// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// INTcoin Miner with GPU Support

#include "intcoin/miner.h"
#include "intcoin/blockchain.h"
#include "intcoin/mempool.h"
#include "intcoin/crypto.h"

#if defined(ENABLE_CUDA) || defined(ENABLE_OPENCL)
#include "intcoin/gpu_miner.h"
#define GPU_SUPPORT_ENABLED
#endif

#include <iostream>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>
#include <iomanip>
#include <getopt.h>
#include <string>
#include <vector>

using namespace intcoin;

// Global flag for graceful shutdown
std::atomic<bool> g_shutdown(false);

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    g_shutdown = true;
}

// Helper to convert hash to hex string
std::string hash_to_hex(const Hash256& hash) {
    static const char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(64);
    for (uint8_t byte : hash) {
        result += hex_chars[byte >> 4];
        result += hex_chars[byte & 0x0F];
    }
    return result;
}

void print_usage(const char* program_name) {
    std::cout << "INTcoin Miner v1.3.0" << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core (Maddison Lane)" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -a, --address <address>   Mining reward address (required)" << std::endl;
    std::cout << "  -t, --threads <n>         Number of CPU mining threads (default: auto-detect)" << std::endl;
    std::cout << "  -d, --data-dir <path>     Data directory (default: ~/.intcoin)" << std::endl;
    std::cout << "  -n, --extra-nonce <text>  Extra nonce text (default: empty)" << std::endl;
    std::cout << "  -v, --verbose             Verbose output" << std::endl;
    std::cout << "  -h, --help                Show this help message" << std::endl;
    std::cout << std::endl;

#ifdef GPU_SUPPORT_ENABLED
    std::cout << "GPU Mining Options:" << std::endl;
    std::cout << "  --gpu                     Enable GPU mining (auto-detect platform)" << std::endl;
    std::cout << "  --gpu-platform <type>     GPU platform: cuda, opencl, or auto (default: auto)" << std::endl;
    std::cout << "  --gpu-device <id>         GPU device ID (default: all devices)" << std::endl;
    std::cout << "  --list-gpus               List available GPU devices and exit" << std::endl;
    std::cout << "  --gpu-intensity <1-31>    Mining intensity (default: 20)" << std::endl;
    std::cout << "  --gpu-threads <n>         Threads per block (CUDA) / work group size (OpenCL)" << std::endl;
    std::cout << "  --gpu-blocks <n>          Blocks per grid (CUDA only)" << std::endl;
    std::cout << std::endl;
#endif

    std::cout << "Mining Modes:" << std::endl;
    std::cout << "  CPU only:  " << program_name << " -a <address> -t 4" << std::endl;
#ifdef GPU_SUPPORT_ENABLED
    std::cout << "  GPU only:  " << program_name << " -a <address> --gpu" << std::endl;
    std::cout << "  CPU + GPU: " << program_name << " -a <address> -t 2 --gpu" << std::endl;
#endif
    std::cout << std::endl;

    std::cout << "Examples:" << std::endl;
    std::cout << "  # CPU mining with 4 threads" << std::endl;
    std::cout << "  " << program_name << " -a INT1qw508d6qejxtdg4y5r3zarvary0c5xw7k -t 4" << std::endl;
#ifdef GPU_SUPPORT_ENABLED
    std::cout << std::endl;
    std::cout << "  # GPU mining with auto-detected platform" << std::endl;
    std::cout << "  " << program_name << " -a INT1qw508d6qejxtdg4y5r3zarvary0c5xw7k --gpu" << std::endl;
    std::cout << std::endl;
    std::cout << "  # GPU mining with specific device" << std::endl;
    std::cout << "  " << program_name << " -a INT1qw508d6qejxtdg4y5r3zarvary0c5xw7k --gpu --gpu-device 0" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Hybrid CPU+GPU mining" << std::endl;
    std::cout << "  " << program_name << " -a INT1qw508d6qejxtdg4y5r3zarvary0c5xw7k -t 2 --gpu" << std::endl;
#endif
    std::cout << std::endl;
}

#ifdef GPU_SUPPORT_ENABLED
void list_gpus() {
    std::cout << "Available GPU Devices:" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << std::endl;

    auto devices = gpu::GPUMinerFactory::get_all_devices();

    if (devices.empty()) {
        std::cout << "No GPU devices found." << std::endl;
        std::cout << std::endl;
        std::cout << "Make sure you have:" << std::endl;
#ifdef ENABLE_CUDA
        std::cout << "  - NVIDIA GPU drivers installed" << std::endl;
        std::cout << "  - CUDA toolkit installed" << std::endl;
#endif
#ifdef ENABLE_OPENCL
        std::cout << "  - OpenCL drivers installed for your GPU" << std::endl;
#endif
        return;
    }

    for (const auto& dev : devices) {
        std::cout << "Device " << dev.device_id << ": " << dev.name << std::endl;
        std::cout << "  Platform:       " << dev.platform << std::endl;
        std::cout << "  Vendor:         " << dev.vendor << std::endl;
        std::cout << "  Global Memory:  "
                  << (dev.global_memory / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  Local Memory:   "
                  << (dev.local_memory / 1024) << " KB" << std::endl;
        std::cout << "  Compute Units:  " << dev.compute_units << std::endl;
        if (dev.clock_frequency > 0) {
            std::cout << "  Clock Speed:    " << dev.clock_frequency << " MHz" << std::endl;
        }
        std::cout << "  Available:      " << (dev.is_available ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
    }
}

void print_gpu_stats(const gpu::GPUMiningStats& stats, int device_id) {
    std::cout << "[GPU " << device_id << "] ";
    std::cout << "Hashrate: " << std::fixed << std::setprecision(2)
              << (stats.hashes_per_second / 1000000.0) << " MH/s | ";
    std::cout << "Total: " << (stats.total_hashes / 1000000) << "M | ";
    std::cout << "Blocks: " << stats.blocks_found;

    if (stats.temperature > 0) {
        std::cout << " | Temp: " << stats.temperature << "°C";
    }
    if (stats.fan_speed > 0) {
        std::cout << " | Fan: " << stats.fan_speed << "%";
    }
    if (stats.power_usage > 0) {
        std::cout << " | Power: " << stats.power_usage << "W";
    }
    std::cout << std::endl;
}
#endif

void print_stats(const MiningStats& stats) {
    std::cout << "[CPU] ";
    std::cout << "Hashrate: " << std::fixed << std::setprecision(2)
              << (stats.hashes_per_second / 1000000.0) << " MH/s | ";
    std::cout << "Total: " << (stats.total_hashes / 1000000) << "M | ";
    std::cout << "Blocks: " << stats.blocks_found;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Register signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Parse command line options
    std::string address_str;
    std::string data_dir;
    std::string extra_nonce;
    size_t num_threads = 0;
    bool verbose = false;

#ifdef GPU_SUPPORT_ENABLED
    bool use_gpu = false;
    bool list_gpus_only = false;
    std::string gpu_platform_str = "auto";
    int gpu_device_id = -1;
    uint32_t gpu_intensity = 20;
    uint32_t gpu_threads = 256;
    uint32_t gpu_blocks = 8192;
#endif

    static struct option long_options[] = {
        {"address",     required_argument, 0, 'a'},
        {"threads",     required_argument, 0, 't'},
        {"data-dir",    required_argument, 0, 'd'},
        {"extra-nonce", required_argument, 0, 'n'},
        {"verbose",     no_argument,       0, 'v'},
        {"help",        no_argument,       0, 'h'},
#ifdef GPU_SUPPORT_ENABLED
        {"gpu",           no_argument,       0, 'G'},
        {"gpu-platform",  required_argument, 0, 'P'},
        {"gpu-device",    required_argument, 0, 'D'},
        {"list-gpus",     no_argument,       0, 'L'},
        {"gpu-intensity", required_argument, 0, 'I'},
        {"gpu-threads",   required_argument, 0, 'T'},
        {"gpu-blocks",    required_argument, 0, 'B'},
#endif
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "a:t:d:n:vh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'a':
                address_str = optarg;
                break;
            case 't':
                num_threads = std::stoul(optarg);
                break;
            case 'd':
                data_dir = optarg;
                break;
            case 'n':
                extra_nonce = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
#ifdef GPU_SUPPORT_ENABLED
            case 'G':
                use_gpu = true;
                break;
            case 'P':
                gpu_platform_str = optarg;
                break;
            case 'D':
                gpu_device_id = std::stoi(optarg);
                break;
            case 'L':
                list_gpus_only = true;
                break;
            case 'I':
                gpu_intensity = std::stoul(optarg);
                if (gpu_intensity < 1 || gpu_intensity > 31) {
                    std::cerr << "GPU intensity must be between 1 and 31" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            case 'T':
                gpu_threads = std::stoul(optarg);
                break;
            case 'B':
                gpu_blocks = std::stoul(optarg);
                break;
#endif
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

#ifdef GPU_SUPPORT_ENABLED
    // List GPUs and exit
    if (list_gpus_only) {
        list_gpus();
        return EXIT_SUCCESS;
    }
#endif

    // Validate required parameters
    if (address_str.empty()) {
        std::cerr << "Error: Mining address is required" << std::endl;
        std::cerr << "Use --help for usage information" << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize data directory
    if (data_dir.empty()) {
        const char* home = std::getenv("HOME");
        if (home) {
            data_dir = std::string(home) + "/.intcoin";
        } else {
            data_dir = ".intcoin";
        }
    }

    // Print startup banner
    std::cout << "INTcoin Miner v1.3.0" << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core (Maddison Lane)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try {
        // Initialize blockchain and mempool
        if (verbose) {
            std::cout << "Initializing blockchain..." << std::endl;
            std::cout << "Data directory: " << data_dir << std::endl;
        }
        Blockchain blockchain;

        if (verbose) {
            std::cout << "Initializing mempool..." << std::endl;
        }
        Mempool mempool;

        // Parse mining address
        DilithiumPubKey reward_address;
        if (address_str.length() > 0) {
            std::vector<uint8_t> addr_bytes(address_str.begin(), address_str.end());
            Hash256 seed = crypto::SHA3_256::hash(addr_bytes.data(), addr_bytes.size());
            std::memcpy(reward_address.data(), seed.data(),
                       std::min(seed.size(), reward_address.size()));

            if (verbose) {
                std::cout << "Mining to address: " << address_str << std::endl;
            }
        }

        // CPU Miner setup
        std::unique_ptr<Miner> cpu_miner;
        if (num_threads > 0 || !use_gpu) {
            if (verbose) {
                std::cout << "Initializing CPU miner..." << std::endl;
            }
            cpu_miner = std::make_unique<Miner>(blockchain, mempool);

            if (!extra_nonce.empty()) {
                cpu_miner->set_extra_nonce(extra_nonce);
            }

            cpu_miner->set_block_found_callback([&](const Block& block) {
                std::cout << std::endl;
                std::cout << "*** BLOCK FOUND (CPU)! ***" << std::endl;
                std::cout << "Height: " << blockchain.get_height() + 1 << std::endl;
                std::cout << "Hash: " << hash_to_hex(block.get_hash()) << std::endl;
                std::cout << "Nonce: " << block.header.nonce << std::endl;
                std::cout << std::endl;

                if (blockchain.add_block(block)) {
                    std::cout << "Block added to blockchain" << std::endl;
                }
            });
        }

#ifdef GPU_SUPPORT_ENABLED
        // GPU Miner setup
        std::unique_ptr<gpu::GPUMiner> gpu_miner;
        if (use_gpu) {
            if (verbose) {
                std::cout << "Initializing GPU miner..." << std::endl;
                std::cout << "Platform: " << gpu_platform_str << std::endl;
            }

            // Determine platform
            gpu::GPUMinerFactory::Platform platform = gpu::GPUMinerFactory::Platform::AUTO;
            if (gpu_platform_str == "cuda") {
                platform = gpu::GPUMinerFactory::Platform::CUDA;
            } else if (gpu_platform_str == "opencl") {
                platform = gpu::GPUMinerFactory::Platform::OPENCL;
            }

            // Create GPU miner
            gpu_miner = gpu::GPUMinerFactory::create(platform);
            if (!gpu_miner) {
                std::cerr << "Failed to create GPU miner" << std::endl;
                return EXIT_FAILURE;
            }

            // Initialize GPU device(s)
            if (!gpu_miner->initialize(gpu_device_id)) {
                std::cerr << "Failed to initialize GPU miner" << std::endl;
                return EXIT_FAILURE;
            }

            // Configure GPU miner
            gpu::GPUConfig gpu_config;
            gpu_config.intensity = gpu_intensity;
            gpu_config.threads_per_block = gpu_threads;
            gpu_config.blocks_per_grid = gpu_blocks;
            gpu_config.work_group_size = gpu_threads;
            gpu_config.global_work_size = gpu_threads * gpu_blocks;
            gpu_miner->set_config(gpu_config);

            // Set block found callback
            gpu_miner->set_block_found_callback([&](const Block& block, int device_id) {
                std::cout << std::endl;
                std::cout << "*** BLOCK FOUND (GPU " << device_id << ")! ***" << std::endl;
                std::cout << "Height: " << blockchain.get_height() + 1 << std::endl;
                std::cout << "Hash: " << hash_to_hex(block.get_hash()) << std::endl;
                std::cout << "Nonce: " << block.header.nonce << std::endl;
                std::cout << std::endl;

                if (blockchain.add_block(block)) {
                    std::cout << "Block added to blockchain" << std::endl;
                }
            });
        }
#endif

        // Start mining
        std::cout << "Current height: " << blockchain.get_height() << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        std::cout << std::endl;

        bool mining_started = false;

        // Start CPU mining
        if (cpu_miner) {
            if (num_threads == 0) {
                num_threads = std::thread::hardware_concurrency();
                if (num_threads == 0) num_threads = 1;
            }
            std::cout << "Starting CPU miner with " << num_threads << " thread(s)..." << std::endl;

            if (!cpu_miner->start(reward_address, num_threads)) {
                std::cerr << "Failed to start CPU miner" << std::endl;
            } else {
                mining_started = true;
            }
        }

#ifdef GPU_SUPPORT_ENABLED
        // Start GPU mining
        if (gpu_miner) {
            std::cout << "Starting GPU miner on platform: " << gpu_miner->get_platform_name() << std::endl;

            // Create block template
            BlockHeader template_header;
            // (Template would be properly constructed from blockchain state)

            if (!gpu_miner->start_mining(template_header, reward_address)) {
                std::cerr << "Failed to start GPU miner" << std::endl;
            } else {
                mining_started = true;
            }
        }
#endif

        if (!mining_started) {
            std::cerr << "No miners started" << std::endl;
            return EXIT_FAILURE;
        }

        // Main loop - print statistics
        while (!g_shutdown) {
            if (verbose) {
                if (cpu_miner && cpu_miner->is_mining()) {
                    print_stats(cpu_miner->get_stats());
                }

#ifdef GPU_SUPPORT_ENABLED
                if (gpu_miner && gpu_miner->is_mining()) {
                    // Print stats for each GPU device
                    auto devices = gpu_miner->list_devices();
                    for (size_t i = 0; i < devices.size(); i++) {
                        print_gpu_stats(gpu_miner->get_stats(i), i);
                    }
                }
#endif
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Stop mining
        std::cout << std::endl;
        std::cout << "Stopping miners..." << std::endl;

        if (cpu_miner) {
            cpu_miner->stop();
        }

#ifdef GPU_SUPPORT_ENABLED
        if (gpu_miner) {
            gpu_miner->stop_mining();
        }
#endif

        // Print final statistics
        std::cout << std::endl;
        std::cout << "Mining Statistics:" << std::endl;
        std::cout << "==================" << std::endl;

        if (cpu_miner) {
            MiningStats cpu_stats = cpu_miner->get_stats();
            std::cout << "CPU:" << std::endl;
            std::cout << "  Total hashes: " << cpu_stats.total_hashes << std::endl;
            std::cout << "  Blocks found: " << cpu_stats.blocks_found << std::endl;
            std::cout << "  Average hashrate: " << std::fixed << std::setprecision(2)
                      << (cpu_stats.hashes_per_second / 1000000.0) << " MH/s" << std::endl;
        }

#ifdef GPU_SUPPORT_ENABLED
        if (gpu_miner) {
            auto devices = gpu_miner->list_devices();
            for (size_t i = 0; i < devices.size(); i++) {
                auto gpu_stats = gpu_miner->get_stats(i);
                std::cout << "GPU " << i << " (" << devices[i].name << "):" << std::endl;
                std::cout << "  Total hashes: " << gpu_stats.total_hashes << std::endl;
                std::cout << "  Blocks found: " << gpu_stats.blocks_found << std::endl;
                std::cout << "  Average hashrate: " << std::fixed << std::setprecision(2)
                          << (gpu_stats.hashes_per_second / 1000000.0) << " MH/s" << std::endl;
            }
        }
#endif

        std::cout << std::endl;
        std::cout << "Shutdown complete" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
