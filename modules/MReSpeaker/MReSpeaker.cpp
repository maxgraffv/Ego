#include "MReSpeaker.h"
#include <ctime>
#include <iostream>


/**************************************************
 *   CONSTRUCTORS
 **************************************************/

MReSpeaker::MReSpeaker(ITC::Bus& bus, std::string bus_name, std::string device)
    : AModule(bus, bus_name), _device(std::move(device))
{
}

MReSpeaker::~MReSpeaker()
{
    closeAlsa();
}


/**************************************************
 *   THREAD
 **************************************************/

void MReSpeaker::run()
{
    if (!_pcm && !openAlsa())
    {
        return;
    }

    std::vector<int16_t> interleaved(_num_channels * kFrameSamples);

    snd_pcm_sframes_t read = snd_pcm_readi(_pcm, interleaved.data(), kFrameSamples);

    if (read == -EPIPE)
    {
        std::cerr << "MReSpeaker: overrun, recovering\n";
        snd_pcm_prepare(_pcm);
        return;
    }
    if (read < 0)
    {
        std::cerr << "MReSpeaker: read error: " << snd_strerror(static_cast<int>(read)) << "\n";
        closeAlsa();
        return;
    }

    AudioFrame frame;
    frame.num_channels = static_cast<int>(_num_channels);
    frame.num_samples  = static_cast<int>(read);
    frame.sample_rate  = static_cast<int>(_sample_rate);
    frame.ts           = std::time(nullptr);
    frame.channels.resize(_num_channels);

    for (unsigned int ch = 0; ch < _num_channels; ++ch)
    {
        frame.channels[ch].resize(static_cast<std::size_t>(read));
        for (snd_pcm_sframes_t s = 0; s < read; ++s)
        {
            frame.channels[ch][static_cast<std::size_t>(s)] = interleaved[static_cast<std::size_t>(s) * _num_channels + ch];
        }
    }

    bus().publish<AudioFrame>(busName(), frame);
}


/**************************************************
 *   ALSA
 **************************************************/

bool MReSpeaker::openAlsa()
{
    int err = snd_pcm_open(&_pcm, _device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0)
    {
        std::cerr << "MReSpeaker: cannot open device " << _device << ": " << snd_strerror(err) << "\n";
        _pcm = nullptr;
        return false;
    }

    snd_pcm_hw_params_t* hw = nullptr;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(_pcm, hw);
    snd_pcm_hw_params_set_access(_pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(_pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(_pcm, hw, _num_channels);
    snd_pcm_hw_params_set_rate_near(_pcm, hw, &_sample_rate, nullptr);

    snd_pcm_uframes_t period = kFrameSamples;
    snd_pcm_hw_params_set_period_size_near(_pcm, hw, &period, nullptr);

    checkAlsa(snd_pcm_hw_params(_pcm, hw), "snd_pcm_hw_params");
    checkAlsa(snd_pcm_prepare(_pcm),       "snd_pcm_prepare");

    std::cout << "MReSpeaker: opened " << _device
              << " @ " << _sample_rate << " Hz, "
              << _num_channels << " ch, "
              << kFrameSamples << " samples/frame (20ms)\n";
    return true;
}

void MReSpeaker::closeAlsa()
{
    if (_pcm)
    {
        snd_pcm_close(_pcm);
        _pcm = nullptr;
    }
}

void MReSpeaker::checkAlsa(int err, const char* what)
{
    if (err < 0)
    {
        std::cerr << "MReSpeaker: " << what << " failed: " << snd_strerror(err) << "\n";
        std::exit(1);
    }
}
