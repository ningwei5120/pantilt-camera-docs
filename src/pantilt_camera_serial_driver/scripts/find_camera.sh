#!/bin/bash
# 自动查找 Wasintek 云台相机设备路径

echo "查找 Wasintek 相机设备..."
echo ""

# 方法1: 通过by-path查找
for link in /dev/v4l/by-path/*; do
    if [ -L "$link" ]; then
        target=$(readlink -f "$link")
        device=$(basename "$target")
        name=$(cat /sys/class/video4linux/$device/name 2>/dev/null)
        if [[ "$name" == *"Wasintek"* ]]; then
            echo "找到 Wasintek 相机:"
            echo "  路径: $link"
            echo "  设备: $target"
            echo "  名称: $name"
            echo ""
        fi
    fi
done

# 方法2: 通过USB ID查找
echo "USB设备信息:"
lsusb | grep -i "wasintek\|2aad" || echo "未找到 Wasintek USB设备"
echo ""

# 方法3: 显示所有视频设备
echo "所有视频设备:"
for i in /sys/class/video4linux/video*; do
    if [ -d "$i" ]; then
        name=$(basename "$i")
        devname=$(cat "$i/name" 2>/dev/null || echo "Unknown")
        echo "  /dev/$name: $devname"
    fi
done
