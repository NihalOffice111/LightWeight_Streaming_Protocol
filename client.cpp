// Video and audio Receiving client
#include <iostream>
#include <opencv2/opencv.hpp>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <alsa/asoundlib.h>
#include <opus/opus.h>  // Opus decoder

#define VIDEO_PORT 5000
#define AUDIO_PORT 5002
#define BUFFER_SIZE 65536
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_DEVICE "default"
#define SAMPLE_RATE 48000
#define CHANNELS 1

// VIDEO RECEIVER — Same as before
void videoReceiver() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(VIDEO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (sockaddr*)&addr, sizeof(addr));

    std::vector<uint8_t> buffer(BUFFER_SIZE);

    while (true) {
        ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (len < 12) continue;

        std::cout<<"Video Decoding\n";
        // Skip RTP header
        std::vector<uchar> jpeg(buffer.begin() + 12, buffer.begin() + len);
        cv::Mat frame = cv::imdecode(jpeg, cv::IMREAD_COLOR);
        if (frame.empty()) {
            std::cerr << "Failed to decode JPEG frame\n";
        } else {
            cv::imshow("Video", frame);
            if (cv::waitKey(1) == 27) break;  // ESC to exit
        }
    }

    close(sock);
}

// AUDIO RECEIVER — with Opus decoding
void audioReceiver() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(AUDIO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (sockaddr*)&addr, sizeof(addr));

    snd_pcm_t *handle;
    snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                       CHANNELS, SAMPLE_RATE, 1, 500000);

    int err;
    OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
    if (err != OPUS_OK) {
        std::cerr << "Failed to create Opus decoder: " << opus_strerror(err) << "\n";
        return;
    }
    std::cout << "Creating Opus decoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    std::vector<opus_int16> decodedPcm(2048 * CHANNELS); // Enough for 20ms at 48kHz

    while (true) {
        ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (len < 12) continue;

        std::cout<<"Audio Decoding\n";
        unsigned char* opusPayload = buffer.data() + 128;
        int frameSize = opus_decode(decoder, opusPayload, len - 12, decodedPcm.data(), 2048, 0);
        if (frameSize < 0) {
            std::cerr << "Opus decode error: " << opus_strerror(frameSize) << "\n";
            continue;
        }

        snd_pcm_writei(handle, decodedPcm.data(), frameSize);
    }

    close(sock);
    snd_pcm_close(handle);
    opus_decoder_destroy(decoder);
}

// Main: run both threads
int main() {
    std::thread videoThread(videoReceiver);
    std::thread audioThread(audioReceiver);
    videoThread.join();
    audioThread.join();
    return 0;
}



//g++ client.cpp -o client `pkg-config --cflags --libs opencv4` -lopus -lasound -lpthread

