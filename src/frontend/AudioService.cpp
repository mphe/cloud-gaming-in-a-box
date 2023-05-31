#include "AudioService.hpp"
#include <iostream>

using std::endl;
using std::cout;
using std::cerr;

namespace frontend {
    // Maximum number of bytes the audio queue is allowed to have before being cleared.
    constexpr int max_queued_audio_bytes = 2048;


    AudioService::AudioService() : _audioDev(0), _running(false) {}

    bool AudioService::open(const char* url) {
        _stream.format()->probesize = 16;  // low latency audio
        _stream.format()->max_analyze_duration = 0;

        if (!_stream.open(url))
            return false;

        auto audio = _stream.audio();

        if (audio->sample_fmt != AV_SAMPLE_FMT_FLTP) {
            cerr << "Only " << av_get_sample_fmt_name(AV_SAMPLE_FMT_FLTP) << " is supported for now.\n";
            return false;
        }

        SDL_AudioSpec wanted;
        wanted.freq = audio->sample_rate;
        wanted.format = AUDIO_F32;
        wanted.channels = audio->ch_layout.nb_channels;
        wanted.samples = 128;  // Good low-latency value
        wanted.callback = nullptr;
        wanted.userdata = nullptr;

        _audioDev = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);

        if (_audioDev <= 0) {
            cerr << "Failed to open audio device: " << SDL_GetError() << endl;
            return false;
        }

        SDL_PauseAudioDevice(_audioDev, 0);
        return true;
    }

    void AudioService::start() {
        _running = true;
        _thread = std::thread(_process, this);
    }

    void AudioService::join() {
        _running = false;
        _thread.join();
        SDL_CloseAudioDevice(_audioDev);
    }

    void AudioService::_process(AudioService* self) {
        AVStream& stream = self->_stream;
        auto audio = stream.audio();
        auto sampleBufSize = av_get_bytes_per_sample(audio->sample_fmt);
        auto dev = self->_audioDev;
        auto frame = self->_frame.get();

        if (sampleBufSize < 0) {
            cerr << "Failed to calculate audio buffer size\n";
            return;
        }

        // Audio decode example:
        // https://github.com/FFmpeg/FFmpeg/blob/82278e874989b36f8f7f6b56651c12872bb40771/doc/examples/decode_audio.c#L72

        SDL_ClearQueuedAudio(dev);
        avcodec_flush_buffers(audio);

        while (self->_running) {
            if (!stream.readPacket())
                break;

            // At start, the buffer fills up, causing about 0.5s latency, but I don't know when or why
            // this happens exactly. To prevent this, simply clear the audio queue when it becomes too
            // full. This might not be the nicest way to fix this delay, but it works.
            if (SDL_GetQueuedAudioSize(dev) > max_queued_audio_bytes) [[unlikely]] {
                SDL_ClearQueuedAudio(dev);
                avcodec_flush_buffers(audio);
                cout << "Flushing audio queue to reduce latency\n";
            }

            if (!stream.retrieveFrame(audio, frame))
                break;

            for (int i = 0; i < frame->nb_samples; ++i)
                for (int ch = 0; ch < audio->ch_layout.nb_channels; ++ch)
                    SDL_QueueAudio(dev, frame->data[ch] + i * sampleBufSize, sampleBufSize);

        }
    }
}
