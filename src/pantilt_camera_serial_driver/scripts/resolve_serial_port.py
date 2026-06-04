#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动解析/校验云台串口路径，供 pantiltcamera_ready.launch 调用。

用法:
    resolve_serial_port.py [USER_PORT]

逻辑:
    1. 若传入非空路径，校验其存在且可读写，通过则输出。
    2. 若传入空路径或 AUTO，扫描 /dev/ttyUSB* 和 /dev/ttyACM*，
       对每个候选串口尝试打开、发送 GetPantiltPose 查询命令，
       验证响应帧的 0x55 头、长度 0x12 及 CRC8，通过则输出。
    3. 校验失败或找不到设备时，向 stderr 输出错误信息并返回非 0。
"""

import os
import sys
import glob
import time

try:
    import serial
except ImportError:
    sys.stderr.write("[resolve_serial_port] ERROR: pyserial 未安装，请执行 pip install pyserial\n")
    sys.exit(1)

BAUDRATE = 115200
TIMEOUT_OPEN = 0.5
PROBE_DELAY = 0.20          # 发送后等待响应
FEEDBACK_HEADER = 0x55
EXPECTED_LENGTH = 0x12      # 角度查询响应固定 18 字节
CRC_POLY = 0xD5

# GetPantiltPose 查询命令: AA 06 00 05 02 6E
PROBE_CMD = bytes([0xAA, 0x06, 0x00, 0x05, 0x02, 0x6E])


def log_error(msg):
    sys.stderr.write("[resolve_serial_port] ERROR: {}\n".format(msg))


def log_info(msg):
    sys.stderr.write("[resolve_serial_port] INFO: {}\n".format(msg))


def crc8(data):
    crc = 0
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ CRC_POLY) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def validate_port(path):
    if not os.path.exists(path):
        return False, "路径不存在"
    if not os.access(path, os.R_OK | os.W_OK):
        return False, "路径不可读写"
    return True, None


def probe_port(path):
    """尝试探测串口是否为云台设备。返回 (bool, str)"""
    try:
        with serial.Serial(
            path,
            BAUDRATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=TIMEOUT_OPEN,
            write_timeout=TIMEOUT_OPEN,
        ) as ser:
            # 清空缓冲区
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            time.sleep(0.05)

            # 发送查询命令
            ser.write(PROBE_CMD)
            ser.flush()

            # 等待响应
            time.sleep(PROBE_DELAY)

            available = ser.in_waiting
            if available == 0:
                return False, "无响应数据"

            data = ser.read(available)

            # 在响应中查找有效帧
            for i in range(len(data)):
                if data[i] != FEEDBACK_HEADER:
                    continue
                if i + 1 >= len(data):
                    break
                frame_len = data[i + 1]
                if i + frame_len > len(data):
                    continue  # 帧不完整，跳过

                frame = data[i:i + frame_len]
                crc_payload = frame[:-1]
                expected_crc = frame[-1]
                if crc8(crc_payload) == expected_crc:
                    # 进一步校验：长度应为 0x12，命令应为 0x00，错误码应为 0x00
                    if (
                        frame_len == EXPECTED_LENGTH
                        and len(frame) > 3
                        and frame[2] == 0x00
                        and frame[3] == 0x00
                    ):
                        return True, None
                    else:
                        return False, "CRC通过但帧内容不符 (len={:02X}, cmd={:02X}, err={:02X})".format(
                            frame_len,
                            frame[2] if len(frame) > 2 else 0xFF,
                            frame[3] if len(frame) > 3 else 0xFF,
                        )
                else:
                    continue

            return False, "未找到有效的反馈帧 (收到 {} 字节)".format(len(data))
    except serial.SerialException as e:
        return False, "串口异常: {}".format(e)
    except Exception as e:
        return False, "探测异常: {}".format(e)


def auto_detect():
    candidates = sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))
    if not candidates:
        return None, "未找到任何 /dev/ttyUSB* 或 /dev/ttyACM* 设备"

    log_info("发现 {} 个串口候选: {}".format(len(candidates), ", ".join(candidates)))

    for path in candidates:
        ok, err = validate_port(path)
        if not ok:
            log_info("跳过 {} — {}".format(path, err))
            continue

        ok, err = probe_port(path)
        if ok:
            log_info("探测 {} 成功 — 云台协议验证通过".format(path))
            return path, None
        else:
            log_info("探测 {} 失败 — {}".format(path, err))

    return None, "所有候选串口均未通过云台协议探测"


def main():
    user_path = sys.argv[1] if len(sys.argv) > 1 else ""

    # 处理用户传入的路径（仅做基本校验，不做协议探测，给用户完全控制权）
    if user_path and user_path.strip().upper() not in ("", "AUTO"):
        user_path = user_path.strip()
        ok, err = validate_port(user_path)
        if ok:
            print(user_path)
            return 0
        else:
            log_error("用户指定串口无效 — {}".format(err))
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
