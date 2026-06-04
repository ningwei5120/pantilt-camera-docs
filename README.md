# Autolabor C1 云台相机 ROS 驱动

> **版本**: v1.0  
> **适用系统**: Ubuntu 20.04 + ROS Noetic  
> **最后更新**: 2026-06-04

本仓库包含 Autolabor C1 云台相机的 **完整 ROS 驱动源码** 与 **部署使用文档**，方便在其他环境快速部署与分享使用。

---

## 仓库结构

```
.
├── README.md                          # 本文件（总览）
├── docs/                              # 使用文档
│   ├── QUICK_START.md                 # 5 分钟快速上手指南
│   ├── USER_MANUAL.md                 # 完整使用说明书
│   └── DEPLOYMENT.md                  # 详细部署与故障排查文档
├── src/                               # ROS 源码（3 个包）
│   ├── cv_camera/                     # 相机驱动（OpenCV + 标定参数）
│   ├── pantilt_camera_serial_driver/  # 云台串口控制（包名: pantilt_camera_serial）
│   └── rviz_pantilt_plugin/           # RViz 可视化插件
├── scripts/
│   └── setup_device_permissions.sh    # 设备权限一键设置脚本
└── assets/
    └── Autolabor C1 用户手册.pdf      # 官方硬件用户手册
```

---

## 包含的软件包

| 包名 | 功能 | 说明 |
|------|------|------|
| `cv_camera` | 相机图像采集 + 标定 | USB 摄像头驱动，内置 1280×720 / 1920×1080 / 3840×2160 标定参数 |
| `pantilt_camera_serial` | 云台串口控制 | UART (115200 8N1) 通信，支持角度查询、速度控制、模式切换 |
| `rviz_pantilt_plugin` | RViz 可视化插件 | GUI 面板，支持键盘/按钮/输入框实时控制云台 |

---

## 30 秒极速预览

```bash
# 1. 安装依赖
sudo apt-get install ros-noetic-image-proc ros-noetic-image-view

# 2. 编译（在工作空间根目录）
cd ~/catkin_ws
catkin_make
source devel/setup.bash

# 3. 设置设备权限
sudo ./scripts/setup_device_permissions.sh

# 4. 一键启动（自动检测摄像头和串口）
roslaunch pantilt_camera_serial pantiltcamera_ready.launch

# 5. 控制云台（新终端）
rosrun pantilt_camera_serial pantilt_tool.py info      # 查看角度
rosrun pantilt_camera_serial pantilt_tool.py center    # 回中
```

---

## 文档导航

| 文档 | 面向读者 | 内容 |
|------|----------|------|
| [`docs/QUICK_START.md`](docs/QUICK_START.md) | 首次使用者 | 极简流程：安装 → 配置 → 启动 → 控制 |
| [`docs/DEPLOYMENT.md`](docs/DEPLOYMENT.md) | 运维/部署人员 | 完整环境搭建、编译、设备配置、故障排查 |
| [`docs/USER_MANUAL.md`](docs/USER_MANUAL.md) | 日常操作者 | 所有控制方式详解（命令行工具、RViz 插件、ROS 接口） |

---

## 技术规格

- **ROS 版本**: ROS Noetic (或兼容版本)
- **编程语言**: C++17 / Python3
- **构建系统**: Catkin
- **主要依赖**: OpenCV, Boost.Asio, Qt5, image_transport, cv_bridge

**通信协议**
- 串口参数: 115200 baud, 8N1
- 协议头: 0xAA (发送) / 0x55 (接收)
- 校验: CRC8 (多项式 0xD5)
- 角度范围:
  - Heading (航向): [-160°, 160°]
  - Roll (横滚): [-40°, 40°]
  - Pitch (俯仰): [-90°, 90°]

---

## 常见问题速查

| 现象 | 快速解决 |
|------|----------|
| 找不到摄像头 | `ls /dev/v4l/by-path` 检查连接；确认权限 `sudo chmod a+rw /dev/video*` |
| 打不开串口 | `ls /dev/ttyUSB*` 检查连接；执行 `sudo usermod -a -G dialout $USER` 后重新登录 |
| 图像有畸变 | 确认启动了 `image_proc` 节点，并订阅 `image_rect_color` 话题 |
| RViz 不显示 | `source ~/catkin_ws/devel/setup.bash` 后重新启动 RViz |
| 串口断开 | 直接重启 launch 文件即可恢复连接 |

详细排查步骤请参考 [`docs/DEPLOYMENT.md`](docs/DEPLOYMENT.md) 的「故障排查」章节。

---

## 附注

- 标定数据中的原始标定图像（`.gz` / `.tar.gz`）因体积过大未纳入版本控制，仓库中仅保留 `.yaml` 标定参数文件。
- 如需查看完整硬件说明，请参阅 [`assets/Autolabor C1 用户手册.pdf`](assets/Autolabor%20C1%20用户手册.pdf)。
