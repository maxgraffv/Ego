#include "MAudioPlayer.h"

#include <iostream>


/**************************************************
 *   CONSTRUCTORS
 **************************************************/

MAudioPlayer::MAudioPlayer(ITC::Bus& bus, std::string bus_name, std::string device)
    : AModule(bus, bus_name), _device(std::move(device))
{
    _sub = bus.subscribe<std::string>(bus_name, [this](const std::string& path) {
        play(path);
    });
}

MAudioPlayer::~MAudioPlayer()
{
    _sub.unsubscribe();
}


/**************************************************
 *   THREAD
 **************************************************/

void MAudioPlayer::run()
{
    std::string path;

    {
        std::lock_guard<std::mutex> lock(_queue_mtx);
        if (_queue.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }
        path = _queue.front();
        _queue.pop();
    }

    auto dot = path.rfind('.');
    if (dot == std::string::npos)
    {
        std::cerr << "MAudioPlayer: unknown file format: " << path << "\n";
        return;
    }

    std::string ext = path.substr(dot);
    for (auto& c : ext) c = static_cast<char>(std::tolower(c));

    if (ext == ".mp3")
        playMp3(path);
    else
        playWav(path);
}


/**************************************************
 *   CONTROL
 **************************************************/

void MAudioPlayer::play(const std::string& path)
{
    std::lock_guard<std::mutex> lock(_queue_mtx);
    _queue.push(path);
}

void MAudioPlayer::hello()
{
    play(std::string(DEFAULT_AUDIO_FILES_PATH) + "Hej_jestem_Q_robotic.wav");
}


/**************************************************
 *   WAV PLAYBACK  (libsndfile + ALSA)
 **************************************************/

void MAudioPlayer::playWav(const std::string& path)
{
    SF_INFO info{};
    SNDFILE* sf = sf_open(path.c_str(), SFM_READ, &info);
    if (!sf)
    {
        std::cerr << "MAudioPlayer: cannot open " << path << ": " << sf_strerror(nullptr) << "\n";
        return;
    }

    snd_pcm_t* pcm = nullptr;
    int err = snd_pcm_open(&pcm, _device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    checkAlsa(err, "snd_pcm_open");

    snd_pcm_hw_params_t* hw = nullptr;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(pcm, hw);
    snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hw, static_cast<unsigned int>(info.channels));

    unsigned int rate = static_cast<unsigned int>(info.samplerate);
    snd_pcm_hw_params_set_rate_near(pcm, hw, &rate, nullptr);
    snd_pcm_hw_params(pcm, hw);
    snd_pcm_prepare(pcm);

    const int framesPerBuf = 1024;
    std::vector<int16_t> buf(framesPerBuf * info.channels);

    sf_count_t read;
    while ((read = sf_readf_short(sf, buf.data(), framesPerBuf)) > 0)
    {
        snd_pcm_sframes_t written = snd_pcm_writei(pcm, buf.data(), static_cast<snd_pcm_uframes_t>(read));
        if (written == -EPIPE)
        {
            snd_pcm_prepare(pcm);
            snd_pcm_writei(pcm, buf.data(), static_cast<snd_pcm_uframes_t>(read));
        }
        else if (written < 0)
        {
            std::cerr << "MAudioPlayer: write error: " << snd_strerror(static_cast<int>(written)) << "\n";
            break;
        }
    }

    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    sf_close(sf);
}


/**************************************************
 *   MP3 PLAYBACK  (libmpg123 + ALSA)
 **************************************************/

void MAudioPlayer::playMp3(const std::string& path)
{
    mpg123_init();
    int mErr;
    mpg123_handle* mh = mpg123_new(nullptr, &mErr);
    if (!mh)
    {
        std::cerr << "MAudioPlayer: mpg123_new failed: " << mpg123_plain_strerror(mErr) << "\n";
        mpg123_exit();
        return;
    }

    if (mpg123_open(mh, path.c_str()) != MPG123_OK)
    {
        std::cerr << "MAudioPlayer: cannot open " << path << ": " << mpg123_strerror(mh) << "\n";
        mpg123_delete(mh);
        mpg123_exit();
        return;
    }

    long rate;
    int channels, encoding;
    mpg123_getformat(mh, &rate, &channels, &encoding);

    snd_pcm_t* pcm = nullptr;
    int err = snd_pcm_open(&pcm, _device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    checkAlsa(err, "snd_pcm_open");

    snd_pcm_hw_params_t* hw = nullptr;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(pcm, hw);
    snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hw, static_cast<unsigned int>(channels));
    unsigned int urate = static_cast<unsigned int>(rate);
    snd_pcm_hw_params_set_rate_near(pcm, hw, &urate, nullptr);
    snd_pcm_hw_params(pcm, hw);
    snd_pcm_prepare(pcm);

    std::size_t bufSize = mpg123_outblock(mh);
    std::vector<unsigned char> buf(bufSize);
    std::size_t done;

    while (mpg123_read(mh, buf.data(), bufSize, &done) == MPG123_OK)
    {
        snd_pcm_uframes_t frames = done / static_cast<std::size_t>(channels * 2);
        snd_pcm_sframes_t written = snd_pcm_writei(pcm, buf.data(), frames);
        if (written == -EPIPE)
        {
            snd_pcm_prepare(pcm);
            snd_pcm_writei(pcm, buf.data(), frames);
        }
        else if (written < 0)
        {
            std::cerr << "MAudioPlayer: write error: " << snd_strerror(static_cast<int>(written)) << "\n";
            break;
        }
    }

    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
}


/**************************************************
 *   ALSA HELPER
 **************************************************/

void MAudioPlayer::checkAlsa(int err, const char* what)
{
    if (err < 0)
    {
        std::cerr << "MAudioPlayer: " << what << " failed: " << snd_strerror(err) << "\n";
        std::exit(1);
    }
}
