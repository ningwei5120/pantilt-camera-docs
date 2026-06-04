# Autolabor C1 云台相机调试指南

本文档提供 Autolabor C1 云台相机的完整调试和测试流程。

## 目录

1. [硬件准备](#硬件准备)
2. [环境配置](#环境配置)
3. [设备权限设置](#设备权限设置)
4. [单独组件测试](#单独组件测试)
5. [综合功能测试](#综合功能测试)
6. [常见问题排查](#常见问题排查)

---

## 硬件准备

### 1. 检查设备连接

```bash
# 查看所有视频设备
ls /dev/video*

# 查看by-path设备（稳定路径）
ls /dev/v4l/by-path/

# 查看串口设备（连接云台后）
ls /dev/ttyUSB*

# 查找 Wasintek 相机
lsusb | grep -i wasintek
```

### 2. 设备路径说明

| 设备 | 常见路径 | 说明 |
|------|----------|------|
| 内置摄像头 | `/dev/video0`, `/dev/video1` | 笔记本自带摄像头 |
| Wasintek相机 | `/dev/video2`, `/dev/video3` | Autolabor C1 云台相机 |
| 云台串口 | `/dev/ttyUSB0` | 云台控制串口 |

### 3. 切换相机设备

如果默认设备不对，使用参数指定：

```bash
# 使用 /dev/video2 (Wasintek相机)
roslaunch pantilt_camera_serial pantiltcamera_auto.launch device_path:=/dev/video2

# 使用内置摄像头测试
roslaunch pantilt_camera_serial pantiltcamera_auto.launch device_path:=/dev/video0

# 使用by-path路径（更稳定）
roslaunch pantilt_camera_serial pantiltcamera_auto.launch \
    device_path:=/dev/v4l/by-path/pci-0000:00:14.0-usb-0:13:1.0-video-index0
```

---

## 环境配置

### 1. 安装依赖

```bash
sudo apt-get update
sudo apt-get install ros-noetic-image-proc
sudo apt-get install ros-noetic-image-view
```

### 2. 编译工作空间

```bash
cd ~/catkin_ws
catkin_make
source devel/setup.bash
```

---

## 设备权限设置

### 方法一：使用脚本（推荐）

```bash
cd ~/catkin_ws/src/pantilt_camera_serial_driver/scripts
sudo ./setup_device_permissions.sh
```

### 方法二：手动设置

```bash
# 添加用户到 dialout 组
sudo usermod -a -G dialout $USER

# 设置设备权限
sudo chmod a+rw /dev/ttyUSB0
sudo chmod a+rw /dev/video0

# 重新登录使权限生效
```

---

## 单独组件测试

### 1. 相机单独测试

仅测试摄像头功能，不依赖云台硬件：

```bash
roslaunch pantilt_camera_serial test_camera_only.launch
```

**测试内容：**
- 图像采集是否正常
- 图像畸变矫正是否生效
- 显示原始图像和矫正后图像

### 2. 云台单独测试

仅测试云台串口通信，不依赖摄像头：

```bash
roslaunch pantilt_camera_serial test_pantilt_only.launch
```

**测试内容：**
- 串口连接是否正常
- 云台角度信息是否正确发布
- 服务调用是否响应

**常用测试命令：**

```bash
# 查看云台角度
rostopic echo /pantilt_test/pantilt_angle_info

# 云台回中
rosservice call /pantilt_test/send_command "command_name: 'BackToCenter' data: []"

# 设置锁定模式
rosservice call /pantilt_test/send_command "command_name: 'SetLockMode' data: []"

# 速度控制（俯仰1.0，航向1.0）
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

## 综合功能测试

### 启动完整系统

```bash
# 方式1：使用主 launch 文件
roslaunch pantilt_camera_serial pantiltcamera.launch

# 方式2：使用 demo 测试 launch（推荐）
roslaunch pantilt_camera_serial demo_test.launch

# 方式3：启动带图像显示的测试
roslaunch pantilt_camera_serial demo_test.launch use_image_view:=true
```

### 使用测试脚本

```bash
# 进入脚本目录
cd ~/catkin_ws/src/pantilt_camera_serial_driver/scripts

# 测试云台控制
./test_pantilt_control.sh pantilt_demo

# 测试 ROS Topic
./test_topics.sh cv_camera_demo pantilt_demo
```

---

## 常用命令参考

### 查看话题

```bash
# 列出所有话题
rostopic list

# 查看话题信息
rostopic info /cv_camera_demo/image_raw

# 查看发布频率
rostopic hz /cv_camera_demo/image_raw

# 打印消息内容
rostopic echo /pantilt_demo/pantilt_angle_info
```

### 服务调用

| 功能 | 命令 |
|------|------|
| 云台回中 | `rosservice call /pantilt_demo/send_command "command_name: 'BackToCenter' data: []"` |
| 锁定模式 | `rosservice call /pantilt_demo/send_command "command_name: 'SetLockMode' data: []"` |
| 航向跟随 | `rosservice call /pantilt_demo/send_command "command_name: 'SetHeadingFollow' data: []"` |
| 航向俯仰跟随 | `rosservice call /pantilt_demo/send_command "command_name: 'SetHeadingPitchFollow' data: []"` |
| 全跟随 | `rosservice call /pantilt_demo/send_command "command_name: 'SetFullFollowMode' data: []"` |
| 设置速度 | `rosservice call /pantilt_demo/send_command "command_name: 'SetPantiltSpeed' data: [50]"` |
| 设置角度 | `rosservice call /pantilt_demo/send_command "command_name: 'SetPantiltAngle' data: [0, 0, 0]"` |

### 参数说明

- **heading**: 航向角 [-160°, 160°]
- **roll**: 横滚角 [-40°, 40°]
- **pitch**: 俯仰角 [-90°, 90°]
- **速度**: [0, 100]

---

## 常见问题排查

### 问题1: 无法打开摄像头

**症状：**
```
[ERROR] Cannot open camera device
```

**解决方案：**
```bash
# 1. 检查设备是否存在
ls /dev/video*

# 2. 检查设备权限
ls -l /dev/video0

# 3. 运行权限设置脚本
sudo ./setup_device_permissions.sh

# 4. 检查设备路径是否正确
# 修改 launch 文件中的 device_path 参数
```

### 问题2: 无法连接云台串口

**症状：**
```
[ERROR] Failed to open serial port
```

**解决方案：**
```bash
# 1. 检查串口设备
ls /dev/ttyUSB*

# 2. 检查设备权限
ls -l /dev/ttyUSB0

# 3. 添加用户到 dialout 组
sudo usermod -a -G dialout $USER

# 4. 重新登录或重启系统

# 5. 检查串口是否被占用
lsof /dev/ttyUSB0
```

### 问题3: RViz 无法显示图像

**症状：**
- RViz 中图像显示为黑色或空白

**解决方案：**
```bash
# 1. 检查图像话题是否正确发布
rostopic list | grep image

# 2. 检查图像数据
rostopic hz /cv_camera_demo/image_raw

# 3. 使用 image_view 测试
rosrun image_view image_view image:=/cv_camera_demo/image_raw

# 4. 检查 RViz 中的话题配置
# 确保 Image 插件订阅的话题正确
```

### 问题4: 图像畸变矫正不生效

**症状：**
- 矫正后图像与原始图像相同

**解决方案：**
```bash
# 1. 检查标定文件是否存在
ls $(rospack find cv_camera)/calibrationdata/

# 2. 检查 camera_info 是否正确发布
rostopic echo /cv_camera_demo/camera_info

# 3. 检查 image_proc 是否运行
rosnode list | grep image_proc

# 4. 检查话题映射是否正确
rostopic info /cv_camera_demo/image_rect_color
```

---

## Launch 文件说明

| 文件 | 功能 |
|------|------|
| `pantiltcamera.launch` | 主启动文件，启动完整系统 |
| `test_camera_only.launch` | 仅测试相机 |
| `test_pantilt_only.launch` | 仅测试云台 |
| `demo_test.launch` | 综合测试，带调试输出 |

---

## 脚本说明

| 脚本 | 功能 |
|------|------|
| `setup_device_permissions.sh` | 设置设备权限 |
| `test_pantilt_control.sh` | 交互式云台控制测试 |
| `test_topics.sh` | ROS Topic 测试 |
