# Autolabor 云台相机图像驱动程序

## 简介
cv_camera节点采用OpenCV捕获视频设备，发布图像和相机信息话题。
驱动包中提供了Autolabor云台相机的标定参数和原始图像。
用户可直接启动launch查看矫正前后的数据。

如果你在寻找 ROS2 驱动程序，请查看 [这里](https://github.com/Kapernikov/cv_camera)。

## 使用方法

### 1、依赖项

安装image_proc包，用于矫正图像。

```bash
    sudo apt-get install ros-noetic-image-proc
```

### 2、编译和安装

首先，将这个包克隆到你的 ROS 工作空间的 `src` 目录下，然后使用以下命令编译：

```bash
cd ~/catkin_ws  # 或你的工作空间目录
catkin_make     # 或使用 catkin build，如果你使用 catkin_tools
```

### 3、修改launch文件

在 `cv_camera.launch` 文件中，修改 `device_path` 参数。
可以通过以下命令查看设备路径：
```bash
ls /dev/v4l/by-path
```

### 4、启动launch文件

使用以下命令启动系统：

```bash
roslaunch cv_camera camera1920.launch
```

该launch文件会启动一个节点，发布分辨率为1920x1080的图像以及矫正后的图像。

## 文件说明

- launch文件夹：
  - camera1280.launch：发布分辨率为1280x720的图像以及矫正后的图像。
  - camera1920.launch：发布分辨率为192x1080的图像以及矫正后的图像。
  - camera3840.launch：发布分辨率为3840x2160的图像以及矫正后的图像。
  
- calibrationdata文件夹：
  - 1280x720.yaml ：分辨率为1280x720的相机标定参数。
  - 1920x1080.yaml：分辨率为1920x1080的相机标定参数。
  - 3840x2160.yaml：分辨率为3840x2160的相机标定参数。
  - 12880x720.tar.gz：分辨率为1280x720的标定图像。
  - 1920x1080.gz：分辨率为1920x1080的标定图像。
  - 3840x2160.gz：分辨率为3840x2160的标定图像。

## 发布的话题

* `~image_raw` (*sensor_msgs/Image*)
* `~camera_info` (*sensor_msgs/CameraInfo*)

## 服务

* `~set_camera_info` (*sensor_msgs/SetCameraInfo*)

## 参数

* `~rate` (*double*, 默认值: 30.0) – 发布频率 [Hz]。
* `~device_id` (*int*, 默认值: 0) – 捕获设备 ID。
* `~device_path` (*string*, 默认值: "") – 相机设备文件路径，例如 `/dev/video0`。
* `~frame_id` (*string*, 默认值: "camera") – 消息头的 `frame_id`。
* `~image_width` (*int*) – 尝试设置捕获图像的宽度。
* `~image_height` (*int*) – 尝试设置捕获图像的高度。
* `~camera_info_url` (*string*) – 相机信息 yaml 文件的 URL。
* `~file` (*string*, 默认值: "") – 如果不为空，则使用电影文件而不是设备。
* `~capture_delay` (*double*, 默认值: 0) – 捕获和接收图像的估计持续时间。
* `~rescale_camera_info` (*bool*, 默认值: false) – 自动缩放相机校准信息。
* `~camera_name` (*bool*, 默认值: 与 `frame_id` 相同) – `camera_info_manager` 的相机名称。

支持 CV_CAP_PROP_*，通过以下参数。

* `~cv_cap_prop_pos_msec` (*double*)
* `~cv_cap_prop_pos_avi_ratio` (*double*)
* `~cv_cap_prop_frame_width` (*double*)
* `~cv_cap_prop_frame_height` (*double*)
* `~cv_cap_prop_fps` (*double*)
* `~cv_cap_prop_fourcc` (*double*)
* `~cv_cap_prop_frame_count` (*double*)
* `~cv_cap_prop_format` (*double*)
* `~cv_cap_prop_mode` (*double*)
* `~cv_cap_prop_brightness` (*double*)
* `~cv_cap_prop_contrast` (*double*)
* `~cv_cap_prop_saturation` (*double*)
* `~cv_cap_prop_hue` (*double*)
* `~cv_cap_prop_gain` (*double*)
* `~cv_cap_prop_exposure` (*double*)
* `~cv_cap_prop_convert_rgb` (*double*)
* `~cv_cap_prop_rectification` (*double*)
* `~cv_cap_prop_iso_speed` (*double*)

