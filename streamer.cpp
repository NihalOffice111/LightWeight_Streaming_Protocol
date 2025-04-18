#include <iostream>
#include <fstream> // Include for std::ofstream
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <linux/videodev2.h>
#include <alsa/asoundlib.h>
#include <turbojpeg.h>
#include <opencv2/opencv.hpp>
#include <opus/opus.h> // Opus encoder

#define VIDEO_PORT 5000
#define AUDIO_PORT 5002
#define AUDIO_DEVICE "default"
#define SAMPLE_RATE 48000
#define CHANNELS 1
#define AUDIO_BUFFER_SIZE 4096
#define RTP_HEADER_SIZE 12
#define IP_ADDRESS "127.0.0.1"

// RTP header
// struct RTPHeader
// {
//     uint8_t version : 2;
//     uint8_t padding : 1;
//     uint8_t extension : 1;
//     uint8_t csrcCount : 4;
//     uint8_t marker : 1;
//     uint8_t payloadType : 7;
//     uint16_t sequenceNumber;
//     uint32_t timestamp;
//     uint32_t ssrc;
// };

#pragma pack(push, 1)
struct RTPHeader {
    uint8_t version_p_x_cc;   // V=2, P=0, X=0, CC=0 → 0x80
    uint8_t m_payloadtype;    // M=0, PT=96 or 97
    uint16_t sequenceNumber;  // htons
    uint32_t timestamp;       // htonl
    uint32_t ssrc;            // htonl
};
#pragma pack(pop)



// void createRTPHeader(RTPHeader &header, uint32_t ts, uint16_t seqNum, uint8_t payloadType)
// {
//     header.version = 2;
//     header.padding = 0;
//     header.extension = 0;
//     header.csrcCount = 0;
//     header.marker = 0;
//     header.payloadType = payloadType;
//     header.sequenceNumber = htons(seqNum);
//     header.timestamp = htonl(ts);
//     header.ssrc = htonl(1234);
// }

void createRTPHeader(RTPHeader &header, uint32_t timestamp, uint16_t sequence, uint8_t payloadType)
{
    header.version_p_x_cc = 0x80;                  // V=2
    header.m_payloadtype = payloadType & 0x7F;     // PT = 96 or 97
    header.sequenceNumber = htons(sequence);
    header.timestamp = htonl(timestamp);
    header.ssrc = htonl(123456);                   // Arbitrary SSRC
}




struct Buffer
{
    void *start;
    size_t length;
};

// Utility: Clamp to [0, 255]
uint8_t clamp(int val)
{
    return val < 0 ? 0 : (val > 255 ? 255 : val);
}

void videoStreaming(const std::string &ip)
{
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0)
    {
        perror("Video device open failed");
        return;
    }
    std::cout << "[Video] Opened video device /dev/video0\n";

    // Configure V4L2
    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 320;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // YUYV = YUV422
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    ioctl(fd, VIDIOC_S_FMT, &fmt);

    v4l2_requestbuffers req{};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ioctl(fd, VIDIOC_REQBUFS, &req);

    std::vector<Buffer> buffers(req.count);
    for (int i = 0; i < req.count; ++i)
    {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ioctl(fd, VIDIOC_QUERYBUF, &buf);
        buffers[i].length = buf.length;
        buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        ioctl(fd, VIDIOC_QBUF, &buf);
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in client{};
    client.sin_family = AF_INET;
    client.sin_port = htons(VIDEO_PORT);
    client.sin_addr.s_addr = inet_addr(ip.c_str());

    uint16_t seq = 0;
    uint32_t ts = 32;
    tjhandle tjInstance = tjInitCompress();

    const int width = fmt.fmt.pix.width;
    const int height = fmt.fmt.pix.height;

    // Planar buffer sizes
    const int Y_size = width * height;
    const int UV_size = (width / 2) * height;

    std::vector<uint8_t> Y_plane(Y_size);
    std::vector<uint8_t> U_plane(UV_size);
    std::vector<uint8_t> V_plane(UV_size);

    // Plane pointers
    const unsigned char *planes[3] = {
        Y_plane.data(),
        U_plane.data(),
        V_plane.data()};

    // Stride for each row (no padding)
    int strides[3] = {
        width,     // Y stride
        width / 2, // U stride
        width / 2  // V stride
    };

    while (true)
    {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ioctl(fd, VIDIOC_DQBUF, &buf);

        uint8_t *yuyv = static_cast<uint8_t *>(buffers[buf.index].start);

        // Convert packed YUYV → YUV422 Planar
        for (int i = 0, j = 0; i < width * height * 2; i += 4, j += 2)
        {
            uint8_t y0 = yuyv[i];
            uint8_t u = yuyv[i + 1];
            uint8_t y1 = yuyv[i + 2];
            uint8_t v = yuyv[i + 3];

            Y_plane[j] = y0;
            Y_plane[j + 1] = y1;
            U_plane[j / 2] = u;
            V_plane[j / 2] = v;
        }

        unsigned char *jpegBuf = nullptr;
        unsigned long jpegSize = 0;
        int jpegQuality = 80;

        // Compress using tjCompressFromYUVPlanes
        if (tjCompressFromYUVPlanes(tjInstance, planes, width, strides, height,
                                    TJSAMP_422, &jpegBuf, &jpegSize, jpegQuality, TJFLAG_FASTDCT) < 0)
        {
            std::cerr << "TurboJPEG compression failed: " << tjGetErrorStr() << "\n";
            ioctl(fd, VIDIOC_QBUF, &buf);
            continue;
        }

        std::vector<uint8_t> packet(RTP_HEADER_SIZE + jpegSize);
        RTPHeader hdr;
        createRTPHeader(hdr, ts, seq++, 96);
        //memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
        memcpy(packet.data(), reinterpret_cast<void*>(&hdr), sizeof(RTPHeader));

        memcpy(packet.data() + RTP_HEADER_SIZE, jpegBuf, jpegSize);

        sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&client, sizeof(client));
        std::cout << "[Video] Sent frame: seq=" << seq - 1 << ", timestamp=" << ts
                  << ", JPEG size=" << jpegSize << " bytes, total packet=" << packet.size() << " bytes\n";

        tjFree(jpegBuf);
        ioctl(fd, VIDIOC_QBUF, &buf);
        usleep(1000000 / 15);
        ts += 1000 / 15;
    }

    tjDestroy(tjInstance);
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    close(fd);
    close(sock);
}

// AUDIO THREAD — Using Opus
void audioStreaming(const std::string &ip)
{
    int err;
    OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_AUDIO, &err);
    if (err != OPUS_OK)
    {
        std::cerr << "Opus encoder error: " << opus_strerror(err) << "\n";
        return;
    }
    std::cout << "[Audio] Created Opus encoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";
    snd_pcm_t *handle;
    snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
    snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                       CHANNELS, SAMPLE_RATE, 1, 500000);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in client{};
    client.sin_family = AF_INET;
    client.sin_port = htons(AUDIO_PORT);
    client.sin_addr.s_addr = inet_addr(ip.c_str());

    const int frame_size = 960;                         // 20ms at 48kHz
    std::vector<int16_t> buffer(frame_size * CHANNELS); // 16-bit samples
    unsigned char opusData[4000];
    uint16_t seq = 0;
    uint32_t ts = 32;

    while (true)
    {
        // Read exactly frame_size frames (frame_size * CHANNELS * sizeof(int16_t) bytes)
        snd_pcm_readi(handle, buffer.data(), frame_size);

        int encodedBytes = opus_encode(encoder, buffer.data(), frame_size, opusData, sizeof(opusData));
        if (encodedBytes < 0)
        {
            std::cerr << "Opus encode error: " << opus_strerror(encodedBytes) << "\n";
            continue;
        }

        std::vector<uint8_t> packet(RTP_HEADER_SIZE + encodedBytes);
        RTPHeader hdr;
        createRTPHeader(hdr, ts, seq++, 97);
        memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
        memcpy(packet.data() + RTP_HEADER_SIZE, opusData, encodedBytes);
        sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&client, sizeof(client));
        std::cout << "[Audio] Sent frame: seq=" << seq - 1 << ", timestamp=" << ts
                  << ", Opus size=" << encodedBytes << " bytes, total packet=" << packet.size() << " bytes\n";
        ts += frame_size; // Increment timestamp by number of samples
    }

    close(sock);
    snd_pcm_close(handle);
    opus_encoder_destroy(encoder);
}

int main()
{



    std::string ip = IP_ADDRESS;
    std::thread videoThread(videoStreaming, ip);
    std::thread audioThread(audioStreaming, ip);
    videoThread.join();
    audioThread.join();
    return 0;
}






// FOR X86
//  g++ streamer.cpp -o streamer -lturbojpeg `pkg-config --cflags --libs opencv4` -lasound -lopus -lpthread

// FOR ARM
//