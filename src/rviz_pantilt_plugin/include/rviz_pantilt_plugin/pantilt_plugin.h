//
// Created by autolabor on 24-6-21.
//

#ifndef RVIZ_PANTILT_PLUGIN_PANTILT_PLUGIN_H
#define RVIZ_PANTILT_PLUGIN_PANTILT_PLUGIN_H

#include <ros/ros.h>
#include <rviz/panel.h>
#include <geometry_msgs/Twist.h>

#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QGroupBox>
#include <QEvent>
#include <QKeyEvent>

#include "pantilt_camera_serial/PantiltCommand.h"
#include "pantilt_camera_serial/PantiltAngleInfo.h"

namespace autolabor_plugin {


    class PantiltRvizPanel : public rviz::Panel {
    Q_OBJECT
    public:
        PantiltRvizPanel(QWidget *parent = nullptr);


    public Q_SLOTS:
        // 用于响应 UI 事件的槽
        // 设置锁定模式
        void setLockMode();
        // 设置跟随头部
        void setHeadingFollow();
        // 设置跟随头部和俯仰
        void setHeadingPitchFollow();
        // 设置全跟随模式
        void setFullFollowMode();
        // 摄像头回中
        void backToCenter();
        // 发送目标角度
        void sendAngles();
        // 更新选择的节点
        void updateSelectedNode(int index);
        // 更新节点选择
        void populateNodeSelector();

    private:
        // 事件过滤器
        bool eventFilter(QObject* watched, QEvent* event);
        // 用于接收角度信息的回调函数
        void angleCallback(const pantilt_camera_serial::PantiltAngleInfo::ConstPtr &msg);
        // 处理键盘按下事件
        void handleKeyPressEvent(QKeyEvent *event);
        // 处理键盘松开事件
        void handleKeyReleaseEvent(QKeyEvent *event);
        // 发送摄像头速度
        void sendVel(const ros::TimerEvent& event);
        // 服务调用
        bool callService(pantilt_camera_serial::PantiltCommand& srv, const QString& serviceName);

    private:
        // 按键标识位
        bool pitch_up_, pitch_down_, head_left_, head_right_;
        // ROS 相关
        ros::NodeHandle nh_;
        ros::ServiceClient service_client_;
        ros::Subscriber angle_sub_;
        ros::Publisher vel_pub_;
        ros::Timer vel_send_timer_;
        // 界面元素
        QComboBox* node_selector_;
        QLabel* select_node_label_;

        QPushButton *set_lock_mode_button_;
        QPushButton *set_heading_follow_button_;
        QPushButton *set_heading_pitch_follow_button_;
        QPushButton *set_full_follow_mode_button_;

        QPushButton *center_button_;
        QLineEdit *angle_input_;
        QLabel *angle_label_;

        QPushButton *send_angles_button_;
        QLineEdit *head_input_;
        QLineEdit *roll_input_;
        QLineEdit *pitch_input_;
        QLabel *head_label_;
        QLabel *roll_label_;
        QLabel *pitch_label_;

        QLabel *log_label_;

    };

}

#endif //RVIZ_PANTILT_PLUGIN_PANTILT_PLUGIN_H
