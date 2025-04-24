#!/bin/bash
set -euo pipefail

TARGET_PLATFORM="10.25.8.115"
PRODUCTS_DIR="./openamplibs"
CORE_FILES=("internuclear.ko" "openamp_demo" "rtthread.bin")
LIB_FILES=(
    "libhugetlbfs.so"
    "libmetal.so" "libmetal.so.1" "libmetal.so.1.7.0"
    "libopen_amp.so" "libopen_amp.so.1" "libopen_amp.so.1.7.0"
    "libsysfs.so" "libsysfs.so.2" "libsysfs.so.2.0.1"
)

graceful_exit() {
    echo -e "\033[31mERROR: $1\033[0m" >&2
    exit 1
}

tftp_download() {
    echo -e "\033[34m[TFTP] Downloading $1...\033[0m"
    if ! tftp -g -r "$1" "$TARGET_PLATFORM"; then
        graceful_exit "download failed: $1"
    fi
}

download_core() {
    for file in "${CORE_FILES[@]}"; do
        tftp_download "$file"
    done
}

download_libs() {
    for lib in "${LIB_FILES[@]}"; do
        tftp_download "${PRODUCTS_DIR}/${lib}"
    done
}

load_kernel_module() {
    echo -e "\033[32m[KERNEL] Load the kernel module $1...\033[0m"
    if ! insmod "$1"; then
        graceful_exit "Module loading failed.: $1"
    fi
}

start_slave_core() {
    echo -e "\033[36m[CORE] Start the slave core $1...\033[0m"
    if ! ./"$1"; then
        graceful_exit "Failed to boot from the core: $1"
    fi
}

case "${1:-}" in
    "libs")
        download_libs
        echo -e "\033[1;32m[OK] The library file download has been completed\033[0m"
        ;;
    "core")
        download_core
        echo -e "\033[1;32m[OK] The core components have been downloaded successfully\033[0m"
        ;;
    "all")
        download_libs
        download_core
        echo -e "\033[1;32m[OK] All components have been downloaded successfully\033[0m"
        ;;
    "boot")
        load_kernel_module "internuclear.ko"
        start_slave_core "openamp_demo"
        echo -e "\033[1;32m[OK] The system started successfully.\033[0m"
        ;;
    *)
        cat <<EOF

\033[1;33musage: $0 [command]\033[0m

Available command:
  libs       - Only download the library files
  core       - Download the core components
  all        - Download all the files
  boot       - Start the slave system

example:
  $0 core     # Download the core components
  $0 libs     # Only download the library files
  $0 all      # Download all the files
  $0 boot     # Start the slave system

EOF
        exit 1
        ;;
esac