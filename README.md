## openamp_core 文件夹说明

`openamp_core` 文件夹包含与 OpenAMP 用户代码及配置相关的内容，是项目的核心部分之一。以下是该文件夹的主要功能和结构：

### 功能
- 提供用户自定义的 OpenAMP 配置和实现。
- 包含多核通信的具体代码逻辑。
- 支持项目的构建和运行。

### 结构
```
openamp_core/
├── build/					# 构建输出目录
├── openamp/				# OpenAMP 框架相关代码
├── openamp_demo/			# 示例代码
├── operation_interface/	# 操作接口实现
├── products/				# 相关可执行文件
├── scripts/				# 脚本文件
```

### 使用说明
1. 根据需要自行修改**./build.sh**和**./openamp/build.sh**编译参数相关配置：

```
SYSTERM=Linux
TARGET_PLATFORM=loongarch
TOOLCHAIN=loongarch64-linux-gnu
TOOLCHAIN_PATH=/opt/loongson-gnu-toolchain-8.3.2k1000la-x86_64-loongarch64-linux-gnu-rc1.1a/bin
```

2. 根据需要修改用户代码./openamp_demo/openamp_app.c相关配置：

```c
#define AMP_DEV_NAME                     "/dev/hw_internuclear"
#define AMP_SHARE_MEMORY_ADDRESS         0x99000000
#define AMP_SHARE_MEMORY_PHYSICS_ADDRESS 0x8000000099000000
#define AMP_SHARE_MEMORY_SIZE            0x10000
#define AMP_VDEV_STATUS_SIZE             0x1000
#define AMP_VDEV_ALIGN_SIZE              0x1000
#define AMP_DEMO_VRING_TX_DES_NUM        16
#define AMP_DEMO_VRING_RX_DES_NUM        16

#define AMP_AECONDARY_FIRMWARE           "./rtthread.bin"
#define AMP_SLAVE_CPU_ID                 1
#define AMP_SLAVE_ENTRY                  0x97000000
#define AMP_SLAVE_IRQ_ID                 7
```

3. 运行构建脚本生成可执行文件：

​	**默认构建用户逻辑，可加参数[drive|openamp]分别构建平台驱动和openamp框架动态库**

```bash
#第一次构建需依次输入以下命令
./build.sh openamp
./build.sh drive
#后续修改app，只需输入
./build.sh
```

4. 构建完成后，可在**products**文件夹查看生成文件：

```bash
Generated files in /openamp_core/products:
internuclear.ko  openamp_demo  openamplibs  rtthread_amp_s.bin  tftp.sh
```

### 注意事项
- 修改配置文件后需要重新构建项目。
- 确保依赖工具（如 CMake 和 GCC）已正确安装。
- **linux**需要使能 **Hugepage** 支持，输入**./build.sh menuconfig** 进入配置界面。

```
1. 使能 Memory Management options->Transparent Hugepage Support
2. File systems->Pseudo filesystems->HugeTLB file system support
```

- 对于共享内存和固件内存，最好让**linux**不映射该段内存，如果使用设备树需要添加相关节点，反之则需查看与修改linux的内存映射配置。

```
	reserved-memory {
		#address-cells = <2>;
		#size-cells = <2>;
		ranges;

		amp_core_reserved: amp-core@2800000 {
			reg = <0x0 0x2800000 0x0 0x400000>;
			no-map;
		};

		openamp_reserved: openamp@70000000 {
			reg = <0x0 0x70000000 0x0 0x400000>;
			no-map;
		};
	};
```

