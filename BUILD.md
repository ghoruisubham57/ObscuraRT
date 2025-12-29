# Build Instructions

## Quick Start

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./obscura_rt
```

## Detailed Setup

### 1. Install Dependencies

#### Linux (Ubuntu 22.04)

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    vulkan-tools libvulkan-dev vulkan-headers \
    libglfw3-dev \
    libopencv-dev \
    glslc
```

#### macOS

```bash
brew install cmake glfw vulkan-loader vulkan-headers opencv glslang
```

#### Windows (MSVC + vcpkg)

```bash
# With vcpkg installed:
vcpkg install vulkan:x64-windows glfw3:x64-windows opencv:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="[vcpkg]/scripts/buildsystems/vcpkg.cmake"
```

### 2. Clone & Configure

```bash
cd ObscuraRT
mkdir -p build
cd build
```

### 3. Build

```bash
# Debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) VERBOSE=1

# Release (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 4. Validation

```bash
# Check Vulkan availability
vulkaninfo

# Run with validation layers
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./obscura_rt
```

### 5. Troubleshooting

#### "glslc not found"

```bash
# Install Vulkan SDK from https://vulkan.lunarg.com/
# Or use your package manager's glslang
sudo apt-get install glslang-tools
```

#### "Vulkan libraries not found"

```bash
# Set library path explicitly
export LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:$LD_LIBRARY_PATH
```

#### GPU not detected

```bash
# Check GPU drivers
lspci | grep -i vga
vulkaninfo | grep "GPU"
```

---

## Development Workflow

### Build & Test Loop

```bash
cd build
cmake .. && make -j$(nproc) && ./obscura_rt
```

### Shader Recompilation

```bash
# Manual compilation (CMake does this)
glslc ../shaders/pixelation.comp -o shaders/pixelation.comp.spv

# Or trigger full rebuild
cmake .. && make clean && make -j$(nproc)
```

### Enable Address Sanitizer (ASan)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make -j$(nproc)
```

---

## Performance Profiling

### GPU Timeline

```bash
# Using RenderDoc
renderdoc ./obscura_rt
```

### CPU Profile

```bash
# With perf
perf record -g ./obscura_rt
perf report
```

---

## CI/CD Setup (Optional)

### GitHub Actions

```yaml
name: Build ObscuraRT
on: [push]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install deps
        run: sudo apt-get install -y vulkan-tools libvulkan-dev vulkan-headers glslc libglfw3-dev libopencv-dev
      - name: Build
        run: mkdir build && cd build && cmake .. && make -j$(nproc)
```

---
