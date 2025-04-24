# TFTP 自动化部署工具

.
├── openamplibs/              # 库文件存储目录
│   ├── libhugetlbfs.so
│   ├── libmetal.*
│   └── ...
├── internuclear.ko          # 内核模块
├── openamp_demo             # 从核系统可执行文件
└── rtthread.bin             # 实时系统固件

## 功能概述
本脚本提供一站式部署解决方案，主要包含以下功能：
- **文件下载**：通过 TFTP 协议批量获取二进制文件
- **内核模块管理**：动态加载内核驱动程序
- **从核系统启动**：初始化协处理器环境
- **色彩化输出**：不同操作类型使用不同颜色标识

## 环境要求
- `tftp-hpa` 客户端 (v5.2+)
- `insmod` 内核模块加载工具
- Bash 4.0+ 环境
- 可访问的 TFTP 服务器（当前配置地址：`10.25.8.115`）

## 快速开始
```bash
# 下载核心组件
./tftp.sh core

# 仅下载库文件
./tftp.sh libs

# 完整部署（库文件+核心组件）
./tftp.sh all

# 启动从核系统
./tftp.sh boot