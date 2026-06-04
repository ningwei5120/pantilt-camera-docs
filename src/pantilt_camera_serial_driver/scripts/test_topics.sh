#!/bin/bash
# =============================================================================
# Autolabor C1 云台相机 ROS Topic 测试脚本
# =============================================================================
# 使用方法: 
#   chmod +x test_topics.sh
#   ./test_topics.sh [相机命名空间] [云台命名空间]
# 示例:
#   ./test_topics.sh                          # 使用默认命名空间
#   ./test_topics.sh cv_camera_demo pantilt_demo  # 使用自定义命名空间
# =============================================================================

# 设置命名空间
CAM_NS="${1:-cv_camera_demo}"
PAN_NS="${2:-pantilt_demo}"

echo "========================================"
echo "Autolabor C1 ROS Topic 测试"
echo "========================================"
echo "相机命名空间: /$CAM_NS"
echo "云台命名空间: /$PAN_NS"
echo ""

# 检查ROS环境
if ! command -v rostopic &> /dev/null; then
    echo "错误: 找不到 rostopic 命令"
    echo "请确保已 source ROS 环境: source /opt/ros/noetic/setup.bash"
    exit 1
fi

# 等待ROS master
wait_for_master() {
    echo "等待 ROS Master..."
    for i in {1..10}; do
        if rostopic list &>/dev/null; then
            echo "ROS Master 已连接"
            return 0
        fi
        sleep 1
    done
    echo "错误: 无法连接到 ROS Master"
    echo "请确保 roscore 正在运行"
    return 1
}

wait_for_master || exit 1
echo ""

# 测试相机话题
test_camera_topics() {
    echo "========================================"
    echo "测试相机相关 Topic"
    echo "========================================"
    
    local topics=(
        "/$CAM_NS/image_raw"
        "/$CAM_NS/image_raw/compressed"
        "/$CAM_NS/camera_info"
        "/$CAM_NS/image_rect_color"
    )
    
    for topic in "${topics[@]}"; do
        if rostopic list | grep -q "^$topic$"; then
            echo "[✓] $topic"
            # 获取发布频率
            hz_output=$(timeout 2 rostopic hz "$topic" 2>&1 | tail -1)
            if [[ $hz_output == *"average rate:"* ]]; then
                echo "    频率: $(echo $hz_output | grep -oP 'average rate: \K[0-9.]+') Hz"
            fi
        else
            echo "[✗] $topic (未找到)"
        fi
    done
    echo ""
}

# 测试云台话题
test_pantilt_topics() {
    echo "========================================"
    echo "测试云台相关 Topic"
    echo "========================================"
    
    local topics=(
        "/$PAN_NS/pantilt_angle_info"
        "/$PAN_NS/pantilt_vel"
    )
    
    for topic in "${topics[@]}"; do
        if rostopic list | grep -q "^$topic$"; then
            echo "[✓] $topic"
        else
            echo "[✗] $topic (未找到)"
        fi
    done
    echo ""
}

# 测试服务
test_services() {
    echo "========================================"
    echo "测试云台相关 Service"
    echo "========================================"
    
    local services=(
        "/$PAN_NS/send_command"
    )
    
    for service in "${services[@]}"; do
        if rosservice list | grep -q "^$service$"; then
            echo "[✓] $service"
            # 显示服务类型
            srv_type=$(rosservice type "$service" 2>/dev/null)
            echo "    类型: $srv_type"
        else
            echo "[✗] $service (未找到)"
        fi
    done
    echo ""
}

# 显示节点信息
show_nodes() {
    echo "========================================"
    echo "当前运行的节点"
    echo "========================================"
    rosnode list | grep -E "($CAM_NS|$PAN_NS)" || echo "未找到相机或云台节点"
    echo ""
}

# 显示话题详情
show_topic_details() {
    echo "========================================"
    echo "Topic 详细信息"
    echo "========================================"
    echo ""
    
    echo "相机原始图像:"
    rostopic info "/$CAM_NS/image_raw" 2>/dev/null || echo "  未找到"
    echo ""
    
    echo "相机标定信息:"
    rostopic info "/$CAM_NS/camera_info" 2>/dev/null || echo "  未找到"
    echo ""
    
    echo "云台角度信息:"
    rostopic info "/$PAN_NS/pantilt_angle_info" 2>/dev/null || echo "  未找到"
    echo ""
}

# 主函数
main() {
    show_nodes
    test_camera_topics
    test_pantilt_topics
    test_services
    
    read -p "是否显示详细信息? (y/n): " show_detail
    if [[ $show_detail == "y" || $show_detail == "Y" ]]; then
        show_topic_details
    fi
    
    echo "========================================"
    echo "测试完成"
    echo "========================================"
}

main
