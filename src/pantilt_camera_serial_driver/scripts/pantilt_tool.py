#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
云台控制工具脚本

功能:
  info    - 查询当前云台角度状态
  set     - 设置云台目标角度 (heading, roll, pitch)
  center  - 云台回中
  mode    - 设置云台工作模式
  capture - 保存相机图像到指定文件夹

使用示例:
  # 查询云台角度
  python3 pantilt_tool.py info -n pantilt_demo
  python3 pantilt_tool.py info -n pantilt_demo --watch

  # 设置目标角度
  python3 pantilt_tool.py set -n pantilt_demo --heading 0 --roll 0 --pitch 0

  # 云台回中
  python3 pantilt_tool.py center -n pantilt_test

  # 切换工作模式
  python3 pantilt_tool.py mode -n pantilt_demo lock
  python3 pantilt_tool.py mode -n pantilt_demo full_follow

  # 保存图像 (默认话题: /image_color)
  python3 pantilt_tool.py capture -o ./captures
  python3 pantilt_tool.py capture -t /image_color -o ./pics -c 5 --interval 1

说明:
  - 不同 launch 文件的 namespace 不同：
      demo_test.launch  -> pantilt_demo
      test_pantilt_only.launch -> pantilt_test
      pantiltcamera.launch -> pantilt_camera_serial0
  - capture 默认订阅 /image_color，可通过 -t 指定其他话题
"""

import sys
import os
import argparse
from datetime import datetime
import rospy
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2
from pantilt_camera_serial.srv import PantiltCommand, PantiltCommandRequest
from pantilt_camera_serial.msg import PantiltAngleInfo


def get_service_proxy(ns):
    service_name = "/{}/send_command".format(ns)
    rospy.wait_for_service(service_name, timeout=5.0)
    return rospy.ServiceProxy(service_name, PantiltCommand)


def cmd_set_angle(args):
    try:
        proxy = get_service_proxy(args.ns)
        req = PantiltCommandRequest()
        req.command_name = "SetPantiltAngle"
        req.data = [args.heading, args.roll, args.pitch]
        resp = proxy(req)
        if resp.success:
            rospy.loginfo("设置角度成功: heading=%.2f, roll=%.2f, pitch=%.2f",
                          args.heading, args.roll, args.pitch)
        else:
            rospy.logwarn("设置角度失败: %s", resp.message)
    except rospy.exceptions.ROSException as e:
        rospy.logerr("无法连接到服务 /%s/send_command: %s", args.ns, e)
        sys.exit(1)


def cmd_center(args):
    try:
        proxy = get_service_proxy(args.ns)
        req = PantiltCommandRequest()
        req.command_name = "BackToCenter"
        req.data = []
        resp = proxy(req)
        if resp.success:
            rospy.loginfo("回中命令发送成功")
        else:
            rospy.logwarn("回中命令失败: %s", resp.message)
    except rospy.exceptions.ROSException as e:
        rospy.logerr("无法连接到服务 /%s/send_command: %s", args.ns, e)
        sys.exit(1)


def cmd_mode(args):
    mode_map = {
        "lock": "SetLockMode",
        "heading": "SetHeadingFollow",
        "heading_pitch": "SetHeadingPitchFollow",
        "full_follow": "SetFullFollowMode",
    }
    if args.mode_name not in mode_map:
        rospy.logerr("未知模式: %s。可选: %s", args.mode_name, ", ".join(mode_map.keys()))
        sys.exit(1)

    try:
        proxy = get_service_proxy(args.ns)
        req = PantiltCommandRequest()
        req.command_name = mode_map[args.mode_name]
        req.data = []
        resp = proxy(req)
        if resp.success:
            rospy.loginfo("模式设置成功: %s", args.mode_name)
        else:
            rospy.logwarn("模式设置失败: %s", resp.message)
    except rospy.exceptions.ROSException as e:
        rospy.logerr("无法连接到服务 /%s/send_command: %s", args.ns, e)
        sys.exit(1)


def cmd_capture(args):
    # 确保输出目录存在
    if not os.path.exists(args.output):
        os.makedirs(args.output)
        rospy.loginfo("创建输出目录: %s", args.output)

    bridge = CvBridge()
    saved = 0

    for i in range(args.count):
        try:
            rospy.loginfo("等待图像话题 %s ... (%d/%d)", args.topic, i + 1, args.count)
            msg = rospy.wait_for_message(args.topic, Image, timeout=10.0)
            cv_img = bridge.imgmsg_to_cv2(msg, desired_encoding="bgr8")

            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
            filename = "{}_{}.jpg".format(args.prefix, timestamp)
            filepath = os.path.join(args.output, filename)

            cv2.imwrite(filepath, cv_img)
            rospy.loginfo("已保存: %s (%dx%d)", filepath, cv_img.shape[1], cv_img.shape[0])
            saved += 1

            if i < args.count - 1 and args.interval > 0:
                rospy.sleep(args.interval)
        except rospy.exceptions.ROSException as e:
            rospy.logerr("获取图像失败: %s", e)
            break

    rospy.loginfo("完成，共保存 %d 张图片到 %s", saved, os.path.abspath(args.output))


def print_angle_info(msg):
    print("=" * 50)
    print("云台角度信息")
    print("=" * 50)
    print("  对地航向角 (heading)     : {:.2f}°".format(msg.heading))
    print("  对地横滚角 (roll)        : {:.2f}°".format(msg.roll))
    print("  对地俯仰角 (pitch)       : {:.2f}°".format(msg.pitch))
    print("  编码器航向角 (enc_heading): {:.2f}°".format(msg.encoder_heading))
    print("  编码器横滚角 (enc_roll)   : {:.2f}°".format(msg.encoder_roll))
    print("  编码器俯仰角 (enc_pitch)  : {:.2f}°".format(msg.encoder_pitch))
    print("=" * 50)


def cmd_info(args):
    topic_name = "/{}/pantilt_angle_info".format(args.ns)

    if args.watch:
        rospy.loginfo("开始持续监听 %s，按 Ctrl+C 停止...", topic_name)

        def cb(msg):
            # 清屏效果，使用回车符覆盖
            print("\r\033[K", end="")
            line = "H:{:7.2f} R:{:7.2f} P:{:7.2f} | EncH:{:7.2f} EncR:{:7.2f} EncP:{:7.2f}".format(
                msg.heading, msg.roll, msg.pitch,
                msg.encoder_heading, msg.encoder_roll, msg.encoder_pitch
            )
            print(line, end="", flush=True)

        sub = rospy.Subscriber(topic_name, PantiltAngleInfo, cb)
        rospy.spin()
        print()  # 最后换行
    else:
        try:
            msg = rospy.wait_for_message(topic_name, PantiltAngleInfo, timeout=5.0)
            print_angle_info(msg)
        except rospy.exceptions.ROSException as e:
            rospy.logerr("无法获取话题 %s 的消息: %s", topic_name, e)
            sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="云台相机控制与查询工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  %(prog)s info -n pantilt_demo
  %(prog)s info -n pantilt_demo --watch
  %(prog)s set -n pantilt_demo --heading 0 --roll 0 --pitch 0
  %(prog)s center -n pantilt_test
  %(prog)s mode -n pantilt_demo lock
  %(prog)s mode -n pantilt_demo full_follow
        """
    )
    parser.add_argument("-n", "--ns", default="pantilt_demo",
                        help="云台节点命名空间 (默认: pantilt_demo)")

    subparsers = parser.add_subparsers(dest="command", help="子命令")

    # info
    p_info = subparsers.add_parser("info", help="查询当前云台角度状态")
    p_info.add_argument("-w", "--watch", action="store_true",
                        help="持续监听并实时显示")

    # set
    p_set = subparsers.add_parser("set", help="设置云台目标角度")
    p_set.add_argument("--heading", type=float, default=0.0, help="航向角 heading (°)")
    p_set.add_argument("--roll", type=float, default=0.0, help="横滚角 roll (°)")
    p_set.add_argument("--pitch", type=float, default=0.0, help="俯仰角 pitch (°)")

    # center
    subparsers.add_parser("center", help="云台回中")

    # mode
    p_mode = subparsers.add_parser("mode", help="设置云台工作模式")
    p_mode.add_argument("mode_name", choices=["lock", "heading", "heading_pitch", "full_follow"],
                        help="模式名称: lock(锁定), heading(航向跟随), heading_pitch(航向俯仰跟随), full_follow(全跟随)")

    # capture
    p_cap = subparsers.add_parser("capture", help="保存相机图像到指定文件夹")
    p_cap.add_argument("-t", "--topic", default="/image_color",
                       help="图像话题 (默认: /image_color)")
    p_cap.add_argument("-o", "--output", default="./captures",
                       help="输出文件夹路径 (默认: ./captures)")
    p_cap.add_argument("-c", "--count", type=int, default=1,
                       help="保存张数 (默认: 1)")
    p_cap.add_argument("--interval", type=float, default=0.0,
                       help="连续保存间隔秒数 (默认: 0)")
    p_cap.add_argument("-p", "--prefix", default="capture",
                       help="文件名前缀 (默认: capture)")

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        sys.exit(1)

    rospy.init_node("pantilt_tool", anonymous=True)

    if args.command == "info":
        cmd_info(args)
    elif args.command == "set":
        cmd_set_angle(args)
    elif args.command == "center":
        cmd_center(args)
    elif args.command == "mode":
        cmd_mode(args)
    elif args.command == "capture":
        cmd_capture(args)


if __name__ == "__main__":
    main()
