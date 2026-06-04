#ifndef PANTILT_CAMERA_SERIAL_SERIAL_PROTOCOL_H
#define PANTILT_CAMERA_SERIAL_SERIAL_PROTOCOL_H

#include <ros/ros.h>

#include "pantilt_camera_serial/PantiltAngleInfo.h"

namespace autolabor_driver{

    struct CommandInfo {
        uint8_t cmd; // 命令
        std::vector<uint8_t> Params; // 默认参数
    };

    class PantiltProtocol {

    public:
        static const uint8_t HEADER = 0xAA; // 协议头
        static const uint8_t FEEDBACK_HEADER = 0x55; // 反馈头

        static const uint8_t LENGTH_IDX = 1;
        static const uint8_t COMMAND_IDX = 2;

        static constexpr double MaxHeading = 160.0;
        static constexpr double MinHeading = -160.0;
        static constexpr double MaxRoll = 40.0;
        static constexpr double MinRoll = -40.0;
        static constexpr double MaxPitch = 90.0;
        static constexpr double MinPitch = -90.0;


        struct Error {
            static const uint8_t NO_ERROR = 0x00; //无错误
            static const uint8_t NO_HEADER = 0x01; // 没有0xaa头字节
            static const uint8_t NO_VALID_COMMAND = 0x02; // 没有接收到正确的命令
            static const uint8_t PARAMETER_MISMATCH = 0x03; // 输入参数不等于计算总字节数
            static const uint8_t CHECKSUM_ERROR = 0x04; // 校验出来的结果与校验位不相等，校验错误
        };

        static const std::map<std::string, CommandInfo> commandMap;

    };

    class PantiltUtils {

    public:
        // 构建命令的方法
        static std::vector<uint8_t> buildCommand(const std::string& commandName, const std::vector<uint8_t>& data) {
            std::vector<uint8_t> cmd;
            uint8_t header = PantiltProtocol::HEADER;
            auto it = PantiltProtocol::commandMap.find(commandName);
            if (it != PantiltProtocol::commandMap.end()) {
                const CommandInfo& cmdInfo = it->second;
                // 添加参数长度，计算总长度：命令1字节 + 参数长度 + 数据长度 + CRC 1字节
                uint8_t length = 2 + 1 + static_cast<uint8_t>(cmdInfo.Params.size() + data.size()) + 1;
                cmd.push_back(header); // 添加协议头
                cmd.push_back(length); // 添加长度
                cmd.push_back(cmdInfo.cmd); // 添加命令字节
                cmd.insert(cmd.end(), cmdInfo.Params.begin(), cmdInfo.Params.end()); // 插入默认参数
                cmd.insert(cmd.end(), data.begin(), data.end()); // 插入数据数组
                uint8_t crc = crc8_calculate(cmd); // 计算CRC
                cmd.push_back(crc); // 添加CRC
            } else {
                std::cerr << "Command not found" << std::endl;
            }
            return cmd;
        }

        // 解析16位有符号整数
        static int16_t parse_int16(const std::vector<uint8_t>& data, size_t index) {
            return static_cast<int16_t>(data[index]| data[index + 1] << 8 );
        }

        // 从 int16_t 转换到 uint8_t 数组，低位在前
        static std::vector<uint8_t> int16_to_uint8_vector(int16_t value) {
            std::vector<uint8_t> bytes;
            bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF)); // 高字节
            bytes.push_back(static_cast<uint8_t>(value & 0xFF));        // 低字节
            return bytes;
        }

        // 解析数据帧
        static pantilt_camera_serial::PantiltAngleInfo parse_angles(const std::vector<uint8_t>& frame) {
            pantilt_camera_serial::PantiltAngleInfo angles;
            angles.heading = parse_int16(frame, 4) / 100.0;          // 从第4个字节开始
            angles.roll = parse_int16(frame, 6) / 100.0;
            angles.pitch = parse_int16(frame, 8) / 100.0;
            angles.encoder_heading = parse_int16(frame, 10) / 100.0;
            angles.encoder_roll = parse_int16(frame, 12) / 100.0;
            angles.encoder_pitch = parse_int16(frame, 14) / 100.0;
            return angles;
        }

        // Function to prepare motion data with range checks
        static std::vector<uint8_t> prepareMotionData(double heading, double roll, double pitch) {
            std::vector<uint8_t> data;
            // Check ranges and prepare data for "SetPantiltAngle"
            if (heading < PantiltProtocol::MinHeading || heading > PantiltProtocol::MaxHeading ||
                roll < PantiltProtocol::MinRoll || roll > PantiltProtocol::MaxRoll ||
                pitch < PantiltProtocol::MinPitch || pitch > PantiltProtocol::MaxPitch) {
                std::cerr << "Error: One or more parameters are out of the allowable range." << std::endl;
                return data; // Return empty vector on error
            }

            // Convert angles to int16_t and multiply by 10 for precision handling
            int16_t heading_int = static_cast<int16_t>(heading * 10);
            int16_t roll_int = static_cast<int16_t>(roll * 10);
            int16_t pitch_int = static_cast<int16_t>(pitch * 10);

            // Convert int16_t values to byte vectors and append to data
            auto heading_bytes = int16_to_uint8_vector(heading_int);
            auto roll_bytes = int16_to_uint8_vector(roll_int);
            auto pitch_bytes = int16_to_uint8_vector(pitch_int);

            data.insert(data.end(), heading_bytes.begin(), heading_bytes.end());
            data.insert(data.end(), roll_bytes.begin(), roll_bytes.end());
            data.insert(data.end(), pitch_bytes.begin(), pitch_bytes.end());

            data.push_back(0x01); // Add slow command speed byte

            return data;
        }

        static std::pair<bool, std::string> checkFrameError(const std::vector<uint8_t>& frame) {
            if (frame.size() < 4) {
                return std::make_pair(false, "Error: Frame is too short to check error code.");
            }

            uint8_t errorCode = frame[3];
            switch (errorCode) {
                case PantiltProtocol::Error::NO_ERROR:
                    return std::make_pair(true, "No error.");
                case PantiltProtocol::Error::NO_HEADER:
                    return std::make_pair(false, "Error: No 0xAA header byte found.");
                case PantiltProtocol::Error::NO_VALID_COMMAND:
                    return std::make_pair(false, "Error: No valid command received.");
                case PantiltProtocol::Error::PARAMETER_MISMATCH:
                    return std::make_pair(false, "Error: Input parameters do not match calculated total byte count.");
                case PantiltProtocol::Error::CHECKSUM_ERROR:
                    return std::make_pair(false, "Error: Checksum mismatch, validation failed.");
                default:
                    return std::make_pair(false, "Error: Unknown error code.");
            }
        }

        static uint8_t crc8_calculate(const std::vector<uint8_t>& data){
            uint8_t crc = 0; // 初始CRC值
            for (auto byte : data) {
                crc ^= byte;
                for (int i = 0; i < 8; ++i) {
                    if (crc & 0x80) {
                        crc = (crc << 1) ^ 0xD5; // 假设CRC多项式是0xD5
                    } else {
                        crc <<= 1;
                    }
                }
            }
            return crc;
        }
    };

    const std::map<std::string, CommandInfo> PantiltProtocol::commandMap = {
            {"GetCameraVersion",   {0x00, {0x01}}},                 // 获取相机版本
            {"GetProtocolVersion", {0x00, {0x02}}},                 // 获取协议版本
            {"GetCameraMode",      {0x00, {0x03}}},                 // 获取相机模式
            {"GetPantiltMode",     {0x00, {0x05,0x01}}},        // 获取云台模式
            {"GetPantiltPose",     {0x00, {0x05,0x02}}},        // 获取相机角度
            {"SetLockMode",        {0x05, {0x01, 0x00}}},       // 锁定模式
            {"SetHeadingFollow",   {0x05, {0x01, 0x01}}},       // 航向跟随, 俯仰锁定
            {"SetHeadingPitchFollow", {0x05, {0x01, 0x02}}},    // 航向俯仰跟随TF, 云台跟随
            {"SetFullFollowMode",  {0x05, {0x01, 0x03}}},       // 全跟随
            {"BackToCenter",       {0x05, {0x02}}},                 // 回中
            {"SetPantiltSpeed",    {0x05, {0x06}}},                 // 设置云台速度
            {"SetPantiltAngle",    {0x05, {0x05}}}                  // 设置云台角度
            // 可以继续添加更多命令配置
    };


}

#endif //PANTILT_CAMERA_SERIAL_SERIAL_PROTOCOL_H
