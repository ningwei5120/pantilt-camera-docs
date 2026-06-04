# Autolabor相机云台串口控制节点

## 简介
Pantilt_camera_serial节点是一个为 ROS 开发的包，用于控制和管理与相机云台相关的硬件接口。该节点允许用户通过 ROS 控制云台的方向和角度，同时通过摄像头捕获视频数据。

![ControlPanel](./picture/ControlPanel.png)

## 功能
- 控制云台的方向和角度。
- 从摄像头捕获视频。
- 提供视频流和云台控制的 ROS 话题和服务。

## 使用方法

### 1. 编译和安装
确保已安装 `pantilt_camera_serial`, `cv_camera`, 和 `rviz_pantilt_plugin` 包。首先，将这些包克隆到你的 ROS 工作空间的 `src` 目录下，然后使用以下命令编译：

```bash
cd ~/catkin_ws  # 或你的工作空间目录
catkin_make     # 或使用 catkin build，如果你使用 catkin_tools
```

### 2. 修改 launch 文件
在 `pantiltcamera.launch` 文件中，修改 `device_path` 和 `port_name` 参数：
- `device_path`：视频设备的路径，如 `/dev/v4l/by-path/pci-...`
- `port_name`：云台控制的串口路径，如 `/dev/ttyUSB0`

### 3. 添加设备操作权限
为了让 ROS 节点能够访问硬件设备，你可能需要为当前用户添加设备操作权限：

```bash
sudo usermod -a -G dialout $USER
sudo chmod a+rw /dev/ttyUSB0  # 云台控制的串口
sudo chmod a+rw /dev/video0   # 摄像头设备
```

### 4. 启动 launch 文件
使用以下命令启动系统：

```bash
roslaunch pantilt_camera_serial pantiltcamera.launch
```

### 5. 订阅和发布话题

此节点与其他 ROS 节点交互主要通过以下话题：

#### 发布话题

- **/node_name/pantilt_angle_info**
    - **类型**: `pantilt_camera_serial/PantiltAngleInfo`
    - **描述**: 发布云台当前的角度信息，包括对地航向角、横滚角和俯仰角，以及对应的相对基座读数。
    - **单位**：角度单位均为度（degrees）。
    - **消息结构**:
        - `heading`: 对地航向角（degrees）
        - `roll`: 对地横滚角（degrees）
        - `pitch`: 对地俯仰角（degrees）
        - `encoder_heading`: 相对基座航向角读数
        - `encoder_roll`: 相对基座横滚角读数
        - `encoder_pitch`: 相对基座俯仰角读数

#### 订阅话题

- **/node_name/pantilt_vel**
    - **类型**: `geometry_msgs/Twist`
    - **描述**: 接收控制云台的速度和方向的指令。`linear.x` 和 `angular.z` 分别用于控制航向（heading）和俯仰（pitch）轴的速度。
    - **消息用法**:
        - `linear.x`: 控制航向轴的速度,取值范围 [0,2]
        - `angular.z`: 控制俯仰轴的速度，取值范围 [0,2]

### 示例

发送云台控制命令示例：

```bash
rostopic pub /pantilt_camera_serial/pantilt_vel geometry_msgs/Twist '{linear: {x: 0.5}, angular: {z: 0.3}}'
```

### 6. 服务节点
提供服务 `/node_name/send_command`，允许用户发送具体的控制命令到云台。

- **SetLockMode**
    - 描述：锁定当前云台的方向，使其不随输入改变。
- **SetHeadingFollow**
    - 描述：设置云台仅跟随航向（水平方向），俯仰方向（垂直方向）保持锁定。
- **SetHeadingPitchFollow**
    - 描述：设置云台跟随航向和俯仰方向，即云台将根据输入完全动态调整。
- **SetFullFollowMode**
    - 描述：云台将完全跟随输入的方向移动，包括水平和垂直方向的全方位跟随。
- **BackToCenter**
    - 描述：命令云台回到中心位置，即所有方向重置为初始状态。
- **SetPantiltSpeed**
    - 描述：设置云台的运动速度，适用于需要控制运动快慢的场景。
- **SetPantiltAngle**
    - 描述：直接设置云台的目标角度，参数为三个角度值：Head角度、Roll角度和Pitch角度（如果有）。
    - 需要添加参数，data[0]: Head角度，data[1]: Roll角度，data[2]: Pitch角度。
    - 取值范围分别为：Head角度[-160,160]，Roll角度[-40,40]，Pitch角度[-90,90]。
- **使用示例**

```bash
rosservice call /pantilt_camera_serial/send_command "{command_name: 'SetLockMode', data: []}"
rosservice call /pantilt_camera_serial/send_command "{command_name: 'SetPantiltAngle', data: [10.0,0.0,0.0]}"
```

### 7. 使用 rviz_pantilt_plugins
在 RViz 中加载 `rviz_pantilt_plugin` 来实时查看和控制云台及摄像头。确保在 RViz 配置中加载了正确的 `.rviz` 配置文件，以查看相应的视图和控制界面。

> 注：详细说明参考(http://git.autolabor.com.cn/Ros/rviz_pantilt_plugin/src/branch/master/README.md)

## 注意
在使用之前，请确保所有硬件设备都已正确连接，并且系统上已正确安装了所有依赖的 ROS 包和库。
```
