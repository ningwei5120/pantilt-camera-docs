# Autolabor C1 云台相机 ROS 驱动 — 部署与使用文档包

> **版本**: v1.0  
> **适用系统**: Ubuntu 20.04 + ROS Noetic  
> **最后更新**: 2026-06-04

---

## 文档包说明

本文档包整理了 Autolabor C1 云台相机在 ROS 环境下的**完整部署流程**与**日常使用说明**，方便你在新机器上快速部署，或将使用方法分享给他人。

| 文档 | 面向读者 | 内容 |
|------|----------|------|
| [`QUICK_START.md`](QUICK_START.md) | 首次使用者 | 5 分钟极简上手：安装 → 配置 → 启动 → 控制 |
| [`DEPLOYMENT.md`](DEPLOYMENT.md) | 运维/部署人员 | 完整环境搭建、编译、设备配置、故障排查 |
| [`USER_MANUAL.md`](USER_MANUAL.md) | 日常操作者 | 所有控制方式详解（命令行工具、RViz 插件、ROS 接口） |

---

## 项目简介

Autolabor C1 云台相机 ROS 驱动包含 **3 个核心软件包**：

| 包名 | 功能 | 说明 |
|------|------|------|
| `cv_camera` | 相机图像采集 + 标定 | 基于 OpenCV 的 USB 摄像头驱动，内置 1280×720 / 1920×1080 / 3840×2160 标定参数 |
| `pantilt_camera_serial` | 云台串口控制 | 通过 UART (115200 8N1) 与云台通信，支持角度查询、速度控制、模式切换 |
| `rviz_pantilt_plugin` | RViz 可视化插件 | 提供 GUI 面板，可用键盘/按钮/输入框实时控制云台 |

**通信协议**
- 串口参数: 115200 baud, 8N1
- 协议头: 0xAA (发送) / 0x55 (接收)
- 校验: CRC8 (多项式 0xD5)
- 角度范围:
  - Heading (航向): [-160°, 160°]
  - Roll (横滚): [-40°, 40°]
  - Pitch (俯仰): [-90°, 90°]

---

## 目录结构

```
pantilt_camera_docs/
├── README.md              # 本文档（总览）
├── QUICK_START.md         # 快速上手指南
├── DEPLOYMENT.md          # 详细部署文档
├── USER_MANUAL.md         # 完整使用说明书
├── scripts/
│   └── setup_device_permissions.sh   # 设备权限一键设置脚本
└── launch_examples/
    └── pantiltcamera_ready.launch    # 推荐启动文件（带自动检测）
```

---

## 30 秒极速预览

```bash
# 1. 安装依赖
sudo apt-get install ros-noetic-image-proc ros-noetic-image-view

# 2. 编译（在工作空间根目录）
cd ~/catkin_ws && catkin_make && source devel/setup.bash

# 3. 设置设备权限
sudo ./scripts/setup_device_permissions.sh

# 4. 一键启动（自动检测摄像头和串口）
roslaunch pantilt_camera_serial pantiltcamera_ready.launch

# 5. 控制云台（新终端）
rosrun pantilt_camera_serial pantilt_tool.py info      # 查看角度
rosrun pantilt_camera_serial pantilt_tool.py center    # 回中
```

---

## 常见问题速查

| 现象 | 快速解决 |
|------|----------|
| 找不到摄像头 | `ls /dev/v4l/by-path` 检查连接；确认权限 `sudo chmod a+rw /dev/video*` |
| 打不开串口 | `ls /dev/ttyUSB*` 检查连接；执行 `sudo usermod -a -G dialout $USER` 后重新登录 |
| 图像有畸变 | 确认启动了 `image_proc` 节点，并订阅 `image_rect_color` 话题 |
| RViz 不显示 | `source ~/catkin_ws/devel/setup.bash` 后重新启动 RViz |
| 串口断开 | 直接重启 launch 文件即可恢复连接 |

详细排查步骤请参考 [`DEPLOYMENT.md`](DEPLOYMENT.md) 的「故障排查」章节。

---

## 附注

- 原始项目源代码位于 `~/catkin_ws/src/` 下，包含三个包：`cv_camera`、`pantilt_camera_serial_driver`（包名 `pantilt_camera_serial`）、`rviz_pantilt_plugin`。
- 如需查看原始用户手册 PDF，请参见源码目录中的 `src/Autolabor C1 用户手册.pdf`。
