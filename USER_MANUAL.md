# 云台相机使用说明书

> 本文档详细介绍所有控制云台相机的方式，适合日常操作者和需要二次开发的工程师。

---

## 目录

1. [控制方式概览](#控制方式概览)
2. [命令行工具 pantilt_tool.py](#命令行工具-pantilt_toolpy)
3. [RViz 可视化插件](#rviz-可视化插件)
4. [ROS 原生接口（Topic / Service）](#ros-原生接口topic--service)
5. [仅启动相机（不依赖云台）](#仅启动相机不依赖云台)
6. [常用命令速查表](#常用命令速查表)

---

## 控制方式概览

| 方式 | 适用场景 | 优点 |
|------|----------|------|
| `pantilt_tool.py` | 日常调试、脚本自动化 | 命令行一键操作，无需写代码 |
| RViz 插件 | 可视化调试、演示 | 图形界面，实时反馈，键盘控制 |
| `rostopic` / `rosservice` | 深度集成、自定义节点 | ROS 标准接口，灵活性最高 |

> **命名空间对照表**：不同 launch 文件启动后，节点命名空间不同，控制时需注意。

| 启动文件 | 相机节点 | 云台节点 | 图像话题 | 角度话题 | 控制服务 |
|----------|----------|----------|----------|----------|----------|
| `pantiltcamera_ready.launch` | `cv_camera_demo` | `pantilt_demo` | `/cv_camera_demo/image_raw` | `/pantilt_demo/pantilt_angle_info` | `/pantilt_demo/send_command` |
| `demo_test.launch` | `cv_camera_demo` | `pantilt_demo` | 同上 | 同上 | 同上 |
| `pantiltcamera.launch` | `cv_camera0` | `pantilt_camera_serial0` | `/cv_camera0/image_raw` | `/pantilt_camera_serial0/pantilt_angle_info` | `/pantilt_camera_serial0/send_command` |
| `test_pantilt_only.launch` | — | `pantilt_test` | — | `/pantilt_test/pantilt_angle_info` | `/pantilt_test/send_command` |

下文默认以 `pantiltcamera_ready.launch` 的命名空间 `pantilt_demo` 为例。

---

## 命令行工具 pantilt_tool.py

### 基本用法

```bash
rosrun pantilt_camera_serial pantilt_tool.py <子命令> [选项]
```

### 全局选项

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `-n, --ns` | `pantilt_demo` | 云台节点命名空间 |

---

### 1. info — 查询云台角度

```bash
# 查询一次
rosrun pantilt_camera_serial pantilt_tool.py info

# 持续实时监听（按 Ctrl+C 停止）
rosrun pantilt_camera_serial pantilt_tool.py info --watch
```

输出示例：
```
当前云台角度:
  Heading:  0.12°
  Roll:    -0.05°
  Pitch:    0.08°
```

---

### 2. set — 设置目标角度

角度范围：
- heading（航向）: [-160°, 160°]
- roll（横滚）: [-40°, 40°]
- pitch（俯仰）: [-90°, 90°]

```bash
# 设置航向 30°，俯仰 -10°
rosrun pantilt_camera_serial pantilt_tool.py set --heading 30 --pitch -10

# 全部归零
rosrun pantilt_camera_serial pantilt_tool.py set --heading 0 --roll 0 --pitch 0
```

---

### 3. center — 云台回中

```bash
rosrun pantilt_camera_serial pantilt_tool.py center
```

---

### 4. mode — 切换工作模式

| 模式 | 说明 |
|------|------|
| `lock` | 完全锁定，保持当前方向不动 |
| `heading` | 航向跟随，俯仰锁定 |
| `heading_pitch` | 航向和俯仰都跟随 |
| `full_follow` | 三轴全部跟随 |

```bash
rosrun pantilt_camera_serial pantilt_tool.py mode lock
rosrun pantilt_camera_serial pantilt_tool.py mode heading
rosrun pantilt_camera_serial pantilt_tool.py mode heading_pitch
rosrun pantilt_camera_serial pantilt_tool.py mode full_follow
```

> 模式之间直接切换即可，无需"退出"当前模式。

---

### 5. capture — 保存相机图像

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `-t, --topic` | `/image_color` | 图像话题 |
| `-o, --output` | `./captures` | 输出文件夹 |
| `-c, --count` | `1` | 保存张数 |
| `--interval` | `0.0` | 连续保存间隔（秒） |
| `-p, --prefix` | `capture` | 文件名前缀 |

```bash
# 保存单张到默认目录
rosrun pantilt_camera_serial pantilt_tool.py capture

# 指定话题和输出目录
rosrun pantilt_camera_serial pantilt_tool.py capture -t /cv_camera_demo/image_rect_color -o ~/Desktop/pics

# 连续保存 5 张，间隔 1 秒
rosrun pantilt_camera_serial pantilt_tool.py capture -c 5 --interval 1 -o ./images
```

文件名格式：`capture_YYYYMMDD_HHMMSS_mmm.jpg`

---

## RViz 可视化插件

### 启动方式

```bash
roslaunch pantilt_camera_serial pantiltcamera_ready.launch use_rviz:=true
```

或在已有 launch 运行时单独启动 RViz：

```bash
rviz -d $(rospack find pantilt_camera_serial)/rviz/singleCamera.rviz
```

### 界面功能

#### 1. 节点选择
在下拉框中选择当前活动的云台节点（如 `pantilt_demo`）。

#### 2. 模式设置
点击按钮直接切换工作模式：
- **SetLockMode** — 锁定当前方向
- **SetHeadingFollow** — 航向跟随
- **SetHeadingPitchFollow** — 航向+俯仰跟随
- **SetFullFollowMode** — 全跟随

#### 3. 键盘控制
先用鼠标单击 PantiltRvizPanel 窗口使其获得焦点，然后：

| 按键 | 动作 |
|------|------|
| `W` 或 `↑` | 向上（Pitch 增大） |
| `S` 或 `↓` | 向下（Pitch 减小） |
| `A` 或 `←` | 向左（Heading 减小） |
| `D` 或 `→` | 向右（Heading 增大） |

#### 4. 角度设置
在文本框中输入目标角度（单位：度），点击发送：
- Heading: [-160, 160]
- Roll: [-40, 40]
- Pitch: [-90, 90]

#### 5. 角度反馈
界面实时显示当前云台的角度数值。

---

## ROS 原生接口（Topic / Service）

### 发布的话题

#### `/pantilt_demo/pantilt_angle_info`
- **类型**: `pantilt_camera_serial/PantiltAngleInfo`
- **说明**: 当前云台角度信息
- **字段**:
  - `heading` — 对地航向角（°）
  - `roll` — 对地横滚角（°）
  - `pitch` — 对地俯仰角（°）
  - `encoder_heading` — 编码器航向读数
  - `encoder_roll` — 编码器横滚读数
  - `encoder_pitch` — 编码器俯仰读数

#### `/cv_camera_demo/image_raw`
- **类型**: `sensor_msgs/Image`
- **说明**: 原始图像

#### `/cv_camera_demo/image_rect_color`
- **类型**: `sensor_msgs/Image`
- **说明**: 畸变矫正后的图像（由 `image_proc` 发布）

#### `/cv_camera_demo/camera_info`
- **类型**: `sensor_msgs/CameraInfo`
- **说明**: 相机标定参数

---

### 订阅的话题

#### `/pantilt_demo/pantilt_vel`
- **类型**: `geometry_msgs/Twist`
- **说明**: 云台速度控制指令
- **用法**:
  - `linear.x` — 俯仰轴速度 [0, 2]
  - `angular.z` — 航向轴速度 [0, 2]

示例：
```bash
rostopic pub /pantilt_demo/pantilt_vel geometry_msgs/Twist "linear:
  x: 1.0
  y: 0.0
  z: 0.0
angular:
  x: 0.0
  y: 0.0
  z: 1.0"
```

---

### 提供的服务

#### `/pantilt_demo/send_command`
- **类型**: `pantilt_camera_serial/PantiltCommand`
- **说明**: 发送控制命令到云台

| 命令名 | 参数 | 说明 |
|--------|------|------|
| `SetLockMode` | `[]` | 锁定模式 |
| `SetHeadingFollow` | `[]` | 航向跟随，俯仰锁定 |
| `SetHeadingPitchFollow` | `[]` | 航向俯仰跟随 |
| `SetFullFollowMode` | `[]` | 全跟随模式 |
| `BackToCenter` | `[]` | 回中 |
| `SetPantiltSpeed` | `[speed]` | 设置速度，speed ∈ [0, 100] |
| `SetPantiltAngle` | `[h, r, p]` | 设置目标角度，h∈[-160,160], r∈[-40,40], p∈[-90,90] |

示例：
```bash
# 回中
rosservice call /pantilt_demo/send_command "command_name: 'BackToCenter' data: []"

# 设置角度
rosservice call /pantilt_demo/send_command "command_name: 'SetPantiltAngle' data: [10.0, 0.0, -5.0]"

# 设置速度
rosservice call /pantilt_demo/send_command "command_name: 'SetPantiltSpeed' data: [50]"
```

---

## 仅启动相机（不依赖云台）

如果只需要图像采集和畸变矫正，无需云台硬件：

```bash
# 1920×1080（含畸变矫正）
roslaunch cv_camera camera1920.launch

# 1280×720
roslaunch cv_camera camera1280.launch

# 3840×2160
roslaunch cv_camera camera3840.launch
```

相机参数可调：
- `device_path` — 指定摄像头路径
- `camera_info_url` — 指定标定文件

---

## 常用命令速查表

```bash
# ========== 启动 ==========
roslaunch pantilt_camera_serial pantiltcamera_ready.launch
roslaunch pantilt_camera_serial pantiltcamera_ready.launch use_rviz:=true use_image_view:=true

# ========== 查看状态 ==========
rosrun pantilt_camera_serial pantilt_tool.py info
rosrun pantilt_camera_serial pantilt_tool.py info --watch
rostopic echo /pantilt_demo/pantilt_angle_info

# ========== 控制 ==========
rosrun pantilt_camera_serial pantilt_tool.py center
rosrun pantilt_camera_serial pantilt_tool.py set --heading 0 --roll 0 --pitch 0
rosrun pantilt_camera_serial pantilt_tool.py mode full_follow

# ========== 图像 ==========
rosrun image_view image_view image:=/cv_camera_demo/image_raw
rosrun image_view image_view image:=/cv_camera_demo/image_rect_color
rosrun pantilt_camera_serial pantilt_tool.py capture -o ./pics

# ========== 速度控制 ==========
rostopic pub /pantilt_demo/pantilt_vel geometry_msgs/Twist "linear:
  x: 1.0
  y: 0.0
  z: 0.0
angular:
  x: 0.0
  y: 0.0
  z: 1.0"

# ========== 服务调用 ==========
rosservice call /pantilt_demo/send_command "command_name: 'BackToCenter' data: []"
rosservice call /pantilt_demo/send_command "command_name: 'SetPantiltAngle' data: [0, 0, 0]"
```
