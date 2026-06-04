#include "../include/rviz_pantilt_plugin/pantilt_plugin.h"

namespace autolabor_plugin {

     PantiltRvizPanel::PantiltRvizPanel(QWidget *parent) : rviz::Panel(parent) {
        QGridLayout *layout = new QGridLayout(this);

        // Node selector
        QGroupBox *nodeGroupBox = new QGroupBox("Node Selector");
        QGridLayout *nodeLayout = new QGridLayout(nodeGroupBox);
        select_node_label_ = new QLabel("Pantilt Name:");
        node_selector_ = new QComboBox();
        nodeLayout->addWidget(select_node_label_, 0, 0);
        nodeLayout->addWidget(node_selector_, 0, 1);
        nodeLayout->setColumnStretch(0, 1);  // 第一列的拉伸系数设置为1
        nodeLayout->setColumnStretch(1, 3);  // 第二列的拉伸系数设置为3，使得下拉列表比标签宽

        nodeGroupBox->setLayout(nodeLayout);
        layout->addWidget(nodeGroupBox, 0, 0, 1, 2); // Span across two columns

        // Mode settings group
        QGroupBox *modeGroupBox = new QGroupBox("Mode Settings");
        QGridLayout *modeLayout = new QGridLayout(modeGroupBox);

        set_lock_mode_button_ = new QPushButton("LockMode");
        set_heading_follow_button_ = new QPushButton("HeadingFollow");
        set_heading_pitch_follow_button_ = new QPushButton("HeadPitchFollow");
        set_full_follow_mode_button_ = new QPushButton("FullFollow");

        modeLayout->addWidget(set_lock_mode_button_, 0, 0);
        modeLayout->addWidget(set_heading_follow_button_, 0, 1);
        modeLayout->addWidget(set_heading_pitch_follow_button_, 1, 0);
        modeLayout->addWidget(set_full_follow_mode_button_, 1, 1);

        modeGroupBox->setLayout(modeLayout);
        layout->addWidget(modeGroupBox, 1, 0, 1, 2); // Span across two columns

        // Angle control group
        QGroupBox *angleGroupBox = new QGroupBox("Angle Control");
        QGridLayout *angleLayout = new QGridLayout(angleGroupBox);

        head_label_ = new QLabel("Head:");
        head_input_ = new QLineEdit;
        roll_label_ = new QLabel("Roll:");
        roll_input_ = new QLineEdit;
        pitch_label_ = new QLabel("Pitch:");
        pitch_input_ = new QLineEdit;
        send_angles_button_ = new QPushButton("Send Angles");
        center_button_ = new QPushButton("Back to Center");

        angleLayout->addWidget(head_label_, 0, 0);
        angleLayout->addWidget(head_input_, 0, 1);
        angleLayout->addWidget(roll_label_, 1, 0);
        angleLayout->addWidget(roll_input_, 1, 1);
        angleLayout->addWidget(pitch_label_, 2, 0);
        angleLayout->addWidget(pitch_input_, 2, 1);
        angleLayout->addWidget(send_angles_button_, 3, 0, 1, 2); // Span across two columns
        angleLayout->addWidget(center_button_, 4, 0, 1, 2); // Span across two columns

        angleGroupBox->setLayout(angleLayout);
        layout->addWidget(angleGroupBox, 2, 0, 1, 2); // Span across two columns

        angle_label_ = new QLabel("Current Angle: 0.0");
        layout->addWidget(angle_label_, 3, 0, 1, 2); // Span across two columns

        log_label_ = new QLabel("Log:");
        layout->addWidget(log_label_, 4, 0, 1, 2); // Span across two columns

        // Connect signals to slots
        connect(set_lock_mode_button_, SIGNAL(clicked()), this, SLOT(setLockMode()));
        connect(set_heading_follow_button_, SIGNAL(clicked()), this, SLOT(setHeadingFollow()));
        connect(set_heading_pitch_follow_button_, SIGNAL(clicked()), this, SLOT(setHeadingPitchFollow()));
        connect(set_full_follow_mode_button_, SIGNAL(clicked()), this, SLOT(setFullFollowMode()));
        connect(send_angles_button_, SIGNAL(clicked()), this, SLOT(sendAngles()));
        connect(center_button_, SIGNAL(clicked()), this, SLOT(backToCenter()));

        // Connect combo box change signal to update function
        connect(node_selector_, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSelectedNode(int)));
        // 安装全局事件过滤器
        this->installEventFilter(this);
        // 允许面板接收焦点
        this->setFocusPolicy(Qt::StrongFocus);
        // 安装combobox事件过滤器
        node_selector_->installEventFilter(this);

    }

    void PantiltRvizPanel::setLockMode() {
        pantilt_camera_serial::PantiltCommand srv;
        srv.request.command_name = "SetLockMode";
        callService(srv, "SetLockMode");
    }

    void PantiltRvizPanel::setHeadingFollow() {
        pantilt_camera_serial::PantiltCommand srv;
        srv.request.command_name = "SetHeadingFollow";
        callService(srv, "SetHeadingFollow");
    }

    void PantiltRvizPanel::setHeadingPitchFollow() {
        pantilt_camera_serial::PantiltCommand srv;
        srv.request.command_name = "SetHeadingPitchFollow";
        callService(srv, "SetHeadingPitchFollow");
    }

    void PantiltRvizPanel::setFullFollowMode() {
        pantilt_camera_serial::PantiltCommand srv;
        srv.request.command_name = "SetHeadingPitchFollow";
        callService(srv, "SetFullFollowMode");
    }


    void PantiltRvizPanel::backToCenter() {
        pantilt_camera_serial::PantiltCommand srv;
        srv.request.command_name = "BackToCenter";
        srv.request.data.push_back(10.0);  // 示例数据
        callService(srv, "BackToCenter");
    }

    void PantiltRvizPanel::sendAngles() {
        pantilt_camera_serial::PantiltCommand srv;
        srv.request.command_name = "SetPantiltAngle";
        srv.request.data.push_back(head_input_->text().toDouble());
        srv.request.data.push_back(roll_input_->text().toDouble());
        srv.request.data.push_back(pitch_input_->text().toDouble());
        callService(srv, "SetPantiltAngle");
    }

    void PantiltRvizPanel::angleCallback(const pantilt_camera_serial::PantiltAngleInfo::ConstPtr& msg) {
        QString text = QString("Current: Head: %1, Roll: %2, Pitch: %3")
                .arg(msg->encoder_heading, 0, 'f', 2)
                .arg(msg->encoder_roll, 0, 'f', 2)
                .arg(msg->encoder_pitch, 0, 'f', 2);
        QMetaObject::invokeMethod(angle_label_, "setText", Qt::QueuedConnection, Q_ARG(QString, text));
    }

    bool PantiltRvizPanel::eventFilter(QObject* watched, QEvent* event) {
        // 处理鼠标按钮按下事件
        if (watched == node_selector_ && event->type() == QEvent::MouseButtonPress) {
            populateNodeSelector();
        }
        // 处理键盘按键事件
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            handleKeyPressEvent(keyEvent);
        }
        else if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            handleKeyReleaseEvent(keyEvent);
        }

        return QWidget::eventFilter(watched, event);  // 对其他事件使用默认处理
    }

    void PantiltRvizPanel::handleKeyPressEvent(QKeyEvent *event) {
        switch (event->key()) {
            case Qt::Key_Up:
            case Qt::Key_W:
                pitch_up_ = true;
                break;
            case Qt::Key_Down:
            case Qt::Key_S:
                pitch_down_ = true;
                break;
            case Qt::Key_Left:
            case Qt::Key_A:
                head_left_ = true;
                break;
            case Qt::Key_Right:
            case Qt::Key_D:
                head_right_ = true;
                break;
            default:
                return;  // 如果不是我们关心的键，则直接返回
        }
    }


    void PantiltRvizPanel::handleKeyReleaseEvent(QKeyEvent *event) {
        switch (event->key()) {
            case Qt::Key_Up:
            case Qt::Key_W:
                pitch_up_ = false;
                break;
            case Qt::Key_Down:
            case Qt::Key_S:
                pitch_down_ = false;
                break;
            case Qt::Key_Left:
            case Qt::Key_A:
                head_left_ = false;
                break;
            case Qt::Key_Right:
            case Qt::Key_D:
                head_right_ = false;
                break;
            default:
                return;  // 如果不是我们关心的键，则直接返回
        }
    }

    void PantiltRvizPanel::populateNodeSelector() {
        node_selector_->clear();

        ros::master::V_TopicInfo master_topics;
        ros::master::getTopics(master_topics);

        for (const auto& topic : master_topics) {
            // 这里使用ROS消息类型的信息进行比较
            if (ros::message_traits::DataType<pantilt_camera_serial::PantiltAngleInfo>::value() == topic.datatype) {
                // 检查主题名称是否以特定的后缀结束
                if (topic.name.rfind("/pantilt_angle_info") != std::string::npos) {
                    std::string node_namespace = topic.name.substr(0, topic.name.rfind("/pantilt_angle_info"));
                    if (!node_namespace.empty()) {
                        node_selector_->addItem(QString::fromStdString(node_namespace));
                    }
                }
            }
        }
    }

    void PantiltRvizPanel::updateSelectedNode(int index) {
        QString selectedNode = node_selector_->currentText();
        // Update service and subscriber based on selected node
        service_client_ = nh_.serviceClient<pantilt_camera_serial::PantiltCommand>(selectedNode.toStdString() + "/send_command");
        angle_sub_ = nh_.subscribe(selectedNode.toStdString() + "/pantilt_angle_info", 1, &PantiltRvizPanel::angleCallback, this);
        vel_pub_ = nh_.advertise<geometry_msgs::Twist>(selectedNode.toStdString() + "/pantilt_vel", 1);

        // 重置按键状态
        pitch_up_ = pitch_down_ = head_left_ = head_right_ = false;
        vel_send_timer_ = nh_.createTimer(ros::Duration(0.1), &PantiltRvizPanel::sendVel, this);

    }

    void PantiltRvizPanel::sendVel(const ros::TimerEvent &event) {
        geometry_msgs::Twist vel;
        if (head_left_) {
            vel.angular.z = 0.5;
        } else if (head_right_) {
            vel.angular.z = -0.5;
        } else {
            vel.angular.z = 0.0;
        }

        if (pitch_up_) {
            vel.linear.x = 0.5;
        } else if (pitch_down_) {
            vel.linear.x = -0.5;
        } else {
            vel.linear.x = 0.0;
        }

        vel_pub_.publish(vel);
     }

    bool PantiltRvizPanel::callService(pantilt_camera_serial::PantiltCommand& srv, const QString& serviceName) {
        if (service_client_.call(srv)) {
            if (srv.response.success) {
                QMessageBox::information(this, "Service Call", "Success: " + QString::fromStdString(srv.response.message));
                return true;
            } else {
                QMessageBox::critical(this, "Service Call", "Failed: " + QString::fromStdString(srv.response.message));
            }
        } else {
            QMessageBox::critical(this, "Service Call", "Failed to call service: " + serviceName);
        }
        return false;
    }

}

#include <pluginlib/class_list_macros.h>

PLUGINLIB_EXPORT_CLASS(autolabor_plugin::PantiltRvizPanel, rviz::Panel)