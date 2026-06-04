# 云台相机详细部署文档

> 本文档面向需要在全新环境部署云台相机驱动的运维人员或开发者，涵盖从零开始的完整步骤。

---

## 目录

1. [系统要求](#系统要求)
2. [安装依赖](#安装依赖)
3. [获取源代码](#获取源代码)
4. [编译工作空间](#编译工作空间)
5. [硬件连接与设备识别](#硬件连接与设备识别)
6. [配置设备权限](#配置设备权限)
7. [Launch 文件说明](#launch-文件说明)
8. [启动与验证](#启动与验证)
9. [单独组件测试](#单独组件测试)
10. [故障排查](#故障排查)

---

## 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Ubuntu 20.04 LTS |
| ROS 版本 | ROS Noetic Ninjemys |
| C++ 标准 | C++17 |
| 构建工具 | Catkin (catkin_make 或 catkin_tools) |

---

## 安装依赖

```bash
# 更新软件源
sudo apt-get update

# ROS 图像处理包（必须）
sudo apt-get install ros-noetic-image-proc

# 图像查看工具（可选，用于调试）
sudo apt-get install ros-noetic-image-view
```

---

## 获取源代码

将以下三个 ROS 包放入你的工作空间 `src` 目录：

```
catkin_ws/src/
├── cv_camera/                     # 相机驱动
├── pantilt_camera_serial_driver/  # 云台串口控制（ROS包名: pantilt_camera_serial）
└── rviz_pantilt_plugin/           # RViz 可视化插件
```

> 若从本项目的压缩包恢复，解压后确保目录结构如上。

---

## 编译工作空间

```bash
cd ~/catkin_ws

# 方式一：catkin_make（推荐）
catkin_make

# 方式二：catkin_tools
catkin build

# 配置环境
source devel/setup.bash
```

建议将 source 命令加入 `~/.bashrc`，避免每次手动执行：

```bash
echo "source ~/catkin_ws/devel/setup.bash" >> ~/.bashrc
```

---

## 硬件连接与设备识别

### 1. 连接检查清单

- [ ] 云台相机 USB 数据线已连接电脑
- [ ] 云台电源已开启
- [ ] 无其他程序占用串口（如 minicom、screen）

### 2. 查看设备

```bash
# 所有视频设备
ls /dev/video*

# 稳定的 by-path 路径（推荐用于配置）
ls /dev/v4l/by-path/

# 串口设备
ls /dev/ttyUSB* /dev/ttyACM*

# 确认 Wasintek 相机被识别
lsusb | grep -i wasintek
```

### 3. 常见设备路径

| 设备 | 常见路径 | 备注 |
|------|----------|------|
| 内置摄像头 | `/dev/video0`, `/dev/video1` | 笔记本自带 |
| Wasintek 相机 | `/dev/video2`, `/dev/video3` | Autolabor C1 云台相机 |
| 云台串口 | `/dev/ttyUSB0` | CH340/CP2102 等 USB 转串口 |

> **提示**: 使用 `/dev/v4l/by-path/` 下的路径比 `/dev/videoX` 更稳定，插拔后不会改变。

---

## 配置设备权限

### 方法一：一键脚本（推荐）

本文档包已包含 `scripts/setup_device_permissions.sh`：

```bash
cd pantilt_camera_docs
sudo ./scripts/setup_device_permissions.sh
```

脚本会自动完成以下操作：
1. 将当前用户加入 `dialout` 组
2. 设置所有 `/dev/ttyUSB*` 权限为 `a+rw`
3. 设置所有 `/dev/video*` 权限为 `a+rw`
4. 列出检测到的设备信息

### 方法二：手动设置

```bash
# 加入 dialout 组（只需执行一次，需重新登录生效）
sudo usermod -a -G dialout $USER

# 临时授权（每次重新插拔后可能需要重新执行）
sudo chmod a+rw /dev/ttyUSB0
sudo chmod a+rw /dev/video0
```

---

## Launch 文件说明

所有 launch 文件位于 `pantilt_camera_serial/launch/` 目录。

| 文件 | 功能 | 适用场景 |
|------|------|----------|
| `pantiltcamera_ready.launch` | 全自动检测 + 自检 + 回中 | **日常首选**，自动识别摄像头和串口 |
| `pantiltcamera.launch` | 手动配置的传统启动文件 | 设备路径固定且已知 |
| `demo_test.launch` | 综合测试，带调试输出 | 问题排查 |
| `test_camera_only.launch` | 仅启动相机 + image_proc | 不依赖云台硬件，单独测试图像 |
| `test_pantilt_only.launch` | 仅启动云台串口节点 | 不依赖摄像头，单独测试云台 |
| `view_camera.launch` | 仅显示相机图像 | 快速查看画面 |

### pantiltcamera_ready.launch 参数详解

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `device_path` | `""` | 摄像头路径，留空则自动检测 `/dev/v4l/by-path` |
| `port_name` | `""` | 串口路径，留空则自动探测 `/dev/ttyUSB*`、`/dev/ttyACM*` |
| `camera_info_url` | `1920x1080.yaml` | 标定文件路径 |
| `image_width` | `1920` | 图像宽度 |
| `image_height` | `1080` | 图像高度 |
| `rate` | `30` | 图像发布频率 (Hz) |
| `use_rviz` | `false` | 是否启动 RViz |
| `use_image_view` | `false` | 是否启动图像显示窗口 |
| `timeout` | `15.0` | 自检等待超时（秒） |
| `center_timeout` | `20.0` | 回正等待超时（秒） |
| `tolerance` | `2.0` | 回正角度容差（°） |
| `skip_center_wait` | `false` | 是否跳过等待回正到位 |

---

## 启动与验证

### 步骤 1：启动系统

```bash
roslaunch pantilt_camera_serial pantiltcamera_ready.launch
```

### 步骤 2：观察自检输出

正常情况应依次看到：

```
[resolve_device_path] INFO: 自动检测到摄像头: /dev/v4l/by-path/...
[resolve_serial_port] INFO: 探测 /dev/ttyUSB0 成功 — 云台协议验证通过
[自检] 服务 /pantilt_demo/send_command 已上线
[自检] 话题 /cv_camera_demo/image_raw 数据正常
[自检] 话题 /pantilt_demo/pantilt_angle_info 数据正常
[自检] 云台已回正: H=0.12, R=-0.05, P=0.08
==================================================
自检通过: 相机与云台均已就绪
==================================================
```

### 步骤 3：验证控制

另开终端：

```bash
source ~/catkin_ws/devel/setup.bash

# 查看角度
rosrun pantilt_camera_serial pantilt_tool.py info

# 回中测试
rosrun pantilt_camera_serial pantilt_tool.py center
```

---

## 单独组件测试

当完整系统启动异常时，建议先隔离测试单个组件。

### 相机单独测试（不依赖云台）

```bash
roslaunch pantilt_camera_serial test_camera_only.launch device_path:=/dev/video2
```

验证项：
- 图像是否正常采集
- `image_proc` 是否发布矫正后图像
- `rostopic hz /cv_camera_test/image_raw` 是否有正常帧率

### 云台单独测试（不依赖相机）

```bash
roslaunch pantilt_camera_serial test_pantilt_only.launch
```

验证项：
- `rostopic echo /pantilt_test/pantilt_angle_info` 是否有角度数据
- 服务调用是否响应：
  ```bash
  rosservice call /pantilt_test/send_command "command_name: 'BackToCenter' data: []"
  ```
- 速度控制是否生效：
  ```bash
  rostopic pub /pantilt_test/pantilt_vel geometry_msgs/Twist "linear:
    x: 1.0
    y: 0.0
    z: 0.0
  angular:
    x: 0.0
    y: 0.0
    z: 1.0"
  ```

---

## 故障排查

### Q1: 无法打开摄像头

**症状**：
```
[ERROR] Cannot open camera device
```

**排查步骤**：
1. `ls /dev/video*` 确认设备存在
2. `ls -l /dev/video0` 确认当前用户有读写权限
3. 运行 `sudo ./scripts/setup_device_permissions.sh`
4. 若使用 `pantiltcamera_ready.launch`，检查终端输出中的 `resolve_device_path` 日志
5. 手动指定路径测试：
   ```bash
   roslaunch pantilt_camera_serial pantiltcamera_ready.launch device_path:=/dev/video2
   ```

---

### Q2: 无法连接云台串口

**症状**：
```
[ERROR] Failed to open serial port
```

**排查步骤**：
1. `ls /dev/ttyUSB* /dev/ttyACM*` 确认串口设备存在
2. 确认当前用户在 `dialout` 组：
   ```bash
   groups $USER | grep dialout
   ```
   若无输出，执行 `sudo usermod -a -G dialout $USER` 并**重新登录**
3. 检查串口是否被其他进程占用：
   ```bash
   lsof /dev/ttyUSB0
   ```
4. 手动指定串口测试：
   ```bash
   roslaunch pantilt_camera_serial pantiltcamera_ready.launch port_name:=/dev/ttyUSB1
   ```

---

### Q3: 自动检测未找到摄像头

```bash
# 检查系统是否识别到视频设备
ls /dev/v4l/by-path
# 如果没有任何输出，请检查 USB 连接和摄像头供电
```

---

### Q4: 自动探测未找到云台串口

```bash
# 检查系统是否识别到串口设备
ls /dev/ttyUSB* /dev/ttyACM*

# 如果存在多个串口但探测均失败，可能是其他串口占用了 ttyUSB0
# 此时 launch 会自动扫描并定位到正确的串口；若仍失败，可手动指定 port_name
```

---

### Q5: 图像畸变矫正不生效

**症状**：矫正后图像与原始图像看起来相同

**排查步骤**：
1. 确认标定文件存在：
   ```bash
   ls $(rospack find cv_camera)/calibrationdata/
   ```
2. 确认 `camera_info` 已发布：
   ```bash
   rostopic echo /cv_camera_demo/camera_info
   ```
3. 确认 `image_proc` 节点正在运行：
   ```bash
   rosnode list | grep image_proc
   ```
4. 检查矫正后话题：
   ```bash
   rostopic list | grep image_rect
   ```

---

### Q6: RViz 无法显示图像或插件

**排查步骤**：
1. 确认已 source 工作空间：
   ```bash
   source ~/catkin_ws/devel/setup.bash
   ```
2. 检查图像话题是否正常发布：
   ```bash
   rostopic hz /cv_camera_demo/image_raw
   ```
3. 使用 `image_view` 单独测试图像：
   ```bash
   rosrun image_view image_view image:=/cv_camera_demo/image_raw
   ```
4. 重新编译 `rviz_pantilt_plugin`：
   ```bash
   cd ~/catkin_ws && catkin_make --pkg rviz_pantilt_plugin
   ```

---

### Q7: 终端仍有 Corrupt JPEG 刷屏

`pantiltcamera_ready.launch` 已内置 `filter_jpeg.sh` 过滤该警告。如果仍出现：
- 确认使用的是最新版 launch 文件
- 检查是否通过其他方式（如直接运行 `cv_camera_node`）启动了相机节点

---

### Q8: pantilt_tool.py 提示找不到服务

- 确认 launch 文件已正确启动
- 确认 `-n` 参数与节点命名空间一致（`pantiltcamera_ready.launch` 默认使用 `pantilt_demo`，无需指定）
- 使用 `rosservice list | grep send_command` 查看实际服务名

---

## 附录：自动检测原理

`pantiltcamera_ready.launch` 使用以下脚本实现自动检测：

| 脚本 | 功能 |
|------|------|
| `resolve_device_path.py` | 扫描 `/dev/v4l/by-path`，过滤出 `*-video-index0` 主视频流设备，校验可读性后输出路径 |
| `resolve_serial_port.py` | 扫描可用串口，对每个候选串口以 115200 8N1 打开，发送云台 `GetPantiltPose` 查询命令，验证返回帧的协议头和 CRC8 校验 |
| `filter_jpeg.sh` | 通过 `sed` 实时过滤 stderr 中的 `Corrupt JPEG` 刷屏信息 |
| `pantilt_init_check.py` | 等待服务上线、发送回正命令、验证图像流和角度话题、等待云台回正到位 |
