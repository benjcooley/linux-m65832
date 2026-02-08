#!/bin/bash
#
# docker-build.sh - Build M65832 Linux kernel using Docker
#
# This avoids macOS host tool compatibility issues by using
# a Linux container with proper build dependencies.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="$(dirname "$(dirname "$SCRIPT_DIR")")"
# Canonical toolchain location: m65832/bin/
TOOLCHAIN_DIR="${KERNEL_DIR}/../m65832"

IMAGE_NAME="m65832-linux-builder"
CONTAINER_NAME="m65832-build"

# Build Docker image
build_image() {
    echo "Building Docker image..."
    docker build \
        --build-arg UID=$(id -u) \
        --build-arg GID=$(id -g) \
        -t "$IMAGE_NAME" \
        "$SCRIPT_DIR"
}

# Run command in container
run_in_container() {
    docker run --rm \
        -v "${KERNEL_DIR}:/build/kernel" \
        -v "${TOOLCHAIN_DIR}:/build/m65832" \
        -w /build/kernel \
        "$IMAGE_NAME" \
        "$@"
}

# Build kernel in container
do_build() {
    local target="${1:-}"
    
    echo "Building M65832 kernel in Docker container..."
    
    run_in_container bash -c "
        gmake ARCH=m65832 LLVM=1 LLVM_IAS=1 \
            CC=/build/m65832/bin/clang \
            LD=/build/m65832/bin/ld.lld \
            AR=/build/m65832/bin/llvm-ar \
            NM=/build/m65832/bin/llvm-nm \
            STRIP=/build/m65832/bin/llvm-strip \
            OBJCOPY=/build/m65832/bin/llvm-objcopy \
            OBJDUMP=/build/m65832/bin/llvm-objdump \
            READELF=/build/m65832/bin/llvm-readelf \
            -j\$(nproc) \
            $target
    "
}

# Show help
show_help() {
    echo "Docker-based M65832 Linux Kernel Build"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  image       Build the Docker image"
    echo "  config      Run defconfig"
    echo "  menuconfig  Run menuconfig"  
    echo "  build       Build the kernel"
    echo "  clean       Clean build"
    echo "  shell       Open shell in container"
    echo "  help        Show this help"
    echo ""
    echo "Example:"
    echo "  $0 image && $0 config && $0 build"
}

# Main
case "${1:-help}" in
    image)
        build_image
        ;;
    config|defconfig)
        do_build defconfig
        ;;
    menuconfig)
        run_in_container bash -c "gmake ARCH=m65832 menuconfig"
        ;;
    build)
        do_build
        ;;
    clean)
        do_build clean
        ;;
    shell)
        docker run --rm -it \
            -v "${KERNEL_DIR}:/build/kernel" \
            -v "${TOOLCHAIN_DIR}:/build/m65832" \
            -w /build/kernel \
            "$IMAGE_NAME" \
            bash
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
