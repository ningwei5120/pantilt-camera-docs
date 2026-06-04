#ifndef PANTILT_SERIAL_CONTROL_H
#define PANTILT_SERIAL_CONTROL_H

#include <ros/ros.h>

#include <pantilt_camera_serial/serial_protocol.h>
#include <geometry_msgs/Twist.h>

#include "pantilt_camera_serial/PantiltAngleInfo.h"
#include "pantilt_camera_serial/PantiltCommand.h"

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>


namespace autolabor_driver{

    typedef boost::shared_ptr<boost::asio::serial_port> serial_port_ptr;

    class PantiltSerialControl {
    public:
        PantiltSerialControl();
        ~PantiltSerialControl();

        void run();

    private:
        // 初始化
        bool initializeSerialConnection();
        // 打开串口接收
        void startAsynchronousReceive();
        // 处理接收到的数据
        void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
        // 校验数据
        void parseAndProcessFrames();
        // 解析一包数据
        void handleValidFrame(const std::vector<uint8_t>& frame);
        // 处理cmd_vel消息
        void handle_twist_msg(const geometry_msgs::Twist::ConstPtr &msg);
        // 处理服务请求
        bool handle_send_command(pantilt_camera_serial::PantiltCommand::Request &req,pantilt_camera_serial::PantiltCommand::Response &res);
        // 发送命令
        std::vector<uint8_t> send_command(const std::string& commandName, const std::vector<uint8_t>& data);
        // 将接收的数据转换为十六进制字符串，用于调试
        std::string convertDataToHex(const std::vector<uint8_t>& data);
        // 角度查询定时器回调
        void query_timer_callback(const ros::TimerEvent& event);

    private:
        // 串口相关
        std::string port_name_;
        int baud_rate_;
        boost::system::error_code ec_;
        boost::asio::io_service io_service_;
        serial_port_ptr port_;
        boost::mutex mutex_;
        // 串口解析相关
        std::vector<uint8_t> receive_data_;
        std::vector<uint8_t> buffer_; // 用于存储接收到的数据和处理不完整帧
        size_t current_index_ = 0;    // 当前处理到的位置
        // 角度查询
        int query_rate_;
        ros::Timer query_timer_;
        // ROS相关
        ros::Publisher angle_publisher_;
        ros::Subscriber twist_subscriber_;
        ros::ServiceServer cmd_service_;

    };

}



#endif
