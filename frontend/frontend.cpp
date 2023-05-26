#include <iostream>
#include <thread>
#include <SDL2/SDL.h>
#include "av.hpp"
#include "ui.hpp"
#include <mutex>

#define USE_VIDEO_THREADING

using std::endl;
using std::cout;
using std::cerr;

std::mutex vMutex;

void processV(frontend::AVStream& stream, AVFrame* avFrame, SDL_Texture* texFrame, bool* running) {
    auto video = stream.video();

    while (*running) {
        if (!stream.readPacket())
            break;

        {
            std::lock_guard<std::mutex> guard(vMutex);
            // av_frame_unref(avFrame);

            if (!stream.retrieveFrame(video, avFrame))
                break;
        }
    }
    // av_frame_unref(avFrame);
    cout << "Video EOF\n";
}

void processA(frontend::AVStream& stream, AVFrame* avFrame, SDL_AudioDeviceID dev, bool* running) {
    auto audio = stream.audio();
    auto sampleBufSize = av_get_bytes_per_sample(audio->sample_fmt);

    if (sampleBufSize < 0) {
        cerr << "Failed to calculate audio buffer size\n";
    }

    // Audio decode example:
    // https://github.com/FFmpeg/FFmpeg/blob/82278e874989b36f8f7f6b56651c12872bb40771/doc/examples/decode_audio.c#L72

    while (*running) {
        if (!stream.readPacket())
            break;

        if (!stream.retrieveFrame(audio, avFrame))
            break;

        for (int i = 0; i < avFrame->nb_samples; ++i)
            for (int ch = 0; ch < audio->ch_layout.nb_channels; ++ch)
                SDL_QueueAudio(dev, avFrame->data[ch] + i * sampleBufSize, sampleBufSize);
    }

    cout << "Audio EOF\n";
}


SDL_AudioDeviceID openAudio(const AVCodecContext* audio) {
    if (audio->sample_fmt != AV_SAMPLE_FMT_FLTP) {
        cerr << "Only " << av_get_sample_fmt_name(AV_SAMPLE_FMT_FLTP) << " is supported for now.\n";
        return -1;
    }

    SDL_AudioSpec wanted;
    wanted.freq = audio->sample_rate;
    wanted.format = AUDIO_F32;
    wanted.channels = audio->ch_layout.nb_channels;
    wanted.samples = 1024;  // Good low-latency value for callback
    wanted.callback = nullptr;
    wanted.userdata = nullptr;
    return SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Missing video filename/URL\n";
        return 1;
    }

    if (argc < 3) {
        cerr << "Missing audio filename/URL\n";
        return 1;
    }

    frontend::AVStream videoStream;
    if (!videoStream.open(argv[1]))
        return 1;

    frontend::AVStream audioStream;
    if (!audioStream.open(argv[2]))
        return 1;

    auto video = videoStream.video();
    auto audio = audioStream.audio();

    frontend::UI ui;
    if (!ui.init(video->width, video->height, false))
        return 1;

    auto audioDev = openAudio(audio);
    if (audioDev <= 0) {
        cerr << "Couldn't open audio: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_PauseAudioDevice(audioDev, 0);

    auto texFrame = ui.getFrameTexture();
    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *audioFrame = av_frame_alloc();
    bool running = true;
    std::thread audioThread(processA, std::ref(audioStream), audioFrame, audioDev, &running);

#ifdef USE_VIDEO_THREADING
    std::thread videoThread(processV, std::ref(videoStream), videoFrame, texFrame, &running);
#endif

    while (running) {
        running = ui.handleInput();

#ifndef USE_VIDEO_THREADING
        if (!stream.readPacket())
            running = false;

        if (!stream.retrieveFrame(video, videoFrame))
            running = false;
#endif

        {
#ifdef USE_VIDEO_THREADING
            std::lock_guard<std::mutex> guard(vMutex);
#endif
            SDL_UpdateYUVTexture(texFrame, nullptr,
                    videoFrame->data[0], videoFrame->linesize[0],
                    videoFrame->data[1], videoFrame->linesize[1],
                    videoFrame->data[2], videoFrame->linesize[2]);
        }

        {
            // std::lock_guard<std::mutex> guard(avMutex);
            ui.renderFrame();
            ui.present();
        }
    }

    audioThread.join();
    av_frame_free(&audioFrame);
    SDL_CloseAudioDevice(audioDev);

#ifdef USE_VIDEO_THREADING
    videoThread.join();
#endif
    av_frame_free(&videoFrame);


    return 0;
}
