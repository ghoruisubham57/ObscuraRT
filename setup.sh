#!/bin/bash

# Setup ObscuraRT development environment

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[Setup] ObscuraRT Project Initialize"
echo "[Setup] Root: $PROJECT_ROOT"

# 1. Check prerequisites
echo "[Setup] Checking prerequisites..."

for cmd in cmake vulkaninfo glslc; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "[WARN] $cmd not found. Install Vulkan SDK and tools."
    fi
done

# 2. Download GLM (header-only math library)
echo "[Setup] Setting up third-party libraries..."

mkdir -p "$PROJECT_ROOT/third_party"

if [ ! -d "$PROJECT_ROOT/third_party/glm" ]; then
    echo "[Setup] Cloning GLM..."
    cd "$PROJECT_ROOT/third_party"
    git clone --depth 1 https://github.com/g-truc/glm.git
    echo "[Setup] GLM ready"
else
    echo "[Setup] GLM already present"
fi

# 3. Create build directory
echo "[Setup] Creating build directory..."

mkdir -p "$PROJECT_ROOT/build"

# 4. Run CMake
echo "[Setup] Configuring CMake..."

cd "$PROJECT_ROOT/build"
cmake -DCMAKE_BUILD_TYPE=Release ..

echo ""
echo "============================================"
echo "[Setup] Complete!"
echo "============================================"
echo ""
echo "Next steps:"
echo "  cd $PROJECT_ROOT/build"
echo "  make -j$(nproc)"
echo "  ./obscura_rt"
echo ""
