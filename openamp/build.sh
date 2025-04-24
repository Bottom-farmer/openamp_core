#!/bin/bash
set -e  # 启用“出错即退出”模式

# 当前目录
CUR_DIR=$(pwd)

# 目标平台和工具链
SYSTERM=Linux
# TARGET_PLATFORM=aarch64
# TOOLCHAIN=aarch64-none-linux-gnu
# TOOLCHAIN_PATH=/home/bigdog/share/tools/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/bin
TARGET_PLATFORM=loongarch64
TOOLCHAIN=loongarch64-linux-gnu
TOOLCHAIN_PATH=/opt/loongson-gnu-toolchain-8.3.2k1000la-x86_64-loongarch64-linux-gnu-rc1.1a/bin
# 库的生成目录
LIB_PRODUCTS=$CUR_DIR/products

# 库的源码目录
LIBSYSFSUTILS_DIR=$CUR_DIR/sysfsutils
LIBHUGETLBFS_DIR=$CUR_DIR/libhugetlbfs
LIBMETAL_DIR=$CUR_DIR/libmetal
LIBOPENAMP_DIR=$CUR_DIR/open-amp

# 设置环境变量
export PATH=$TOOLCHAIN_PATH:$PATH
export SYSTERM_NAME=$SYSTERM
export PLATFORM_NAME=$TARGET_PLATFORM
export TOOLCHAIN_NAME=$TOOLCHAIN
export LIBSYSFS_PATH=$LIBSYSFSUTILS_DIR
export LIBHUGETLBFS_PATH=$LIBHUGETLBFS_DIR
export LIBMETAL_PATH=$LIBMETAL_DIR

# 创建生成目录
mkdir -p $LIB_PRODUCTS
echo "SYSTERM: $SYSTERM"
echo "TARGET_PLATFORM: $TARGET_PLATFORM"
echo "TOOLCHAIN: $TOOLCHAIN"
echo "TOOLCHAIN_PATH: $TOOLCHAIN_PATH"

# 检查源码目录是否存在，如果不存在则克隆仓库
check_source_dir_is_exist() {
    local repo_urls=(
        "https://github.com/libhugetlbfs/libhugetlbfs.git"
        "https://github.com/linux-ras/sysfsutils.git"
        "https://github.com/OpenAMP/libmetal.git"
        "https://github.com/OpenAMP/open-amp.git"
    )
    local repo_dirs=(
        "$LIBHUGETLBFS_DIR"
        "$LIBSYSFSUTILS_DIR"
        "$LIBMETAL_DIR"
        "$LIBOPENAMP_DIR"
    )

    for i in "${!repo_dirs[@]}"; do
        if [ ! -d "${repo_dirs[$i]}" ]; then
            echo "Directory ${repo_dirs[$i]} does not exist. Cloning repository..."
            git clone "${repo_urls[$i]}" "${repo_dirs[$i]}"
            if [ $? -eq 0 ]; then
                echo "Repository cloned successfully to ${repo_dirs[$i]}."
            else
                echo "Failed to clone repository of ${repo_dirs[$i]}."
                exit 1
            fi
        fi
    done
}

# 编译库的通用函数
compile_library() {
    local lib_name=$1
    local lib_dir=$2
    local build_cmd=$3
    local install_cmd=$4
    local copy_cmd=$5

    echo "Building $lib_name..."
    cd "$lib_dir" || exit 1
    eval "$build_cmd"
    if [ $? -eq 0 ]; then
        eval "$install_cmd"
        if [ $? -eq 0 ]; then
            eval "$copy_cmd"
            echo "$lib_name compiled and installed successfully!"
        else
            echo "Failed to install $lib_name!"
            exit 1
        fi
    else
        echo "Failed to compile $lib_name!"
        exit 1
    fi
}

# 编译 sysfsutils
compile_sysfsutils() {
    compile_library "sysfsutils" "$LIBSYSFSUTILS_DIR" \
        "rm -rf ./build && mkdir -p ./build && autoreconf -ivf && ./configure --host=\"$TOOLCHAIN\" --prefix=\"$LIBSYSFSUTILS_DIR/build\" ac_cv_func_malloc_0_nonnull=yes && make CC=$TOOLCHAIN-gcc" \
        "make install" \
        "cp $LIBSYSFSUTILS_DIR/build/lib/libsysfs.so* $LIB_PRODUCTS"
}

# 编译 libhugetlbfs
compile_libhugetlbfs() {
    compile_library "libhugetlbfs" "$LIBHUGETLBFS_DIR" \
        "./autogen.sh && ./configure --host=\"$TOOLCHAIN\" --prefix=\"$LIBHUGETLBFS_DIR\" && make ARCH=$TARGET_PLATFORM CC=$TOOLCHAIN-gcc LD=$TOOLCHAIN-ld" \
        ":" \
        "cp ./obj64/libhugetlbfs.so $LIB_PRODUCTS"
}

# 编译 libmetal
compile_libmetal() {
    compile_library "libmetal" "$LIBMETAL_DIR" \
        "rm -rf CMakeCache.txt CMakeFiles build && mkdir -p build && cmake ./ -DCMAKE_TOOLCHAIN_FILE=$CUR_DIR/Openamp-config.cmake && make VERBOSE=1" \
        "make DESTDIR=$LIBMETAL_DIR/build install" \
        "cp $LIBMETAL_DIR/build/usr/local/lib/libmetal.so* $LIB_PRODUCTS"
}

# 编译 open-amp
compile_openamp() {
    compile_library "open-amp" "$LIBOPENAMP_DIR" \
        "rm -rf CMakeCache.txt CMakeFiles build && mkdir -p build && cmake ./ -DCMAKE_TOOLCHAIN_FILE=$CUR_DIR/Openamp-config.cmake && make VERBOSE=1" \
        "make DESTDIR=$LIBOPENAMP_DIR/build install" \
        "cp $LIBOPENAMP_DIR/build/usr/local/lib/libopen_amp.so* $LIB_PRODUCTS"
}

# 清理编译结果
clean() {
    echo "Cleaning build directories..."
    # 清除 sysfsutils
    cd "$LIBSYSFSUTILS_DIR" && make clean && make uninstall && rm -rf ./build
    # 清除 libhugetlbfs
    cd "$LIBHUGETLBFS_DIR" && make clean
    # 清除 libmetal
    cd "$LIBMETAL_DIR" && make clean && rm -rf ./build
    # 清除 open-amp
    cd "$LIBOPENAMP_DIR" && make clean && rm -rf ./build
    # 清除生成文件
    rm -rf $LIB_PRODUCTS
}

# 显示帮助信息
show_help() {
    echo "Instructions: $0 <Options>"
    echo "Options:"
    echo "  sysfsutils/    - Compile only sysfsutils"
    echo "  libhugetlbfs/  - Compile only libhugetlbfs"
    echo "  libmetal/      - Compile only libmetal"
    echo "  open-amp/      - Compile only open-amp"
    echo "  clean          - Clean all build directories"
    echo "  -h, --help        - Show this help message"
}

# 主逻辑
if [ "$#" -eq 0 ]; then
    check_source_dir_is_exist
    compile_sysfsutils
    compile_libhugetlbfs
    compile_libmetal
    compile_openamp
else
    case "$1" in
        sysfsutils/)
            check_source_dir_is_exist
            compile_sysfsutils
            ;;
        libhugetlbfs/)
            check_source_dir_is_exist
            compile_libhugetlbfs
            ;;
        libmetal/)
            check_source_dir_is_exist
            compile_libmetal
            ;;
        open-amp/)
            check_source_dir_is_exist
            compile_openamp
            ;;
        clean)
            clean
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Invalid option: $1"
            show_help
            exit 1
            ;;
    esac
fi

# 打印成功消息
echo "------------------------------------------------------------------------"
echo -e "\033[1;32m"
echo "   ____                      _      _                              _ _ "
echo "  / ___|___  _ __ ___  _ __ | | ___| |_ ___    ___ _   _ _ __   __| | |"
echo " | |   / _ \| '_ \` _ \| '_ \| |/ _ \ __/ __|  / __| | | | '_ \ / _\` | |"
echo " | |__| (_) | | | | | | |_) | |  __/ |_\__ \ | (__| |_| | | | | (_| |_|"
echo "  \____\___/|_| |_| |_| .__/|_|\___|\__|___/  \___|\__,_|_| |_|\__,_(_)"
echo "                      |_|"
echo -e "\033[0m"
echo "------------------------------------------------------------------------"
echo "Generated files in $LIB_PRODUCTS:"
ls $LIB_PRODUCTS