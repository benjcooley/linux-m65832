#!/bin/bash
#
# build-m65832.sh - Build Linux kernel for M65832 architecture
#
# Usage:
#   ./scripts/build-m65832.sh [config|build|clean|menuconfig]
#
# Requirements:
#   - LLVM-M65832 toolchain built at ../llvm-m65832/build/
#   - GNU Make 4.0+ (gmake on macOS)
#   - On macOS: brew install make
#
# macOS Note:
#   Building Linux host tools on macOS has compatibility issues.
#   For reliable builds on macOS, use Docker:
#     ./scripts/docker/docker-build.sh image
#     ./scripts/docker/docker-build.sh build
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="$(dirname "$SCRIPT_DIR")"
LLVM_DIR="${KERNEL_DIR}/../llvm-m65832/build/bin"

# Check for required tools
check_requirements() {
    echo "Checking requirements..."
    
    # Check for LLVM toolchain
    if [ ! -x "${LLVM_DIR}/clang" ]; then
        echo "ERROR: LLVM-M65832 toolchain not found at ${LLVM_DIR}"
        echo "Please build LLVM-M65832 first."
        exit 1
    fi
    
    # Check M65832 target support
    if ! "${LLVM_DIR}/clang" --print-targets 2>/dev/null | grep -q m65832; then
        echo "ERROR: LLVM does not have M65832 target support"
        exit 1
    fi
    
    # Check for GNU Make 4.0+
    if command -v gmake &>/dev/null; then
        MAKE=gmake
    elif make --version 2>/dev/null | head -1 | grep -q "GNU Make [4-9]"; then
        MAKE=make
    else
        echo "ERROR: GNU Make 4.0+ required"
        echo "On macOS: brew install make"
        exit 1
    fi
    
    echo "  LLVM: ${LLVM_DIR}/clang"
    echo "  Make: ${MAKE}"
    echo "  Kernel: ${KERNEL_DIR}"
    echo ""
}

# Create clang wrapper with M65832 target
create_wrapper() {
    local wrapper="${LLVM_DIR}/m65832-linux-clang"
    if [ ! -x "$wrapper" ]; then
        echo "Creating M65832 clang wrapper..."
        cat > "$wrapper" << 'WRAPPER'
#!/bin/bash
exec "$(dirname "$0")/clang" --target=m65832-unknown-linux "$@"
WRAPPER
        chmod +x "$wrapper"
    fi
}

# Setup macOS compatibility headers (if needed)
setup_macos_compat() {
    if [[ "$(uname)" == "Darwin" ]]; then
        local compat_dir="${KERNEL_DIR}/scripts/include"
        mkdir -p "$compat_dir"
        
        # Create byteswap.h if missing  
        if [ ! -f "$compat_dir/byteswap.h" ]; then
            echo "Creating macOS byteswap compatibility header..."
            cat > "$compat_dir/byteswap.h" << 'EOF'
#ifndef _BYTESWAP_H
#define _BYTESWAP_H
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)
#endif
EOF
        fi
        
        echo ""
        echo "WARNING: Building on macOS has compatibility issues with host tools."
        echo "For reliable builds, use Docker: ./scripts/docker/docker-build.sh"
        echo ""
    fi
}

# Build kernel using make
do_make() {
    local target="${1:-}"
    local compat_dir="${KERNEL_DIR}/scripts/include"
    
    # Base make arguments  
    $MAKE ARCH=m65832 LLVM=1 LLVM_IAS=1 \
        CC="${LLVM_DIR}/m65832-linux-clang" \
        LD="${LLVM_DIR}/ld.lld" \
        AR="${LLVM_DIR}/llvm-ar" \
        NM="${LLVM_DIR}/llvm-nm" \
        STRIP="${LLVM_DIR}/llvm-strip" \
        OBJCOPY="${LLVM_DIR}/llvm-objcopy" \
        OBJDUMP="${LLVM_DIR}/llvm-objdump" \
        READELF="${LLVM_DIR}/llvm-readelf" \
        HOSTCC="/usr/bin/clang -I${compat_dir}" \
        HOSTCXX="/usr/bin/clang++ -I${compat_dir}" \
        $target
}

# Run defconfig
do_config() {
    echo "Configuring kernel..."
    cd "$KERNEL_DIR"
    do_make defconfig
    echo ""
    echo "Configuration complete. Run '$0 build' to compile."
}

# Run menuconfig
do_menuconfig() {
    echo "Running menuconfig..."
    cd "$KERNEL_DIR"
    do_make menuconfig
}

# Build kernel
do_build() {
    echo "Building kernel..."
    cd "$KERNEL_DIR"
    
    # Check if configured
    if [ ! -f ".config" ]; then
        echo "No .config found. Running defconfig first..."
        do_config
    fi
    
    # Build with parallel jobs
    JOBS=${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}
    echo "Using $JOBS parallel jobs"
    
    do_make "-j$JOBS"
    
    echo ""
    echo "Build complete!"
    if [ -f "vmlinux" ]; then
        echo "Kernel: vmlinux"
        ls -la vmlinux
    fi
}

# Clean build
do_clean() {
    echo "Cleaning build..."
    cd "$KERNEL_DIR"
    do_make clean
}

# Deep clean
do_mrproper() {
    echo "Deep cleaning build..."
    cd "$KERNEL_DIR"
    $MAKE ARCH=m65832 mrproper 2>/dev/null || {
        # Manual clean if mrproper fails (common on macOS)
        rm -rf .config include/config include/generated arch/m65832/include/generated
        rm -rf scripts/*.o scripts/mod/*.o scripts/basic/*.o
        rm -rf vmlinux System.map
        echo "Manual clean complete"
    }
}

# Show help
show_help() {
    echo "M65832 Linux Kernel Build Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  config      Configure kernel with defconfig"
    echo "  menuconfig  Run interactive menuconfig"
    echo "  build       Build the kernel (default)"
    echo "  clean       Clean build files"
    echo "  mrproper    Deep clean (removes .config)"
    echo "  help        Show this help"
    echo ""
    echo "Environment variables:"
    echo "  JOBS        Number of parallel build jobs (default: auto)"
    echo ""
    echo "Example:"
    echo "  $0 config && $0 build"
}

# Main
main() {
    check_requirements
    create_wrapper
    setup_macos_compat
    
    case "${1:-build}" in
        config|defconfig)
            do_config
            ;;
        menuconfig)
            do_menuconfig
            ;;
        build)
            do_build
            ;;
        clean)
            do_clean
            ;;
        mrproper)
            do_mrproper
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            echo "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
