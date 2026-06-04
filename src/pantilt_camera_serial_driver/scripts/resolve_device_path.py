#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动解析/校验摄像头设备路径，供 pantiltcamera_ready.launch 调用。

用法:
    resolve_device_path.py [USER_DEVICE_PATH]

逻辑:
    1. 若传入非空路径，校验其存在且可读，通过则输出。
    2. 若传入空路径或 AUTO，自动扫描 /dev/v4l/by-path，
       选取第一个有效的 *-video-index0 设备。
    3. 校验失败或找不到设备时，向 stderr 输出错误信息并返回非 0。
"""

import os
import sys

BY_PATH_DIR = "/dev/v4l/by-path"


def log_error(msg):
    sys.stderr.write("[resolve_device_path] ERROR: {}\n".format(msg))


def log_info(msg):
    sys.stderr.write("[resolve_device_path] INFO: {}\n".format(msg))


def validate_device(path):
    """校验路径是否为存在且可读的设备文件。"""
    if not os.path.exists(path):
        return False, "路径不存在: {}".format(path)
    if not os.access(path, os.R_OK):
        return False, "路径不可读: {}".format(path)
    return True, None


def auto_detect():
    """自动扫描 /dev/v4l/by-path，返回第一个有效的 video-index0 设备。"""
    if not os.path.isdir(BY_PATH_DIR):
        return None, "目录不存在: {}".format(BY_PATH_DIR)

    candidates = []
    try:
        entries = os.listdir(BY_PATH_DIR)
    except OSError as e:
        return None, "无法读取目录 {}: {}".format(BY_PATH_DIR, e)

    for name in entries:
        # video-index0 是主采集节点，video-index1 通常是 metadata
        if name.endswith("-video-index0"):
            full_path = os.path.join(BY_PATH_DIR, name)
            ok, err = validate_device(full_path)
            if ok:
                candidates.append(full_path)
            else:
                log_info("跳过无效候选 {} — {}".format(full_path, err))

    if not candidates:
        return None, "在 {} 中未找到有效的 *-video-index0 设备".format(BY_PATH_DIR)

    candidates.sort()
    chosen = candidates[0]
    if len(candidates) > 1:
        log_info("检测到多个摄像头，自动选择: {}".format(chosen))
        for c in candidates[1:]:
            log_info("  其他候选: {}".format(c))
    else:
        log_info("自动检测到摄像头: {}".format(chosen))

    return chosen, None


def main():
    user_path = sys.argv[1] if len(sys.argv) > 1 else ""

    # 处理用户传入的路径
    if user_path and user_path.strip().upper() not in ("", "AUTO"):
        user_path = user_path.strip()
        ok, err = validate_device(user_path)
        if ok:
            print(user_path)
            return 0
        else:
            log_error("用户指定设备无效 — {}".format(err))
            return 1

    # 自动检测
    path, err = auto_detect()
    if err:
        log_error(err)
        return 1

    sys.stdout.write(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
