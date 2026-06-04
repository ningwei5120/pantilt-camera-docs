#!/bin/bash
# =============================================================================
# Autolabor C1 云台控制测试脚本
# =============================================================================
# 使用方法: 
#   chmod +x test_pantilt_control.sh
#   ./test_pantilt_control.sh [节点命名空间]
# 示例:
#   ./test_pantilt_control.sh                    # 使用默认命名空间 /pantilt_demo
#   ./test_pantilt_control.sh pantilt_camera_serial0  # 使用自定义命名空间
# =============================================================================

# 设置节点命名空间
NAMESPACE="${1:-pantilt_demo}"

echo "========================================"
echo "Autolabor C1 云台控制测试"
echo "========================================"
echo "使用命名空间: /$NAMESPACE"
echo ""

# 检查ROS环境
if ! command -v rostopic &> /dev/null; then
    echo "错误: 找不到 rostopic 命令"
    echo "请确保已 source ROS 环境: source /opt/ros/noetic/setup.bash"
    exit 1
fi

# 检查节点是否运行
if ! rostopic list | grep -q "$NAMESPACE"; then
    echo "错误: 未找到节点 /$NAMESPACE"
    echo "请先启动云台节点:"
    echo "  roslaunch pantilt_camera_serial test_pantilt_only.launch"
    echo "  或"
    echo "  roslaunch pantilt_camera_serial demo_test.launch"
    exit 1
fi

echo "检测到节点正在运行"
echo ""

# 显示菜单
show_menu() {
    echo "========================================"
    echo "请选择测试功能:"
    echo "========================================"
    echo "  1. 查看当前云台角度"
    echo "  2. 云台回中 (BackToCenter)"
    echo "  3. 设置锁定模式 (SetLockMode)"
    echo "  4. 设置航向跟随模式 (SetHeadingFollow)"
    echo "  5. 设置航向俯仰跟随模式 (SetHeadingPitchFollow)"
    echo "  6. 设置全跟随模式 (SetFullFollowMode)"
    echo "  7. 设置云台速度"
    echo "  8. 设置云台角度 (heading, roll, pitch)"
    echo "  9. 测试速度控制 (发布 Twist 消息)"
    echo "  0. 退出"
    echo "========================================"
}

# 主循环
while true; do
    show_menu
    read -p "请输入选项 [0-9]: " choice
    echo ""
    
    case $choice in
        1)
            echo "正在获取云台角度信息..."
            echo "按 Ctrl+C 停止"
            rostopic echo "/${NAMESPACE}/pantilt_angle_info"
            ;;
        2)
            echo "发送回中命令..."
            rosservice call "/${NAMESPACE}/send_command" "command_name: 'BackToCenter'
data: []"
            echo ""
            ;;
        3)
            echo "设置锁定模式..."
            rosservice call "/${NAMESPACE}/send_command" "command_name: 'SetLockMode'
data: []"
            echo ""
            ;;
        4)
            echo "设置航向跟随模式..."
            rosservice call "/${NAMESPACE}/send_command" "command_name: 'SetHeadingFollow'
data: []"
            echo ""
            ;;
        5)
            echo "设置航向俯仰跟随模式..."
            rosservice call "/${NAMESPACE}/send_command" "command_name: 'SetHeadingPitchFollow'
data: []"
            echo ""
            ;;
        6)
            echo "设置全跟随模式..."
            rosservice call "/${NAMESPACE}/send_command" "command_name: 'SetFullFollowMode'
data: []"
            echo ""
            ;;
        7)
            read -p "请输入速度值 [0-100]: " speed
            echo "设置云台速度为 $speed..."
            rosservice call "/${NAMESPACE}/send_command" "command_name: 'SetPantiltSpeed'
data: [$speed]"
            echo ""
            ;;
        8)
            read -p "请输入航向角 heading [-160, 160]: " heading
            read -p "请输入横滚角 roll [-40, 40]: " roll
            read -p "请输入俯仰角 pitch [-90, 90]: " pitch
            echo "设置云台角度 (heading=$heading, roll=$roll, pitch=$pitch)..."
            rosservice call "/${NAMESPACE}/send_command" "command_name: 'SetPantiltAngle'
data: [$heading, $roll, $pitch]"
            echo ""
            ;;
        9)
            echo "测试速度控制..."
            echo "发布 Twist 消息: linear.x=1.0 (俯仰), angular.z=1.0 (航向)"
            echo "按 Ctrl+C 停止，10秒后自动停止"
            timeout 10 rostopic pub "/${NAMESPACE}/pantilt_vel" geometry_msgs/Twist "linear:
  x: 1.0
  y: 0.0
  z: 0.0
angular:
  x: 0.0
  y: 0.0
  z: 1.0" --rate 10
            echo ""
            ;;
        0)
            echo "退出测试"
            exit 0
            ;;
        *)
            echo "无效选项，请重新输入"
            echo ""
            ;;
    esac
done
