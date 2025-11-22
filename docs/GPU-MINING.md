# GPU Mining Guide

**INTcoin GPU Mining**
**Version**: 1.3.0+
**Last Updated**: January 2025

---

## Table of Contents

- [Overview](#overview)
- [Supported Platforms](#supported-platforms)
- [Requirements](#requirements)
- [Building with GPU Support](#building-with-gpu-support)
- [Hardware Compatibility](#hardware-compatibility)
- [Getting Started](#getting-started)
- [Configuration](#configuration)
- [Performance Tuning](#performance-tuning)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)

---

## Overview

INTcoin supports GPU mining through two platforms:

- **CUDA**: For NVIDIA GPUs (GeForce, Quadro, Tesla)
- **OpenCL**: For AMD GPUs, Intel GPUs, and other OpenCL-compatible devices

GPU mining provides significantly higher hash rates compared to CPU mining, making it more efficient for finding blocks.

### Performance Comparison

| Hardware | Hash Rate (approx) | Power Usage |
|----------|-------------------|-------------|
| Intel i7-13700K (CPU) | 600 KH/s | 125W |
| AMD Ryzen 9 7950X (CPU) | 1.2 MH/s | 170W |
| NVIDIA RTX 3060 (GPU) | 45 MH/s | 170W |
| NVIDIA RTX 3080 (GPU) | 95 MH/s | 320W |
| NVIDIA RTX 4090 (GPU) | 180 MH/s | 450W |
| AMD RX 6800 XT (GPU) | 70 MH/s | 300W |
| AMD RX 7900 XTX (GPU) | 110 MH/s | 355W |

*Note: Actual performance varies based on configuration, cooling, and mining intensity.*

---

## Supported Platforms

### CUDA (NVIDIA)

**Minimum Requirements:**
- NVIDIA GPU with Compute Capability 5.2+ (Maxwell architecture or newer)
- CUDA Toolkit 11.0 or later
- NVIDIA Driver 450.x or later

**Recommended:**
- CUDA Toolkit 12.x
- Latest NVIDIA drivers

**Supported GPUs:**
- GeForce GTX 900 series and newer
- GeForce RTX 20/30/40 series
- Quadro RTX series
- Tesla V100, A100

### OpenCL (AMD, Intel, Others)

**Minimum Requirements:**
- OpenCL 1.2 compatible GPU
- OpenCL drivers installed

**Supported GPUs:**
- AMD Radeon RX 400 series and newer
- Intel Arc GPUs
- Intel integrated GPUs (UHD, Iris)
- Any OpenCL 1.2+ compatible device

---

## Requirements

### CUDA Platform

**Ubuntu/Debian:**
```bash
# Install NVIDIA drivers
sudo apt install nvidia-driver-535

# Install CUDA Toolkit
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt update
sudo apt install cuda-toolkit-12-3

# Verify installation
nvidia-smi
nvcc --version
```

**macOS:**
```bash
# CUDA is not supported on macOS (Apple Silicon or Intel)
# Use OpenCL instead
```

**Windows:**
1. Download CUDA Toolkit from [NVIDIA website](https://developer.nvidia.com/cuda-downloads)
2. Install NVIDIA GeForce Experience or Studio Drivers
3. Verify installation: `nvcc --version`

### OpenCL Platform

**Ubuntu/Debian:**
```bash
# For AMD GPUs
sudo apt install mesa-opencl-icd ocl-icd-opencl-dev

# For NVIDIA GPUs (alternative to CUDA)
sudo apt install nvidia-opencl-dev

# For Intel GPUs
sudo apt install intel-opencl-icd
```

**macOS:**
```bash
# OpenCL is built into macOS
# No additional installation required
```

**Windows:**
- AMD: Install AMD Adrenalin drivers
- Intel: Install Intel Graphics drivers
- NVIDIA: Install GeForce drivers (includes OpenCL support)

---

## Building with GPU Support

### Build Options

Enable GPU support during compilation:

```bash
# CUDA only
cmake -DENABLE_GPU_CUDA=ON ..

# OpenCL only
cmake -DENABLE_GPU_OPENCL=ON ..

# Both CUDA and OpenCL
cmake -DENABLE_GPU_CUDA=ON -DENABLE_GPU_OPENCL=ON ..
```

### Full Build Example (Ubuntu + CUDA)

```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Install dependencies
sudo apt install build-essential cmake git ccache \
    libboost-all-dev libssl-dev librocksdb-dev libhwloc-dev

# Install CUDA (if not already installed)
sudo apt install cuda-toolkit-12-3

# Create build directory
mkdir build && cd build

# Configure with CUDA support
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GPU_CUDA=ON \
    -DBUILD_MINER=ON

# Build
make -j$(nproc)

# Install
sudo make install
```

### Full Build Example (Ubuntu + OpenCL)

```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Install dependencies
sudo apt install build-essential cmake git ccache \
    libboost-all-dev libssl-dev librocksdb-dev libhwloc-dev \
    mesa-opencl-icd ocl-icd-opencl-dev

# Create build directory
mkdir build && cd build

# Configure with OpenCL support
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GPU_OPENCL=ON \
    -DBUILD_MINER=ON

# Build
make -j$(nproc)

# Install
sudo make install
```

---

## Hardware Compatibility

### Check GPU Support

List all available GPU devices:

```bash
intcoin-miner --list-gpus
```

Example output:
```
Available GPU Devices:
=====================

Device 0: NVIDIA GeForce RTX 3080
  Platform:       CUDA
  Vendor:         NVIDIA
  Global Memory:  10240 MB
  Local Memory:   48 KB
  Compute Units:  68
  Clock Speed:    1710 MHz
  Available:      Yes

Device 1: AMD Radeon RX 6800 XT
  Platform:       OpenCL
  Vendor:         AMD
  Global Memory:  16384 MB
  Local Memory:   64 KB
  Compute Units:  72
  Available:      Yes
```

### Verify CUDA Installation

```bash
# Check NVIDIA driver
nvidia-smi

# Check CUDA compiler
nvcc --version

# Test CUDA samples (optional)
cd /usr/local/cuda/samples/1_Utilities/deviceQuery
make
./deviceQuery
```

### Verify OpenCL Installation

```bash
# Install clinfo
sudo apt install clinfo

# List OpenCL platforms and devices
clinfo
```

---

## Getting Started

### Basic GPU Mining

**Auto-detect GPU platform:**
```bash
intcoin-miner --address INT1qw508d6qejxtdg4y5r3zarvary0c5xw7k --gpu
```

**Specify CUDA platform:**
```bash
intcoin-miner --address <your_address> --gpu --gpu-platform cuda
```

**Specify OpenCL platform:**
```bash
intcoin-miner --address <your_address> --gpu --gpu-platform opencl
```

### Select Specific GPU Device

```bash
# Use GPU device 0
intcoin-miner --address <your_address> --gpu --gpu-device 0

# Use GPU device 1
intcoin-miner --address <your_address> --gpu --gpu-device 1
```

### Hybrid CPU + GPU Mining

Mine with both CPU and GPU simultaneously:

```bash
# 4 CPU threads + GPU
intcoin-miner --address <your_address> --threads 4 --gpu

# 2 CPU threads + GPU device 0
intcoin-miner --address <your_address> --threads 2 --gpu --gpu-device 0
```

---

## Configuration

### Mining Intensity

Control GPU power usage and hash rate with intensity (1-31):

```bash
# Low intensity (power saving)
intcoin-miner --address <your_address> --gpu --gpu-intensity 15

# Medium intensity (balanced)
intcoin-miner --address <your_address> --gpu --gpu-intensity 20

# High intensity (maximum performance)
intcoin-miner --address <your_address> --gpu --gpu-intensity 28
```

**Intensity Guidelines:**

| Intensity | Power Usage | Performance | Use Case |
|-----------|-------------|-------------|----------|
| 10-15 | 40-60% | Low | Power saving, background mining |
| 16-20 | 60-80% | Medium | Balanced, daily mining |
| 21-25 | 80-95% | High | Dedicated mining |
| 26-31 | 95-100% | Maximum | Maximum profit, good cooling required |

### Advanced Configuration

**CUDA-specific options:**

```bash
# Threads per block (default: 256)
intcoin-miner --address <your_address> --gpu --gpu-threads 512

# Blocks per grid (default: 8192)
intcoin-miner --address <your_address> --gpu --gpu-blocks 16384

# Combined
intcoin-miner --address <your_address> --gpu \
    --gpu-threads 256 \
    --gpu-blocks 16384 \
    --gpu-intensity 24
```

**OpenCL-specific options:**

```bash
# Work group size (default: 256)
intcoin-miner --address <your_address> --gpu \
    --gpu-platform opencl \
    --gpu-threads 256
```

---

## Performance Tuning

### Optimal Settings by GPU

#### NVIDIA RTX 3080
```bash
intcoin-miner --address <your_address> --gpu \
    --gpu-platform cuda \
    --gpu-intensity 24 \
    --gpu-threads 256 \
    --gpu-blocks 16384
```
Expected: ~95 MH/s @ 320W

#### NVIDIA RTX 4090
```bash
intcoin-miner --address <your_address> --gpu \
    --gpu-platform cuda \
    --gpu-intensity 26 \
    --gpu-threads 512 \
    --gpu-blocks 20480
```
Expected: ~180 MH/s @ 450W

#### AMD RX 6800 XT
```bash
intcoin-miner --address <your_address> --gpu \
    --gpu-platform opencl \
    --gpu-intensity 22 \
    --gpu-threads 256
```
Expected: ~70 MH/s @ 300W

#### AMD RX 7900 XTX
```bash
intcoin-miner --address <your_address> --gpu \
    --gpu-platform opencl \
    --gpu-intensity 24 \
    --gpu-threads 256
```
Expected: ~110 MH/s @ 355W

### Overclocking (Advanced)

**NVIDIA (Linux):**
```bash
# Enable persistence mode
sudo nvidia-smi -pm 1

# Set power limit (example: 250W)
sudo nvidia-smi -pl 250

# Set GPU clock offset (+100 MHz)
nvidia-settings -a "[gpu:0]/GPUGraphicsClockOffset[3]=100"

# Set memory clock offset (+1000 MHz)
nvidia-settings -a "[gpu:0]/GPUMemoryTransferRateOffset[3]=1000"
```

**AMD (Linux with CoreCtrl):**
```bash
# Install CoreCtrl
sudo apt install corectrl

# Launch CoreCtrl GUI
corectrl
```

---

## Troubleshooting

### Common Issues

#### "No CUDA devices found"

**Solutions:**
1. Check NVIDIA driver installation:
   ```bash
   nvidia-smi
   ```
2. Verify CUDA toolkit:
   ```bash
   nvcc --version
   ```
3. Rebuild with `-DENABLE_GPU_CUDA=ON`

#### "No OpenCL platforms found"

**Solutions:**
1. Install OpenCL drivers:
   ```bash
   # AMD
   sudo apt install mesa-opencl-icd

   # NVIDIA
   sudo apt install nvidia-opencl-dev

   # Intel
   sudo apt install intel-opencl-icd
   ```
2. Verify with clinfo:
   ```bash
   clinfo
   ```

#### Low Hash Rate

**Possible causes:**
- Intensity too low → Increase `--gpu-intensity`
- Thermal throttling → Improve cooling
- Power limit → Increase power limit
- Outdated drivers → Update GPU drivers

#### GPU Overheating

**Solutions:**
1. Reduce intensity:
   ```bash
   --gpu-intensity 18
   ```
2. Improve airflow
3. Set power limit:
   ```bash
   sudo nvidia-smi -pl 200  # 200W limit
   ```
4. Clean GPU cooling system

#### Out of Memory Errors

**Solutions:**
1. Reduce threads/blocks:
   ```bash
   --gpu-threads 128 --gpu-blocks 4096
   ```
2. Close other GPU applications
3. Reduce intensity

---

## FAQ

### Can I mine with multiple GPUs?

Currently, each miner instance supports one GPU. To mine with multiple GPUs, run multiple instances:

```bash
# Terminal 1: GPU 0
intcoin-miner --address <your_address> --gpu --gpu-device 0

# Terminal 2: GPU 1
intcoin-miner --address <your_address> --gpu --gpu-device 1
```

Multi-GPU support in a single instance is planned for v1.4.0.

### Does GPU mining wear out my GPU?

Mining creates sustained load on your GPU, which can accelerate wear. However, with proper cooling and reasonable power limits, modern GPUs can mine safely for years.

**Best practices:**
- Keep temperatures below 75°C
- Use power limits (90% or less)
- Ensure good airflow
- Clean dust regularly

### Can I use my computer while GPU mining?

Yes, but reduce mining intensity for better responsiveness:

```bash
intcoin-miner --address <your_address> --gpu --gpu-intensity 15
```

Alternatively, mine with CPU while using GPU for desktop:
```bash
intcoin-miner --address <your_address> --threads 4
```

### Which is better: CUDA or OpenCL?

**CUDA advantages:**
- Better performance on NVIDIA GPUs
- More mature ecosystem
- Better tooling and profiling

**OpenCL advantages:**
- Cross-vendor compatibility
- Works on AMD, Intel, and NVIDIA
- More portable

**Recommendation**: Use CUDA for NVIDIA GPUs, OpenCL for AMD/Intel.

### Can I mine on laptops?

Yes, but with caution:
- Use low intensity (10-15)
- Monitor temperatures closely
- Ensure laptop has good ventilation
- Consider only mining when plugged in
- Avoid mining on thin/ultraportable laptops

### What about profitability?

Calculate your profitability using:
- Current network difficulty
- Your hash rate
- Block reward (105,113,636 INT)
- Electricity cost
- Hardware cost

Example calculator: https://international-coin.org/calculator

---

## Performance Monitoring

Monitor your GPU while mining:

```bash
# NVIDIA
watch -n 1 nvidia-smi

# AMD (requires radeontop)
sudo apt install radeontop
radeontop
```

Monitor temperatures and adjust intensity if GPU exceeds:
- **Warning**: 75°C
- **Critical**: 80°C
- **Emergency shutdown**: 85°C

---

## Support

**Issues:** https://gitlab.com/intcoin/crypto/-/issues
**Email:** team@international-coin.org
**Wiki:** https://gitlab.com/intcoin/crypto/-/wikis/GPU-Mining

---

**Copyright © 2025 INTcoin Core (Maddison Lane)**
**License**: MIT
