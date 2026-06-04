# 云台相机快速上手指南

> **目标**: 5 分钟内完成从安装到控制云台的完整流程。

---

## 1. 环境准备

### 系统要求
- Ubuntu 20.04 LTS
- ROS Noetic（已安装并配置）

### 安装依赖

```bash
sudo apt-get update
sudo apt-get install ros-noetic-image-proc ros-noetic-image-view
```

---

## 2. 获取代码并编译

假设你的工作空间为 `~/catkin_ws`。

```bash
cd ~/catkin_ws/src

# 将三个 ROS 包放入 src 目录：
#   cv_camera/
#   pantilt_camera_serial_driver/  (包名 pantilt_camera_serial)
#   rviz_pantilt_plugin/

cd ~/catkin_ws
catkin_make
source devel/setup.bash
```

> 提示：将 `source ~/catkin_ws/devel/setup.bash` 加入 `~/.bashrc`，可避免每次手动 source。

---

## 3. 设置设备权限

### 方式一：一键脚本（推荐）

```bash
sudo ./scripts/setup_device_permissions.sh
```

### 方式二：手动设置

```bash
# 添加用户到 dialout 组（串口权限）
sudo usermod -a -G dialout $USER

# 临时设置设备权限
sudo chmod a+rw /dev/ttyUSB0   # 云台串口
sudo chmod a+rw /dev/video0    # 摄像头
```

> **注意**：如果是第一次加入 `dialout` 组，需要**重新登录或重启系统**才能生效。

---

## 4. 确认设备连接

```bash
# 查看摄像头设备
ls /dev/v4l/by-path

# 查看云台串口
ls /dev/ttyUSB*
```

如果能看到设备路径，说明硬件连接正常。

---

## 5. 启动系统

### 推荐方式：全自动启动（自动检测设备）

```bash
roslaunch pantilt_camera_serial pantiltcamera_ready.launch
```

此命令会自动：
1. 扫描并识别摄像头 (`/dev/v4l/by-path`)
2. 探测并验证云台串口 (`/dev/ttyUSB*`、`/dev/ttyACM*`)
3. 启动相机 + 云台 + 图像畸变矫正
4. 自动发送「回中」命令并等待到位
5. 自检通过后会提示：
   ```
   ==================================================
   自检通过: 相机与云台均已就绪
   ==================================================
   ```

### 可选参数

```bash
# 指定摄像头路径
roslaunch pantilt_camera_serial pantiltcamera_ready.launch device_path:=/dev/video2

# 指定串口路径
roslaunch pantilt_camera_serial pantiltcamera_ready.launch port_name:=/dev/ttyUSB1

# 启动图像窗口 + RViz
roslaunch pantilt_camera_serial pantiltcamera_ready.launch use_image_view:=true use_rviz:=true
```

---

## 6. 控制云台

保持 launch 终端运行，另开一个终端执行：

```bash
source ~/catkin_ws/devel/setup.bash

# 查看当前角度
rosrun pantilt_camera_serial pantilt_tool.py info

# 云台回中
rosrun pantilt_camera_serial pantilt_tool.py center

# 设置目标角度（heading=30°, pitch=-10°）
rosrun pantilt_camera_serial pantilt_tool.py set --heading 30 --pitch -10

# 切换工作模式（锁定 / 航向跟随 / 全跟随）
rosrun pantilt_camera_serial pantilt_tool.py mode lock
rosrun pantilt_camera_serial pantilt_tool.py mode heading
rosrun pantilt_camera_serial pantilt_tool.py mode full_follow

# 保存相机图像
rosrun pantilt_camera_serial pantilt_tool.py capture -o ./pics
```

> `pantilt_tool.py` 默认与 `pantiltcamera_ready.launch` 的命名空间 `pantilt_demo` 匹配，无需额外指定。

---

## 7. 查看图像

### 方式一：命令行 image_view

```bash
# 原始图像
rosrun image_view image_view image:=/cv_camera_demo/image_raw

# 畸变矫正后图像
rosrun image_view image_view image:=/cv_camera_demo/image_rect_color
```

### 方式二：RViz

```bash
roslaunch pantilt_camera_serial pantiltcamera_ready.launch use_rviz:=true
```

在 RViz 中添加 **Image** 显示，订阅 `/cv_camera_demo/image_raw` 或 `/cv_camera_demo/image_rect_color`。

---

## 下一步

- 了解完整控制命令 → 查看 [`USER_MANUAL.md`](USER_MANUAL.md)
- 部署到新机器或排查故障 → 查看 [`DEPLOYMENT.md`](DEPLOYMENT.md)
