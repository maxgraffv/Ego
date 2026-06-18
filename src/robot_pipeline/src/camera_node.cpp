#include <atomic>
#include <thread>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>

class CameraNode : public rclcpp::Node
{
public:
    CameraNode() : Node("camera_node"), running_(true)
    {
        declare_parameter<int>("jpeg_quality", 60);
        jpeg_quality_ = get_parameter("jpeg_quality").as_int();

        // best-effort + keep-last-1: never block on a dropped frame, always newest
        rclcpp::QoS qos(rclcpp::KeepLast(1));
        qos.best_effort();
        color_pub_      = create_publisher<sensor_msgs::msg::Image>("/camera/color", qos);
        color_jpeg_pub_ = create_publisher<sensor_msgs::msg::CompressedImage>("/camera/color/compressed", qos);
        depth_pub_      = create_publisher<sensor_msgs::msg::Image>("/camera/depth", qos);

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
        cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
        auto profile = pipe_.start(cfg);

        // Force constant 30 fps: stop the RGB sensor from lowering framerate to
        // lengthen exposure in dim light (this is the usual cause of "max 13 fps").
        try {
            auto color_sensor = profile.get_device().first<rs2::color_sensor>();
            if (color_sensor.supports(RS2_OPTION_AUTO_EXPOSURE_PRIORITY))
                color_sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_PRIORITY, 0.f);
        } catch (const rs2::error & e) {
            RCLCPP_WARN(get_logger(), "Could not disable auto-exposure priority: %s", e.what());
        }

        capture_thread_ = std::thread(&CameraNode::capture_loop, this);
        RCLCPP_INFO(get_logger(), "Camera node started");
    }

    ~CameraNode()
    {
        running_ = false;
        if (capture_thread_.joinable())
            capture_thread_.join();
        pipe_.stop();
    }

private:
    void capture_loop()
    {
        while (running_) {
            rs2::frameset frames;
            try {
                frames = pipe_.wait_for_frames(1000);
            } catch (const rs2::error &) {
                continue;
            }

            auto stamp = now();

            // --- color ---
            auto color_frame = frames.get_color_frame();
            cv::Mat color_mat(
                cv::Size(color_frame.get_width(), color_frame.get_height()),
                CV_8UC3,
                const_cast<void *>(color_frame.get_data()),
                cv::Mat::AUTO_STEP);
            // raw for compute_node (ML)
            auto color_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", color_mat).toImageMsg();
            color_msg->header.stamp = stamp;
            color_msg->header.frame_id = "camera_color_optical_frame";
            color_pub_->publish(*color_msg);

            // JPEG for comms_node (network) — encoded in C++, far cheaper than Python
            std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, jpeg_quality_};
            cv::imencode(".jpg", color_mat, jpeg_buf_, params);
            sensor_msgs::msg::CompressedImage jpeg_msg;
            jpeg_msg.header = color_msg->header;
            jpeg_msg.format = "jpeg";
            jpeg_msg.data   = jpeg_buf_;
            color_jpeg_pub_->publish(jpeg_msg);

            // --- depth ---
            auto depth_frame = frames.get_depth_frame();
            cv::Mat depth_mat(
                cv::Size(depth_frame.get_width(), depth_frame.get_height()),
                CV_16UC1,
                const_cast<void *>(depth_frame.get_data()),
                cv::Mat::AUTO_STEP);
            auto depth_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "mono16", depth_mat).toImageMsg();
            depth_msg->header.stamp = stamp;
            depth_msg->header.frame_id = "camera_depth_optical_frame";
            depth_pub_->publish(*depth_msg);
        }
    }

    rs2::pipeline pipe_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr           color_pub_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr color_jpeg_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr           depth_pub_;
    std::thread capture_thread_;
    std::atomic<bool> running_;
    std::vector<uchar> jpeg_buf_;
    int jpeg_quality_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CameraNode>());
    rclcpp::shutdown();
    return 0;
}
