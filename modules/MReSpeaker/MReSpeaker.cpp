#include "MReSpeaker.h"


MReSpeaker::MReSpeaker(ITC::Bus& bus, std::string bus_name) : AModule(bus, bus_name)
{

}


void MReSpeaker::run()
{

}




void MReSpeaker::checkAlsa(int err, const char* what) 
{
    if (err < 0) {
        std::cerr << what << " failed: " << snd_strerror(err) << std::endl;
        std::exit(1);
    }
}


void MReSpeaker::simple()
{
    const std::string device = "hw:2,0"; // could be hw:1,0 or default

    // Typical ReSpeaker examples use 16 kHz.
    const unsigned int sampleRate = 16000;

    // If firmware exposes multiple channels
    // set this to 6 and process each channel separately.
    const unsigned int channels = 6;

    // 16-bit signed little-endian PCM
    const snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

    // Number of frames read per iteration
    const snd_pcm_uframes_t framesPerBuffer = 512;

    snd_pcm_t* handle = nullptr;
    snd_pcm_hw_params_t* hwParams = nullptr;

    int err = snd_pcm_open(&handle, device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    checkAlsa(err, "snd_pcm_open");

    snd_pcm_hw_params_alloca(&hwParams);
    checkAlsa(snd_pcm_hw_params_any(handle, hwParams), "snd_pcm_hw_params_any");
    checkAlsa(snd_pcm_hw_params_set_access(handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED),
              "snd_pcm_hw_params_set_access");
    checkAlsa(snd_pcm_hw_params_set_format(handle, hwParams, format),
              "snd_pcm_hw_params_set_format");
    checkAlsa(snd_pcm_hw_params_set_channels(handle, hwParams, channels),
              "snd_pcm_hw_params_set_channels");

    unsigned int actualRate = sampleRate;
    checkAlsa(snd_pcm_hw_params_set_rate_near(handle, hwParams, &actualRate, nullptr),
              "snd_pcm_hw_params_set_rate_near");

    snd_pcm_uframes_t actualFrames = framesPerBuffer;
    checkAlsa(snd_pcm_hw_params_set_period_size_near(handle, hwParams, &actualFrames, nullptr),
              "snd_pcm_hw_params_set_period_size_near");

    checkAlsa(snd_pcm_hw_params(handle, hwParams), "snd_pcm_hw_params");
    checkAlsa(snd_pcm_prepare(handle), "snd_pcm_prepare");

    std::cout << "Capture started\n";
    std::cout << "Device      : " << device << "\n";
    std::cout << "Sample rate : " << actualRate << " Hz\n";
    std::cout << "Channels    : " << channels << "\n";
    std::cout << "Frames/buf  : " << actualFrames << "\n\n";

    std::vector<int16_t> buffer(actualFrames * channels);

        int framesRead = snd_pcm_readi(handle, buffer.data(), actualFrames);

        if (framesRead == -EPIPE) {
            std::cerr << "Overrun occurred\n";
            snd_pcm_prepare(handle);
            // continue;
        } else if (framesRead < 0) {
            std::cerr << "Read error: " << snd_strerror(framesRead) << "\n";
            snd_pcm_prepare(handle);
            // continue;
        } else if (framesRead != static_cast<int>(actualFrames)) {
            std::cerr << "Short read: " << framesRead << " frames\n";
        }

        // Analyze channel 0 only
        double sumSquares = 0.0;
        int peak = 0;

        for (int i = 0; i < framesRead; ++i) {
            int16_t sample = buffer[i * channels]; // first channel
            int value = std::abs(static_cast<int>(sample));
            peak = std::max(peak, value);
            sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
        }

        double rms = 0.0;
        if (framesRead > 0) {
            rms = std::sqrt(sumSquares / framesRead);
        }

        // Normalize to 0..1 based on int16 max
        double rmsNorm = rms / 32768.0;
        double peakNorm = static_cast<double>(peak) / 32768.0;

        // Simple text meter
        int barLen = static_cast<int>(rmsNorm * 50.0);
        if (barLen > 50) barLen = 50;

        std::cout << "\rRMS: " << rms
                  << "  Peak: " << peak
                  << "  [";

        for (int i = 0; i < 50; ++i) {
            std::cout << (i < barLen ? '#' : ' ');
        }

        std::cout << "] "
                  << static_cast<int>(peakNorm * 100.0) << "%   "
                  << std::flush;

    snd_pcm_close(handle);

}