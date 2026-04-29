#!/bin/bash
# CMake Installer for DarkAges Development Environment
# Downloads and installs cmake 3.28.3 to /home/openhands/cmake

set -e

CMAKE_VERSION="3.28.3"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh"
INSTALL_DIR="/home/openhands/cmake"

echo "Installing CMake ${CMAKE_VERSION} to ${INSTALL_DIR}..."

mkdir -p "${INSTALL_DIR}"

# Download cmake installer
cd /tmp
if [ ! -f "cmake-${CMAKE_VERSION}-linux-x86_64.sh" ]; then
    echo "Downloading CMake..."
    wget -q "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh" -O "cmake-${CMAKE_VERSION}-linux-x86_64.sh"
fi

chmod +x "cmake-${CMAKE_VERSION}-linux-x86_64.sh"

# Extract cmake
echo "Extracting..."
"./cmake-${CMAKE_VERSION}-linux-x86_64.sh" --prefix="${INSTALL_DIR}" --skip-license

# Verify
export PATH="${INSTALL_DIR}/bin:$PATH"
echo "CMake installed: $(cmake --version)"
echo "Done!"