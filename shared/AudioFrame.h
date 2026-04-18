#ifndef AUDIO_FRAME_H
#define AUDIO_FRAME_H

#include <cstdint>
#include <ctime>
#include <vector>

class AudioFrame {
    public:
        std::vector<std::vector<int16_t>> channels; // [channel_idx][sample]
        int num_channels;  // 6 for ReSpeaker 4-mic
        int num_samples;   // 320 (20ms @ 16kHz)
        int sample_rate;   // 16000
        time_t ts;
};

#endif
