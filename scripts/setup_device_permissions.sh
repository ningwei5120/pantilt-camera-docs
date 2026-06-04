#!/bin/bash
# =============================================================================
# Autolabor C1 云台相机设备权限设置脚本
# =============================================================================
# 使用方法: 
#   chmod +x setup_device_permissions.sh
#   sudo ./setup_device_permissions.sh
# =============================================================================

set -e

echo "========================================"
echo "Autolabor C1 云台相机设备权限设置"
echo "========================================"
echo ""

# 检查是否以root运行
if [ "$EUID" -ne 0 ]; then 
    echo "错误: 请使用 sudo 运行此脚本"
    exit 1
fi

# 获取当前用户名
CURRENT_USER=${SUDO_USER:-$USER}
echo "当前用户: $CURRENT_USER"
echo ""

# 1. 添加用户到 dialout 组（串口访问权限）
echo "[1/4] 添加用户到 dialout 组..."
if id -nG "$CURRENT_USER" | grep -qw "dialout"; then
    echo "      用户已在 dialout 组中"
else
    usermod -a -G dialout "$CURRENT_USER"
    echo "      已添加用户到 dialout 组"
    echo "      注意: 需要重新登录或重启系统使权限生效"
fi
echo ""

# 2. 设置串口设备权限
echo "[2/4] 设置串口设备权限..."
if ls /dev/ttyUSB* 1>/dev/null 2>&1; then
    for dev in /dev/ttyUSB*; do
        chmod a+rw "$dev"
        echo "      已设置权限: $dev"
    done
else
    echo "      警告: 未找到 /dev/ttyUSB* 设备"
    echo "      请确保云台已连接并开启"
fi
echo ""

# 3. 设置摄像头设备权限
echo "[3/4] 设置摄像头设备权限..."
if ls /dev/video* 1>/dev/null 2>&1; then
    for dev in /dev/video*; do
        chmod a+rw "$dev"
        echo "      已设置权限: $dev"
    done
else
    echo "      警告: 未找到 /dev/video* 设备"
    echo "      请确保摄像头已连接"
fi
echo ""

# 4. 显示设备信息
echo "[4/4] 设备信息:"
echo ""
echo "    摄像头设备 (v4l by-path):"
if [ -d /dev/v4l/by-path ]; then
    for link in /dev/v4l/by-path/*; do
        if [ -L "$link" ]; then
            target=$(readlink -f "$link")
            echo "      $link -> $target"
        fi
    done
else
    echo "      未找到 /dev/v4l/by-path 目录"
fi
echo ""

echo "    串口设备:"
if ls /dev/ttyUSB* 1>/dev/null 2>&1; then
    for dev in /dev/ttyUSB*; do
        echo "      $dev"
        ls -l "$dev" | awk '{print "        权限: " $1 " 所有者: " $3 " 组: " $4}'
    done
else
    echo "      未找到串口设备"
fi
echo ""

echo "========================================"
echo "设置完成!"
echo "========================================"
echo ""
echo "提示:"
echo "  1. 如果这是第一次运行，请重新登录或重启系统"
echo "  2. 使用以下命令查看设备:"
echo "     ls /dev/v4l/by-path    # 查看摄像头"
echo "     ls /dev/ttyUSB*        # 查看串口"
echo ""
