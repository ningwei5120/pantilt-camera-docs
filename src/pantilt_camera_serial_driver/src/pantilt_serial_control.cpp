
#include <pantilt_camera_serial/pantilt_serial_control.h>

namespace autolabor_driver {

    PantiltSerialControl::PantiltSerialControl() {
        ROS_INFO("PantiltSerialControl is running...");
    }

    PantiltSerialControl::~PantiltSerialControl() {
        boost::mutex::scoped_lock look(mutex_);
        if (port_) {
            port_->cancel();
            port_->close();
            port_.reset();
        }
        io_service_.stop();
        io_service_.reset();
    }

    /**
     * @brief 初始化并启动控制循环。
     *
     * 初始化 ROS 订阅者、服务和发布者，设置串口，并进入 ROS 事件循环处理收到的消息和命令。
     */
    void PantiltSerialControl::run() {
        ros::NodeHandle node;
        ros::NodeHandle private_node("~");

        private_node.param<std::string>("port_name", port_name_, std::string("/dev/ttyUSB0"));
        private_node.param<int>("baud_rate", baud_rate_, 115200);
        private_node.param<int>("query_rate", query_rate_, 1);

        twist_subscriber_ = private_node.subscribe("pantilt_vel", 10, &PantiltSerialControl::handle_twist_msg, this);
        query_timer_ = private_node.createTimer(ros::Duration(1.0 / query_rate_), &PantiltSerialControl::query_timer_callback, this);
        angle_publisher_ = private_node.advertise<pantilt_camera_serial::PantiltAngleInfo>("pantilt_angle_info", 10);
        cmd_service_ = private_node.advertiseService("send_command", &PantiltSerialControl::handle_send_command,this);

        if (initializeSerialConnection()) {
            ROS_INFO("PantiltSerialControl initializeSerialConnection success...");
            startAsynchronousReceive();
            boost::thread io_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
        }
        ros::spin();
    }

    /**
     * @brief 初始化串口连接。
     *
     * 打开串口，设置串口参数，如波特率和字符大小。
     * @return bool 初始化成功返回 true，否则返回 false。
     */
    bool PantiltSerialControl::initializeSerialConnection() {
        if (port_) {
            ROS_ERROR("error : port is already opened...");
            return false;
        }
        port_ = serial_port_ptr(new boost::asio::serial_port(io_service_));
        port_->open(port_name_, ec_);
        if (ec_) {
            ROS_INFO_STREAM("error : port_->open() failed...port_name=" << port_name_ << ", e=" << ec_.message().c_str());
            return false;
        }

        // option settings...
        port_->set_option(boost::asio::serial_port_base::baud_rate(static_cast<unsigned int>(baud_rate_)));
        port_->set_option(boost::asio::serial_port_base::character_size(8));
        port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
        port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
        port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

        return true;
    }

    /**
     * @brief 开始异步接收串口数据。
     *
     * 配置串口进行异步读取操作，并设置回调函数处理接收到的数据。
     */
    void PantiltSerialControl::startAsynchronousReceive() {
        receive_data_.resize(256); // Resize the buffer to a suitable size for your needs
        port_->async_read_some(boost::asio::buffer(receive_data_),
                               boost::bind(&PantiltSerialControl::handle_receive, this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
    }

    /**
     * @brief 处理从串口接收到的数据。
     *
     * 解析串口数据，处理数据包，并根据需要重新调用开始接收函数。
     * @param error 接收过程中出现的错误
     * @param bytes_transferred 接收到的字节数
     */
    void PantiltSerialControl::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
        if (!error) {
            std::copy(receive_data_.begin(), receive_data_.begin() + bytes_transferred, std::back_inserter(buffer_));
            parseAndProcessFrames();
            // Continue the asynchronous receive
            startAsynchronousReceive();

        } else {
            ROS_ERROR_STREAM("Error on receive: " << error.message());
        }
    }

    /**
     * @brief 处理接收到的串口数据。
     *
     * 从缓冲区中解析完整的数据帧，进行CRC校验，处理有效的帧数据。
     * 识别并跳过非帧头数据，检查数据长度并处理每个有效帧。
     */
    void PantiltSerialControl::parseAndProcessFrames() {
        while (current_index_ + 1 < buffer_.size()) {
            // 快速跳过非帧头数据
            if (buffer_[current_index_] != 0x55) {
                current_index_++;
                continue;
            }
            // 检查是否有足够的数据获取长度
            if (current_index_ + 1 >= buffer_.size()) {
                break; // 退出循环，等待更多数据
            }
            size_t length = buffer_[current_index_ + 1];
            // 检查是否有足够的数据完成整个帧
            if (current_index_ + length + 1 > buffer_.size()) {
                break; // 数据不足，等待更多数据
            }
            std::vector<uint8_t> crc_frame(buffer_.begin() + current_index_, buffer_.begin() + current_index_ + length - 1);
            // 从帧头到校验位前一个字节计算CRC
            uint8_t calculated_crc = PantiltUtils::crc8_calculate(crc_frame);
            if (calculated_crc == buffer_[current_index_ + length - 1]) { // CRC校验通过
                std::vector<uint8_t> frame(buffer_.begin() + current_index_, buffer_.begin() + current_index_ + length);
                // 处理帧数据
                handleValidFrame(frame);
                current_index_ += length; // 移动到下一个帧的起始位置
            } else {
                current_index_++; // CRC校验失败，寻找下一个可能的帧头
            }
        }

        // 处理完所有完整的帧后，重新排列buffer以包含未处理的数据
        if (current_index_ < buffer_.size()) {
            buffer_ = std::vector<uint8_t>(buffer_.begin() + current_index_, buffer_.end());
            current_index_ = 0;
        } else {
            buffer_.clear(); // 清空缓存
            current_index_ = 0;
        }
    }

    /**
     * @brief 处理解析后的数据帧。
     *
     * 根据数据帧的内容执行相应的处理逻辑。
     * @param frame 从串口接收并解析得到的完整数据帧
     */
    void PantiltSerialControl::handleValidFrame(const std::vector<uint8_t>& frame) {
        // 判断指令是否执行成功
        auto result = PantiltUtils::checkFrameError(frame);
        if (!result.first) {
            ROS_ERROR_STREAM(result.second);
            return;
        }

        // 判断是否为查询指令
        if(frame[PantiltProtocol::COMMAND_IDX] == 0x00 && frame[PantiltProtocol::LENGTH_IDX] == 0x12)
        {
            // 解析数值
            pantilt_camera_serial::PantiltAngleInfo angle_info = PantiltUtils::parse_angles(frame);
            angle_publisher_.publish(angle_info);
            ROS_DEBUG_STREAM("Heading: " << angle_info.heading <<
                                        ", Roll: " << angle_info.roll <<
                                        ", Pitch: " << angle_info.pitch <<
                                        ", Encoder Heading: " << angle_info.encoder_heading <<
                                        ", Encoder Roll: " << angle_info.encoder_roll <<
                                        ", Encoder Pitch: " << angle_info.encoder_pitch);
        }
    }

    /**
     * @brief 从 Twist 消息生成并发送平移倾斜命令。
     *
     * 处理 ROS Twist 消息，并将其转换为平移倾斜速度命令发送到硬件。
     * @param msg 接收到的 Twist 消息指针
     */
    void PantiltSerialControl::handle_twist_msg(const geometry_msgs::Twist::ConstPtr &msg)
    {
        // 准备发送的数据
        std::vector<uint8_t> data;

        // 获取 angular.z 的字节并添加到数据向量中
        std::vector<uint8_t> angular_z_bytes = PantiltUtils::int16_to_uint8_vector(static_cast<int16_t>(-msg->angular.z * 50));
        data.insert(data.end(), angular_z_bytes.begin(), angular_z_bytes.end());

        // 获取 linear.x 的字节并添加到数据向量中
        std::vector<uint8_t> linear_x_bytes = PantiltUtils::int16_to_uint8_vector(static_cast<int16_t>(-msg->linear.x * 50));
        data.insert(data.end(), linear_x_bytes.begin(), linear_x_bytes.end());

        // 发送命令
        std::vector<uint8_t> cmd = send_command("SetPantiltSpeed", data);
    }

    /**
     * @brief 处理发送命令服务的请求。
     *
     * 接收并处理来自 ROS 服务的命令发送请求，返回执行结果。
     * @param req 服务请求
     * @param res 服务响应
     * @return bool 请求处理成功返回 true，否则返回 false。
     */
    bool PantiltSerialControl::handle_send_command(pantilt_camera_serial::PantiltCommand::Request &req,
                                                   pantilt_camera_serial::PantiltCommand::Response &res)
    {
        std::vector<uint8_t> data;
        if (req.command_name == "SetPantiltAngle") {
            data = PantiltUtils::prepareMotionData(req.data[0], req.data[1], req.data[2]);
            if(data.empty())
            {
                res.success = false;
                res.message = "Invalid data for SetPantiltAngle command.";
                return true;
            }
        }
        std::vector<uint8_t> response = send_command(req.command_name, data);
        res.success = true;
        res.message = "Command sent successfully.";
        return true;
    }

    /**
     * @brief 发送命令到串口。
     *
     * 构建命令数据包并通过串口发送。
     * @param commandName 命令名称
     * @param data 要发送的数据包
     * @return std::vector<uint8_t> 发送的命令数据包
     */
    std::vector<uint8_t> PantiltSerialControl::send_command(const std::string& commandName, const std::vector<uint8_t>& data)
    {
        std::vector<uint8_t> command = PantiltUtils::buildCommand(commandName, data);
//        ROS_INFO_STREAM("Command sent: " << convertDataToHex(command));

        if (!command.empty()) {
            boost::asio::write(*port_, boost::asio::buffer(command), ec_);
            if (ec_) {
                std::cerr << "Error sending command: " << ec_.message() << std::endl;
            }
        } else {
            std::cerr << "Command build failed or empty." << std::endl;
        }
        return command;
    }

    /**
    * @brief 将接收的数据转换为十六进制字符串格式。
    *
    * 主要用于调试，将字节数据转换成可读的十六进制字符串。
    * @param data 要转换的数据
    * @return std::string 转换后的字符串
    */
    std::string PantiltSerialControl::convertDataToHex(const std::vector<uint8_t>& data) {
        std::ostringstream stream;
        stream << std::hex << std::uppercase << std::setfill('0');
        for (size_t i = 0; i < data.size(); ++i) {
            stream << std::setw(2) << static_cast<int>(data[i]) << " ";
        }
        return stream.str();
    }

    /**
     * @brief 定时查询平移倾斜角度的回调函数。
     *
     * 定时发送查询命令以获取当前的平移倾斜角度。
     * @param event 定时器事件
     */
    void PantiltSerialControl::query_timer_callback(const ros::TimerEvent& event) {
        std::vector<uint8_t> data;
        std::vector<uint8_t> cmd = send_command("GetPantiltPose", data);
    }
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "pantilt_serial_driver");
    autolabor_driver::PantiltSerialControl driver;
    driver.run();

    return 0;
}