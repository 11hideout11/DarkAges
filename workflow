#!/bin/bash
# ============================================================================
# DarkAges Workflow Command
# ============================================================================
# Reference script for standard development workflows in DarkAges MMO project.
# This script provides one-command access to common operations.
#
# Usage:
#   ./workflow <command> [options]
#
# Commands:
#   build           Build the server (default: Release config)
#   build-debug     Build the server in Debug mode
#   test            Run all test suites
#   clean           Clean build artifacts
#   verify         Build + test (full verification)
#
# Examples:
#   ./workflow build
#   ./workflow test
#   ./workflow verify
#
# Dependencies: cmake, ninja (or make), g++/clang++
# Note: If installed via pip, ensure ~/.local/bin is in PATH
# ============================================================================

set -e

# Project paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Logging functions
log_status() { echo -e "${GREEN}[+]${NC} $1"; }
log_error() { echo -e "${RED}[!]${NC} $1" >&2; }
log_info() { echo -e "${BLUE}[>]${NC} $1"; }

# ============================================================================
# WORKFLOW: build
# ============================================================================
do_build() {
    local config="${1:-Release}"
    
    log_status "Building DarkAges server ($config)..."
    
    # Check prerequisites
    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found. Install: sudo apt install cmake"
        return 1
    fi
    
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        log_error "No C++ compiler found. Install: sudo apt install g++"
        return 1
    fi
    
    # Configure with CMake
    log_info "Configuring CMake..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    
    cd "$BUILD_DIR"
    
    local cmake_args=(
        ".."
        "-DCMAKE_BUILD_TYPE=$config"
        "-DBUILD_TESTS=ON"
        "-DBUILD_SHARED_LIBS=OFF"
        "-DENABLE_GNS=OFF"
        "-DENABLE_REDIS=OFF"
        "-DENABLE_SCYLLA=OFF"
    )
    
    # Use Ninja if available, otherwise Make
    if command -v ninja &> /dev/null; then
        cmake_args+=("-G" "Ninja")
    else
        cmake_args+=("-G" "Unix Makefiles")
    fi
    
    # Add policy for older GLM versions compatibility
    cmake_args+=("-DCMAKE_POLICY_VERSION_MINIMUM=3.5")
    
    cmake "${cmake_args[@]}"
    
    # Build
    local jobs=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if command -v ninja &> /dev/null; then
        ninja -j"$jobs"
    else
        make -j"$jobs"
    fi
    
    cd "$PROJECT_ROOT"
    
    log_status "Build complete: $BUILD_DIR"
}

# ============================================================================
# WORKFLOW: test
# ============================================================================
do_test() {
    log_status "Running test suites..."
    
    if [[ ! -d "$BUILD_DIR" ]]; then
        log_error "No build found. Run './workflow build' first."
        return 1
    fi
    
    cd "$BUILD_DIR"
    
    # Run all tests with CTest
    ctest --output-on-failure -j1
    
    local result=$?
    cd "$PROJECT_ROOT"
    
    if [[ $result -eq 0 ]]; then
        log_status "All tests passed!"
    else
        log_error "Tests failed with exit code: $result"
    fi
    
    return $result
}

# ============================================================================
# WORKFLOW: clean
# ============================================================================
do_clean() {
    log_status "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    log_info "Cleaned: $BUILD_DIR"
}

# ============================================================================
# WORKFLOW: verify (build + test)
# ============================================================================
do_verify() {
    log_status "Running full verification (build + test)..."
    
    do_build Release
    do_test
    
    return $?
}

# ============================================================================
# Main
# ============================================================================
show_help() {
    head -n 25 "$0" | tail -n 21
}

COMMAND="${1:-build}"
shift || true

case "$COMMAND" in
    build)
        do_build Release "$@"
        ;;
    build-debug)
        do_build Debug "$@"
        ;;
    test)
        do_test
        ;;
    clean)
        do_clean
        ;;
    verify)
        do_verify
        ;;
    -h|--help|help)
        show_help
        ;;
    *)
        log_error "Unknown command: $COMMAND"
        show_help
        exit 1
        ;;
esac