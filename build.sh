#!/bin/bash
set -e  # 启用“出错即退出”模式

# 当前目录
CUR_DIR=$(pwd)

# 目标平台和工具链
SYSTERM=Linux
TARGET_PLATFORM=loongarch
TOOLCHAIN=loongarch64-linux-gnu
TOOLCHAIN_PATH=/opt/loongson-gnu-toolchain-8.3.2k1000la-x86_64-loongarch64-linux-gnu-rc1.1a/bin
# 库的生成目录
LIB_PRODUCTS=$CUR_DIR/products
OPENAMP_PATH=$CUR_DIR/openamp

# 库的源码目录
LIBSYSFSUTILS_DIR=$OPENAMP_PATH/sysfsutils
LIBHUGETLBFS_DIR=$OPENAMP_PATH/libhugetlbfs
LIBMETAL_DIR=$OPENAMP_PATH/libmetal
LIBOPENAMP_DIR=$OPENAMP_PATH/open-amp
LIBDOCKING_DIR=$CUR_DIR/operation_interface/

# 设置环境变量
export PATH=$TOOLCHAIN_PATH:$PATH
export SYSTERM_NAME=$SYSTERM
export PLATFORM_NAME=$TARGET_PLATFORM
export TOOLCHAIN_NAME=$TOOLCHAIN
export LIBSYSFS_PATH=$LIBSYSFSUTILS_DIR
export LIBHUGETLBFS_PATH=$LIBHUGETLBFS_DIR
export LIBMETAL_PATH=$LIBMETAL_DIR
export LIBOPENAMP_PATH=$LIBOPENAMP_DIR
export LIBDOCKING_PATH=$LIBDOCKING_DIR

# 创建生成目录
mkdir -p $LIB_PRODUCTS
mkdir -p $LIB_PRODUCTS/openamplibs
echo "SYSTERM: $SYSTERM"
echo "TARGET_PLATFORM: $TARGET_PLATFORM"
echo "TOOLCHAIN: $TOOLCHAIN"
echo "TOOLCHAIN_PATH: $TOOLCHAIN_PATH"

if [ "$1" == "drive" ]; then
    cd ../intercore_driver
    ./build.sh $2
    cd ../openamp_core
elif [ "$1" == "openamp" ]; then
    cd ./openamp
    ./build.sh
    cd ..
else
    echo -e "\033[1;32mUsage: $0 [drive|openamp]\033[0m"
fi

rm -rf build
cmake ./ -B build/ -DCMAKE_TOOLCHAIN_FILE=./Openamp-config.cmake
cd ./build || exit
make

cp -u ../../intercore_driver/internuclear.ko ../products
cp -u ./openamp_demo ../products
cp -u ../scripts/tftp.sh ../products
cp -u ../openamp/products/* ../products/openamplibs

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