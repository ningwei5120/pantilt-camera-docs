#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
云台相机初始化自检与回正脚本

功能:
  1. 等待云台控制服务上线
  2. 发送 BackToCenter 命令使云台回正
  3. 等待并检查相机图像流是否正常
  4. 等待并检查云台角度反馈是否正常
  5. 等待云台角度收敛到零位附近（可选）
  6. 打印自检报告后退出

使用示例:
  rosrun pantilt_camera_serial pantilt_init_check.py \
      --camera-topic /cv_camera_demo/image_raw \
      --angle-topic /pantilt_demo/pantilt_angle_info \
      --command-service /pantilt_demo/send_command
"""

import sys
import math
import argparse
import rospy
from sensor_msgs.msg import Image
from pantilt_camera_serial.srv import PantiltCommand, PantiltCommandRequest
from pantilt_camera_serial.msg import PantiltAngleInfo


def wait_for_service(service_name, timeout):
    rospy.loginfo("[自检] 等待服务 %s ...", service_name)
    try:
        rospy.wait_for_service(service_name, timeout=timeout)
        rospy.loginfo("[自检] 服务 %s 已上线", service_name)
        return True
    except rospy.exceptions.ROSException:
        rospy.logerr("[自检] 超时: 服务 %s 未在 %.0f 秒内上线", service_name, timeout)
        return False


def wait_for_message(topic_name, msg_type, timeout):
    rospy.loginfo("[自检] 等待话题 %s 的数据 ...", topic_name)
    try:
        msg = rospy.wait_for_message(topic_name, msg_type, timeout=timeout)
        rospy.loginfo("[自检] 话题 %s 数据正常", topic_name)
        return msg
    except rospy.exceptions.ROSException:
        rospy.logerr("[自检] 超时: 话题 %s 未在 %.0f 秒内收到数据", topic_name, timeout)
        return None


def call_center(service_name):
    rospy.loginfo("[自检] 发送回正命令 ...")
    try:
        proxy = rospy.ServiceProxy(service_name, PantiltCommand)
        req = PantiltCommandRequest()
        req.command_name = "BackToCenter"
        req.data = []
        resp = proxy(req)
        if resp.success:
            rospy.loginfo("[自检] 回正命令已发送")
            return True
        else:
            rospy.logerr("[自检] 回正命令失败: %s", resp.message)
            return False
    except rospy.ServiceException as e:
        rospy.logerr("[自检] 调用服务失败: %s", e)
        return False


def wait_for_zero(angle_topic, timeout, tolerance):
    rospy.loginfo("[自检] 等待云台回正到位 (容差 %.1f°，超时 %.0f 秒) ...", tolerance, timeout)
    start = rospy.Time.now()
    rate = rospy.Rate(10)
    while not rospy.is_shutdown():
        elapsed = (rospy.Time.now() - start).to_sec()
        if elapsed > timeout:
            rospy.logwarn("[自检] 等待回正超时，当前角度可能未完全到位")
            return False
        try:
            msg = rospy.wait_for_message(angle_topic, PantiltAngleInfo, timeout=0.2)
            if (abs(msg.heading) <= tolerance and
                abs(msg.roll) <= tolerance and
                abs(msg.pitch) <= tolerance):
                rospy.loginfo("[自检] 云台已回正: H=%.2f, R=%.2f, P=%.2f",
                              msg.heading, msg.roll, msg.pitch)
                return True
        except rospy.exceptions.ROSException:
            pass
        rate.sleep()
    return False


def main():
    parser = argparse.ArgumentParser(description="云台相机初始化自检与回正")
    parser.add_argument("--camera-topic", default="/cv_camera_demo/image_raw",
                        help="相机图像话题 (默认: /cv_camera_demo/image_raw)")
    parser.add_argument("--angle-topic", default="/pantilt_demo/pantilt_angle_info",
                        help="云台角度话题 (默认: /pantilt_demo/pantilt_angle_info)")
    parser.add_argument("--command-service", default="/pantilt_demo/send_command",
                        help="云台控制服务 (默认: /pantilt_demo/send_command)")
    parser.add_argument("--timeout", type=float, default=10.0,
                        help="服务/话题等待超时 (秒，默认: 10)")
    parser.add_argument("--center-timeout", type=float, default=15.0,
                        help="回正等待超时 (秒，默认: 15)")
    parser.add_argument("--tolerance", type=float, default=2.0,
                        help="回正角度容差 (°，默认: 2)")
    parser.add_argument("--skip-center-wait", action="store_true",
                        help="跳过等待回正到位，仅发送命令")
    # 过滤 ROS 自动注入的 __name / __log 等内部参数
    args = parser.parse_args([a for a in sys.argv[1:] if not a.startswith('__')])

    rospy.init_node("pantilt_init_check", anonymous=True)

    rospy.loginfo("=" * 50)
    rospy.loginfo("云台相机初始化自检开始")
    rospy.loginfo("=" * 50)

    ok = True

    # 1. 等待云台服务
    if not wait_for_service(args.command_service, args.timeout):
        ok = False

    # 2. 发送回正命令
    if ok and not call_center(args.command_service):
        ok = False

    # 3. 等待相机图像
    if wait_for_message(args.camera_topic, Image, args.timeout) is None:
        ok = False

    # 4. 等待云台角度
    angle_msg = wait_for_message(args.angle_topic, PantiltAngleInfo, args.timeout)
    if angle_msg is None:
        ok = False

    # 5. 等待回正到位
    if ok and not args.skip_center_wait:
        wait_for_zero(args.angle_topic, args.center_timeout, args.tolerance)

    # 6. 自检报告
    rospy.loginfo("=" * 50)
    if ok:
        rospy.loginfo("自检通过: 相机与云台均已就绪")
    else:
        rospy.logerr("自检未完全通过，请检查硬件连接")
    rospy.loginfo("=" * 50)

    # 节点完成使命，退出
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
