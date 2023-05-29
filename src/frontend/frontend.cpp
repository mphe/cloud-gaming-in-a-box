#include <iostream>
#include <thread>
#include <SDL2/SDL.h>
#include "av.hpp"
#include "ui.hpp"
#include <mutex>

#define USE_VIDEO_THREADING
#define USE_AUDIO

using std::endl;
using std::cout;
using std::cerr;

std::mutex vMutex;

void processV(frontend::AVStream& stream, AVFrame* avFrame, bool* running) {
    auto video = stream.video();

    while (*running) {
        if (!stream.readPacket())
            break;

        {
            std::lock_guard<std::mutex> guard(vMutex);

            if (!stream.retrieveFrame(video, avFrame))
                break;
        }
    }
    cout << "Video EOF\n";
}

#ifdef USE_AUDIO
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
    wanted.samples = 128;  // Good low-latency value
    wanted.callback = nullptr;
    wanted.userdata = nullptr;
    return SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
}
#endif


void help() {
    cout << "Usage: frontend <video filename/URL> <audio filename/URL> <syncinput IP> <syncinput port>\n";
    cout << "Live-streams the given video and audio streams while transmitting inputs to the given syncinput server.";
}


int main(int argc, char *argv[]) {
    if (argc < 5) {
        help();
        cerr << "Missing arguments\n";
        return 1;
    }

    const char* videoURL = argv[1];
    const char* audioURL = argv[2];
    const char* syncinputIP = argv[3];
    const char* syncinputPort = argv[4];

    input::InputTransmitter inputTransmitter;
    if (!inputTransmitter.connect(syncinputIP, syncinputPort, net::TCP)) {
        cerr << "Failed to establish connection\n";
        return 1;
    }

    frontend::AVStream videoStream;
    if (!videoStream.open(videoURL))
        return 1;

    frontend::UI ui;
    auto video = videoStream.video();

    // Initialize SDL before opening audio device.
    if (!ui.init(video->width, video->height, false))
        return 1;

#ifdef USE_AUDIO
    frontend::AVStream audioStream;
    audioStream.format()->probesize = 0;  // low latency audio

    if (!audioStream.open(audioURL))
        return 1;

    auto audio = audioStream.audio();
    auto audioDev = openAudio(audio);
    if (audioDev <= 0) {
        cerr << "Couldn't open audio: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_PauseAudioDevice(audioDev, 0);
#endif

    auto texFrame = ui.getFrameTexture();
    AVFrame *videoFrame = av_frame_alloc();
    bool running = true;

#ifdef USE_AUDIO
    AVFrame *audioFrame = av_frame_alloc();
    std::thread audioThread(processA, std::ref(audioStream), audioFrame, audioDev, &running);
#endif

#ifdef USE_VIDEO_THREADING
    std::thread videoThread(processV, std::ref(videoStream), videoFrame, &running);
#endif

    while (running) {
        running = ui.handleInput(inputTransmitter);

#ifndef USE_VIDEO_THREADING
        if (!videoStream.readPacket())
            running = false;

        if (!videoStream.retrieveFrame(video, videoFrame))
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

        ui.renderFrame();
        ui.present();
    }

#ifdef USE_AUDIO
    audioThread.join();
    av_frame_free(&audioFrame);
    SDL_CloseAudioDevice(audioDev);
#endif

#ifdef USE_VIDEO_THREADING
    videoThread.join();
#endif
    av_frame_free(&videoFrame);


    return 0;
}
