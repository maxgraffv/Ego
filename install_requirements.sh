#!/bin/bash
set -e

UBUNTU_VERSION=$(lsb_release -rs)
ARCH=$(uname -m)
IS_JETSON=false

if [ -f /etc/nv_tegra_release ]; then
    IS_JETSON=true
fi

echo "========================================"
echo " Ego — Requirements Installer"
echo " Ubuntu : $UBUNTU_VERSION"
echo " Arch   : $ARCH"
echo " Jetson : $IS_JETSON"
echo "========================================"

# ----------------------------------------
# Build tools
# ----------------------------------------
echo ""
echo "[1/5] Build tools..."
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    curl

# ----------------------------------------
# Audio
# ----------------------------------------
echo ""
echo "[2/5] Audio libraries..."
sudo apt install -y \
    libasound2-dev \
    libsndfile1-dev \
    libmpg123-dev

# ----------------------------------------
# Qt — Qt6 on Ubuntu 22.04+, Qt5 otherwise
# ----------------------------------------
echo ""
echo "[3/5] Qt..."
if dpkg --compare-versions "$UBUNTU_VERSION" ge "22.04"; then
    echo "  -> Qt6 (Ubuntu $UBUNTU_VERSION)"
    sudo apt install -y \
        qt6-base-dev \
        libqt6widgets6
else
    echo "  -> Qt5 (Ubuntu $UBUNTU_VERSION — Qt6 unavailable)"
    sudo apt install -y \
        qtbase5-dev \
        libqt5widgets5
fi

# ----------------------------------------
# Intel RealSense
# ----------------------------------------
echo ""
echo "[4/5] Intel RealSense..."

if [ "$IS_JETSON" = true ] || [ "$ARCH" = "aarch64" ]; then
    echo "  -> ARM/Jetson detected — building librealsense2 from source"
    echo "     (this will take ~20 minutes)"

    sudo apt install -y \
        libusb-1.0-0-dev \
        libssl-dev \
        libgtk-3-dev \
        libglfw3-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev

    REALSENSE_VERSION="2.55.1"
    REALSENSE_DIR="/tmp/librealsense2"

    if [ ! -d "$REALSENSE_DIR" ]; then
        git clone --depth 1 --branch "v$REALSENSE_VERSION" \
            https://github.com/IntelRealSense/librealsense.git \
            "$REALSENSE_DIR"
    fi

    mkdir -p "$REALSENSE_DIR/build"
    cd "$REALSENSE_DIR/build"

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_GRAPHICAL_EXAMPLES=OFF \
        -DBUILD_WITH_CUDA=ON \
        -DFORCE_RSUSB_BACKEND=ON

    make -j$(nproc)
    sudo make install
    cd -

    echo "  -> librealsense2 installed from source"
else
    echo "  -> x86_64 — using Intel apt repo"

    sudo apt-key adv \
        --keyserver keyserver.ubuntu.com \
        --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCD || \
    sudo apt-key adv \
        --keyserver hkp://keyserver.ubuntu.com:80 \
        --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCD

    sudo add-apt-repository \
        "deb https://librealsense.intel.com/Debian/apt-repo $(lsb_release -cs) main" -u

    sudo apt install -y \
        librealsense2-dkms \
        librealsense2-utils \
        librealsense2-dev
fi

# ----------------------------------------
# CUDA — already in JetPack on Jetson
# ----------------------------------------
echo ""
echo "[5/5] CUDA..."
if [ "$IS_JETSON" = true ]; then
    echo "  -> Jetson: CUDA included in JetPack — skipping"
    echo "     Make sure JetPack is installed: sudo apt install nvidia-jetpack"
else
    if ! command -v nvcc &> /dev/null; then
        echo "  -> CUDA not found — install manually from https://developer.nvidia.com/cuda-downloads"
        echo "     or: sudo apt install nvidia-cuda-toolkit"
    else
        CUDA_VER=$(nvcc --version | grep release | awk '{print $6}' | cut -d',' -f1)
        echo "  -> CUDA $CUDA_VER found"
    fi
fi

# ----------------------------------------
# Build
# ----------------------------------------
echo ""
echo "========================================"
echo " All requirements installed."
echo ""
echo " To build:"
echo "   mkdir -p build && cd build"
echo "   cmake .. && cmake --build . -j\$(nproc)"
echo "========================================"
