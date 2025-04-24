# openamp


#### 安装教程

1. 安装依赖工具：
   - Git
   - CMake
   - 交叉编译工具链：aarch64-none-linux-gnu
   - Autotools (autoconf, automake, libtool)

2. 克隆仓库：
   ```bash
   git clone https://gitee.com/your-repo/openamp.git
   cd openamp
   ```

3. 设置文件属性：
   ```bash
   chmod 777 build.sh
   ```

4. 根据需要修改build.sh文件里的平台与工具链：

   ```bash
   SYSTERM=Linux
   TARGET_PLATFORM=aarch64
   TOOLCHAIN=aarch64-none-linux-gnu
   TOOLCHAIN_PATH=/home/litao/RK3568/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin
   ```

#### 使用说明

1. 完整构建所有库：
   ```bash
   ./build.sh
   ```

2. 选择性构建：
   ```bash
   # 仅构建sysfsutils
   ./build.sh sysfsutils/
   
   # 仅构建libhugetlbfs
   ./build.sh libhugetlbfs/
   
   # 仅构建libmetal
   ./build.sh libmetal/
   
   # 仅构建open-amp
   ./build.sh open-amp/
   ```

3. 清理构建结果：
   ```bash
   ./build.sh clean
   ```

4. 查看帮助信息：
   ```bash
   ./build.sh -h
   ```

5. 构建产物：
   - 所有生成的库文件将存放在products目录下
   - 包含以下库文件：
     - libsysfs.so*
     - libhugetlbfs.so
     - libmetal.so*
     - libopen_amp.so*

6. sysfsutils编译

   ```
   autoreconf -ivf
   ./configure --host=\"$TOOLCHAIN\" --prefix=\"$LIBSYSFSUTILS_DIR/build\" ac_cv_func_malloc_0_nonnull=yes 
   make CC=aarch64-none-linux-gnu-gcc"
   ```

7. libhugetlbfs编译

   ```
   ./autogen.sh
   ./configure --host=\"$TOOLCHAIN\" --prefix=\"$LIBHUGETLBFS_DIR\"
   make ARCH=$TARGET_PLATFORM CC=$TOOLCHAIN-gcc LD=$TOOLCHAIN-ld"
   ```

8. libmetal编译

   ```
   cmake ./ -DCMAKE_TOOLCHAIN_FILE=$CUR_DIR/Openamp-config.cmake 
   make VERBOSE=1 DESTDIR=$LIBMETAL_DIR/build install
   ```

9. open-amp编译

   ```
   cmake ./ -DCMAKE_TOOLCHAIN_FILE=$CUR_DIR/Openamp-config.cmake 
   make VERBOSE=1 DESTDIR=$LIBOPENAMP_DIR/build install
   ```

   

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request

