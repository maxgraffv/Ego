#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int16_multi_array.hpp>
#include <alsa/asoundlib.h>
#include <atomic>
#include <thread>
#include <vector>

// ReSpeaker 4-mic: 6 channels (4 raw mics + 2 processed), 16 kHz, 16-bit
static constexpr unsigned int kSampleRate   = 16000;
static constexpr unsigned int kChannels     = 6;
static constexpr unsigned int kFramesPerBuf = 512;  // ~32 ms

class MicrophoneNode : public rclcpp::Node
{
private:
    snd_pcm_t *  pcm_{nullptr};
    std::atomic<bool> running_;
    std::thread  capture_thread_;
    rclcpp::Publisher<std_msgs::msg::Int16MultiArray>::SharedPtr audio_pub_;

public:
    MicrophoneNode() : Node("microphone_node"), running_(false)
    {
        declare_parameter<std::string>("device", "respeaker");
        std::string device = get_parameter("device").as_string();

        audio_pub_ = create_publisher<std_msgs::msg::Int16MultiArray>("/audio", 10);

        if (!open_device(device)) {
            RCLCPP_ERROR(get_logger(), "Failed to open ALSA device '%s'", device.c_str());
            return;
        }

        running_ = true;
        capture_thread_ = std::thread(&MicrophoneNode::capture_loop, this);
        RCLCPP_INFO(get_logger(), "Microphone node started — device: %s", device.c_str());
    }

    ~MicrophoneNode()
    {
        running_ = false;
        if (capture_thread_.joinable())
            capture_thread_.join();
        if (pcm_)
            snd_pcm_close(pcm_);
    }

private:
    bool open_device(const std::string & device)
    {
        int err;

        err = snd_pcm_open(&pcm_, device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
        if (err < 0) {
            RCLCPP_ERROR(get_logger(), "snd_pcm_open: %s", snd_strerror(err));
            return false;
        }

        snd_pcm_hw_params_t * hw_params;
        snd_pcm_hw_params_alloca(&hw_params);
        snd_pcm_hw_params_any(pcm_, hw_params);

        snd_pcm_hw_params_set_access(pcm_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_, hw_params, SND_PCM_FORMAT_S16_LE);

        unsigned int rate = kSampleRate;
        snd_pcm_hw_params_set_rate_near(pcm_, hw_params, &rate, nullptr);

        unsigned int channels = kChannels;
        snd_pcm_hw_params_set_channels(pcm_, hw_params, channels);

        err = snd_pcm_hw_params(pcm_, hw_params);
        if (err < 0) {
            RCLCPP_ERROR(get_logger(), "snd_pcm_hw_params: %s", snd_strerror(err));
            return false;
        }

        snd_pcm_prepare(pcm_);
        return true;
    }

    void capture_loop()
    {
        std::vector<int16_t> buf(kFramesPerBuf * kChannels);

        while (running_) {
            snd_pcm_sframes_t n = snd_pcm_readi(pcm_, buf.data(), kFramesPerBuf);

            if (n == -EPIPE) {
                RCLCPP_WARN(get_logger(), "ALSA overrun — recovering");
                snd_pcm_prepare(pcm_);
                continue;
            } else if (n < 0) {
                RCLCPP_ERROR(get_logger(), "snd_pcm_readi: %s", snd_strerror(n));
                break;
            }

            std_msgs::msg::Int16MultiArray msg;
            msg.layout.dim.resize(2);
            msg.layout.dim[0].label  = "frames";
            msg.layout.dim[0].size   = static_cast<uint32_t>(n);
            msg.layout.dim[0].stride = static_cast<uint32_t>(n) * kChannels;
            msg.layout.dim[1].label  = "channels";
            msg.layout.dim[1].size   = kChannels;
            msg.layout.dim[1].stride = kChannels;
            msg.data.assign(buf.begin(), buf.begin() + n * kChannels);

            audio_pub_->publish(msg);
        }
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MicrophoneNode>());
    rclcpp::shutdown();
    return 0;
}
