// #include <iostream>
// #include <opencv2/opencv.hpp>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <sys/time.h>           // <-- FIX 1: Needed for gettimeofday
// #include <vector>
// #include <cstring>              // <-- For memcpy

// #define PORT 5000
// #define HEADER_MAGIC 0xDEADBEEF

// #pragma pack(push, 1)
// struct FrameHeader {
//     uint32_t magic;
//     uint32_t frame_id;
//     uint64_t timestamp;
//     uint32_t frame_size;
// };
// #pragma pack(pop)

// uint64_t get_timestamp_us() {
//     timeval tv{};
//     gettimeofday(&tv, nullptr);   // <-- FIXED: After including <sys/time.h>
//     return tv.tv_sec * 1000000ULL + tv.tv_usec;
// }

// int main() {
//     cv::VideoCapture cap("/dev/video0");
//     if (!cap.isOpened()) {
//         std::cerr << "Failed to open camera\n";
//         return 1;
//     }

//     cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
//     cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in client_addr{};
//     client_addr.sin_family = AF_INET;
//     client_addr.sin_port = htons(PORT);
//     client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // <-- ⚠️ Replace with your client IP

//     uint32_t frame_id = 0;
//     std::vector<uchar> jpeg_buf;

//     while (true) {
//         cv::Mat frame;
//         cap >> frame;
//         if (frame.empty()) continue;

//         std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
//         cv::imencode(".jpg", frame, jpeg_buf, params);

//         FrameHeader header;
//         header.magic = htonl(HEADER_MAGIC);
//         header.frame_id = htonl(frame_id++);
//         header.timestamp = htobe64(get_timestamp_us());
//         header.frame_size = htonl(jpeg_buf.size());

//         size_t total_size = sizeof(FrameHeader) + jpeg_buf.size();
//         if (total_size > 65507) continue; // UDP max size

//         std::vector<uchar> packet(total_size);
//         memcpy(packet.data(), &header, sizeof(FrameHeader));
//         memcpy(packet.data() + sizeof(FrameHeader), jpeg_buf.data(), jpeg_buf.size());

//         // FIX 2: send packet.data(), not the vector itself
//         ssize_t sent_bytes = sendto(sock, packet.data(), total_size, 0,
//                                     (sockaddr*)&client_addr, sizeof(client_addr));

//         if (sent_bytes < 0) {
//             perror("sendto");
//         } else {
//             std::cout << "Frame sent successfully (" << sent_bytes << " bytes)\n";
//         }

//         usleep(1000);  // Throttle a bit
//     }

//     close(sock);
//     return 0;
// }

// ITS NOT using the RTP header, but rather a custom header.

// #include <iostream>
// #include <opencv2/opencv.hpp>
// #include <cstring>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <sys/time.h>
// #include <vector>

// // Define the RTP packet header
// struct RTPHeader {
//     uint8_t version:2;
//     uint8_t padding:1;
//     uint8_t extension:1;
//     uint8_t csrcCount:4;
//     uint8_t marker:1;
//     uint8_t payloadType:7;
//     uint16_t sequenceNumber;
//     uint32_t timestamp;
//     uint32_t ssrc;
// };

// // Constants
// #define PORT 5000
// #define RTP_PAYLOAD_TYPE 96  // For MJPEG
// #define FRAME_RATE 30        // Frames per second

// // Helper to get the current timestamp in ms
// uint32_t getTimestamp() {
//     struct timeval tv;
//     gettimeofday(&tv, nullptr);
//     return (tv.tv_sec * 1000) + (tv.tv_usec / 1000); // timestamp in ms
// }

// // Function to create RTP header
// void createRTPHeader(RTPHeader& header, uint32_t timestamp, uint16_t seqNum) {
//     header.version = 2;            // RTP version
//     header.padding = 0;            // No padding
//     header.extension = 0;          // No extension
//     header.csrcCount = 0;          // No CSRC
//     header.marker = 0;             // No marker
//     header.payloadType = RTP_PAYLOAD_TYPE;
//     header.sequenceNumber = seqNum;
//     header.timestamp = timestamp;
//     header.ssrc = 12345;           // Arbitrary SSRC
// }

// int main() {
//     cv::VideoCapture cap("/dev/video0");
//     if (!cap.isOpened()) {
//         std::cerr << "Failed to open camera\n";
//         return 1;
//     }

//     cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
//     cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

//     // Prepare UDP socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         return 1;
//     }

//     sockaddr_in clientAddr{};
//     clientAddr.sin_family = AF_INET;
//     clientAddr.sin_port = htons(PORT);
//     clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change if using a different machine

//     uint16_t seqNum = 0;
//     uint32_t timestamp = 0;
//     std::vector<uchar> jpegBuffer;
//     std::vector<uint8_t> rtpPacket;

//     while (true) {
//         cv::Mat frame;
//         cap >> frame;
//         if (frame.empty()) continue;

//         // Compress frame to JPEG
//         std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
//         cv::imencode(".jpg", frame, jpegBuffer, params);

//         // Prepare RTP header and packet
//         RTPHeader rtpHeader;
//         createRTPHeader(rtpHeader, timestamp, seqNum++);
//         timestamp += 1000 / FRAME_RATE; // Simulate 30 fps

//         // Copy RTP header into packet buffer
//         rtpPacket.resize(sizeof(RTPHeader) + jpegBuffer.size());
//         memcpy(rtpPacket.data(), &rtpHeader, sizeof(RTPHeader));
//         memcpy(rtpPacket.data() + sizeof(RTPHeader), jpegBuffer.data(), jpegBuffer.size());

//         // Send RTP packet over UDP
//         ssize_t sentBytes = sendto(sock, rtpPacket.data(), rtpPacket.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
//         if (sentBytes < 0) {
//             perror("sendto");
//         }

//         usleep(1000);  // Throttle to maintain frame rate
//     }

//     close(sock);
//     return 0;
// }

// #include <iostream>
// #include <opencv2/opencv.hpp>
// #include <cstring>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <sys/time.h>
// #include <vector>

// // Define the RTP packet header
// struct RTPHeader {
//     uint8_t version:2;
//     uint8_t padding:1;
//     uint8_t extension:1;
//     uint8_t csrcCount:4;
//     uint8_t marker:1;
//     uint8_t payloadType:7;
//     uint16_t sequenceNumber;
//     uint32_t timestamp;
//     uint32_t ssrc;
// };

// // Constants
// #define PORT 5000
// #define RTP_PAYLOAD_TYPE 96  // For MJPEG
// #define FRAME_RATE 30        // Frames per second

// // Helper to get the current timestamp in ms
// uint32_t getTimestamp() {
//     struct timeval tv;
//     gettimeofday(&tv, nullptr);
//     return (tv.tv_sec * 1000) + (tv.tv_usec / 1000); // timestamp in ms
// }

// // Function to create RTP header
// void createRTPHeader(RTPHeader& header, uint32_t timestamp, uint16_t seqNum) {
//     header.version = 2;            // RTP version
//     header.padding = 0;            // No padding
//     header.extension = 0;          // No extension
//     header.csrcCount = 0;          // No CSRC
//     header.marker = 0;             // No marker
//     header.payloadType = RTP_PAYLOAD_TYPE;
//     header.sequenceNumber = seqNum;
//     header.timestamp = timestamp;
//     header.ssrc = 12345;           // Arbitrary SSRC
// }

// int main() {
//     cv::VideoCapture cap("/dev/video0");  // Open camera
//     if (!cap.isOpened()) {
//         std::cerr << "Failed to open camera\n";
//         return 1;
//     }

//     cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
//     cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

//     // Prepare UDP socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         return 1;
//     }

//     sockaddr_in clientAddr{};
//     clientAddr.sin_family = AF_INET;
//     clientAddr.sin_port = htons(PORT);
//     clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change if using a different machine

//     uint16_t seqNum = 0;
//     uint32_t timestamp = 0;
//     std::vector<uchar> jpegBuffer;
//     std::vector<uint8_t> rtpPacket;

//     while (true) {
//         cv::Mat frame;
//         cap >> frame;  // Capture frame from camera
//         if (frame.empty()) continue;

//         // Compress frame to JPEG
//         std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
//         cv::imencode(".jpg", frame, jpegBuffer, params);

//         // Prepare RTP header and packet
//         RTPHeader rtpHeader;
//         createRTPHeader(rtpHeader, timestamp, seqNum++);
//         timestamp += 1000 / FRAME_RATE; // Simulate 30 fps

//         // Copy RTP header into packet buffer
//         rtpPacket.resize(sizeof(RTPHeader) + jpegBuffer.size());
//         memcpy(rtpPacket.data(), &rtpHeader, sizeof(RTPHeader));
//         memcpy(rtpPacket.data() + sizeof(RTPHeader), jpegBuffer.data(), jpegBuffer.size());

//         // Send RTP packet over UDP
//         ssize_t sentBytes = sendto(sock, rtpPacket.data(), rtpPacket.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
//         if (sentBytes < 0) {
//             perror("sendto");
//         }

//         usleep(1000);  // Throttle to maintain frame rate
//     }

//     close(sock);
//     return 0;
// }

// Video and Audio Streaming using RTP
// #include <iostream>
// #include <fstream>
// #include <fcntl.h>
// #include <unistd.h>
// #include <cstring>
// #include <sys/ioctl.h>
// #include <sys/mman.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <thread>
// #include <vector>
// #include <linux/videodev2.h>
// #include <alsa/asoundlib.h>

// #define VIDEO_PORT 5000
// #define AUDIO_PORT 5002
// #define AUDIO_DEVICE "default"
// #define SAMPLE_RATE 44100
// #define CHANNELS 1
// #define AUDIO_BUFFER_SIZE 4096
// #define RTP_HEADER_SIZE 12

// // RTP header structure
// struct RTPHeader {
//     uint8_t version:2;
//     uint8_t padding:1;
//     uint8_t extension:1;
//     uint8_t csrcCount:4;
//     uint8_t marker:1;
//     uint8_t payloadType:7;
//     uint16_t sequenceNumber;
//     uint32_t timestamp;
//     uint32_t ssrc;
// };

// // Create RTP header
// void createRTPHeader(RTPHeader& header, uint32_t ts, uint16_t seqNum, uint8_t payloadType) {
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

// // V4L2 buffer structure
// struct Buffer {
//     void* start;
//     size_t length;
// };

// // MJPEG video streaming
// void videoStreaming(const std::string& ip) {
//     int fd = open("/dev/video0", O_RDWR);
//     if (fd < 0) {
//         perror("Failed to open video device");
//         return;
//     }

//     // Set format to MJPEG
//     v4l2_format fmt{};
//     fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     fmt.fmt.pix.width = 640;
//     fmt.fmt.pix.height = 480;
//     fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
//     fmt.fmt.pix.field = V4L2_FIELD_NONE;
//     ioctl(fd, VIDIOC_S_FMT, &fmt);

//     // Request buffers
//     v4l2_requestbuffers req{};
//     req.count = 4;
//     req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     req.memory = V4L2_MEMORY_MMAP;
//     ioctl(fd, VIDIOC_REQBUFS, &req);

//     std::vector<Buffer> buffers(req.count);
//     for (int i = 0; i < req.count; ++i) {
//         v4l2_buffer buf{};
//         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//         buf.memory = V4L2_MEMORY_MMAP;
//         buf.index = i;
//         ioctl(fd, VIDIOC_QUERYBUF, &buf);
//         buffers[i].length = buf.length;
//         buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
//         ioctl(fd, VIDIOC_QBUF, &buf);
//     }

//     v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     ioctl(fd, VIDIOC_STREAMON, &type);

//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in client{};
//     client.sin_family = AF_INET;
//     client.sin_port = htons(VIDEO_PORT);
//     client.sin_addr.s_addr = inet_addr(ip.c_str());

//     uint16_t seq = 0;
//     uint32_t ts = 0;

//     while (true) {
//         v4l2_buffer buf{};
//         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//         buf.memory = V4L2_MEMORY_MMAP;
//         ioctl(fd, VIDIOC_DQBUF, &buf);

//         uint8_t* jpegData = static_cast<uint8_t*>(buffers[buf.index].start);
//         size_t jpegSize = buf.bytesused;

//         std::vector<uint8_t> packet(RTP_HEADER_SIZE + jpegSize);
//         RTPHeader hdr;
//         createRTPHeader(hdr, ts, seq++, 96);
//         memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
//         memcpy(packet.data() + RTP_HEADER_SIZE, jpegData, jpegSize);

//         sendto(sock, packet.data(), packet.size(), 0, (sockaddr*)&client, sizeof(client));
//         std::cout << "[Video] Sent frame: Size = " << jpegSize << " bytes\n";

//         ts += 1000 / 30;  // Simulate 30 fps
//         usleep(1000000 / 30);

//         ioctl(fd, VIDIOC_QBUF, &buf);
//     }

//     ioctl(fd, VIDIOC_STREAMOFF, &type);
//     close(fd);
//     close(sock);
// }

// // PCM audio streaming
// void audioStreaming(const std::string& ip) {
//     snd_pcm_t *handle;
//     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
//     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
//                        CHANNELS, SAMPLE_RATE, 1, 500000);  // 0.5 sec latency

//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in client{};
//     client.sin_family = AF_INET;
//     client.sin_port = htons(AUDIO_PORT);
//     client.sin_addr.s_addr = inet_addr(ip.c_str());

//     uint16_t seq = 0;
//     uint32_t ts = 0;

//     char buffer[AUDIO_BUFFER_SIZE];
//     while (true) {
//         snd_pcm_readi(handle, buffer, AUDIO_BUFFER_SIZE / 2);  // 2 bytes per sample
//         std::vector<uint8_t> packet(RTP_HEADER_SIZE + AUDIO_BUFFER_SIZE);
//         RTPHeader hdr;
//         createRTPHeader(hdr, ts, seq++, 97);
//         memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
//         memcpy(packet.data() + RTP_HEADER_SIZE, buffer, AUDIO_BUFFER_SIZE);
//         sendto(sock, packet.data(), packet.size(), 0, (sockaddr*)&client, sizeof(client));

//         ts += AUDIO_BUFFER_SIZE;
//         std::cout << "[Audio] Sent " << AUDIO_BUFFER_SIZE << " bytes\n";
//     }

//     close(sock);
//     snd_pcm_close(handle);
// }

// // Entry point
// int main() {
//     std::string clientIP = "127.0.0.1";

//     std::thread audioThread(audioStreaming, clientIP);
//     std::thread videoThread(videoStreaming, clientIP);

//     videoThread.join();
//     audioThread.join();

//     return 0;
// }

#include <iostream>
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
#include <opus/opus.h> // ✅ Opus encoder

#define VIDEO_PORT 5000
#define AUDIO_PORT 5002
#define AUDIO_DEVICE "default"
#define SAMPLE_RATE 48000
#define CHANNELS 1
#define AUDIO_BUFFER_SIZE 4096
#define RTP_HEADER_SIZE 12

// RTP header
struct RTPHeader
{
    uint8_t version : 2;
    uint8_t padding : 1;
    uint8_t extension : 1;
    uint8_t csrcCount : 4;
    uint8_t marker : 1;
    uint8_t payloadType : 7;
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint32_t ssrc;
};

void createRTPHeader(RTPHeader &header, uint32_t ts, uint16_t seqNum, uint8_t payloadType)
{
    header.version = 2;
    header.padding = 0;
    header.extension = 0;
    header.csrcCount = 0;
    header.marker = 0;
    header.payloadType = payloadType;
    header.sequenceNumber = htons(seqNum);
    header.timestamp = htonl(ts);
    header.ssrc = htonl(1234);
}

struct Buffer
{
    void *start;
    size_t length;
};

// ✅ VIDEO THREAD (Same as before)
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
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
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
    uint32_t ts = 0;
    tjhandle tjInstance = tjInitCompress();

    while (true)
    {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ioctl(fd, VIDIOC_DQBUF, &buf);

        uint8_t *yuyvData = static_cast<uint8_t *>(buffers[buf.index].start);
        int width = fmt.fmt.pix.width;
        int height = fmt.fmt.pix.height;

        cv::Mat yuyv(height, width, CV_8UC2, yuyvData);
        cv::Mat bgr;
        cv::cvtColor(yuyv, bgr, cv::COLOR_YUV2BGR_YUYV);

        unsigned char *jpegBuf = nullptr;
        unsigned long jpegSize = 0;
        int jpegQuality = 80;

        if (tjCompress2(tjInstance, bgr.data, width, 0, height, TJPF_BGR,
                        &jpegBuf, &jpegSize, TJSAMP_420, jpegQuality, TJFLAG_FASTDCT) < 0)
        {
            std::cerr << "JPEG compression failed\n";
            continue;
        }

        std::vector<uint8_t> packet(RTP_HEADER_SIZE + jpegSize);
        RTPHeader hdr;
        createRTPHeader(hdr, ts, seq++, 96);
        memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
        memcpy(packet.data() + RTP_HEADER_SIZE, jpegBuf, jpegSize);

        sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&client, sizeof(client));
        std::cout << "[Video] Sent frame: seq=" << seq - 1 << ", timestamp=" << ts
                  << ", JPEG size=" << jpegSize << " bytes, total packet=" << packet.size() << " bytes\n";
        tjFree(jpegBuf);
        ioctl(fd, VIDIOC_QBUF, &buf);
        usleep(1000000 / 15);

        static int videoFrameCount = 0;
        videoFrameCount++;
        if (videoFrameCount % 100 == 0)
        {
            std::cout << "[Video] Sent " << videoFrameCount << " frames so far\n";
        }

        ts += 1000 / 15;
    }

    tjDestroy(tjInstance);
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    close(fd);
    close(sock);
}

// ✅ AUDIO THREAD — Using Opus
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
    uint32_t ts = 0;

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
        std::cout << "[Audio] Sent frame: seq=" << seq-1 << ", timestamp=" << ts 
          << ", Opus size=" << encodedBytes << " bytes, total packet=" << packet.size() << " bytes\n";
        ts += frame_size; // Increment timestamp by number of samples
    }

    close(sock);
    snd_pcm_close(handle);
    opus_encoder_destroy(encoder);
}

int main()
{
    std::string ip = "127.0.0.1";
    std::thread videoThread(videoStreaming, ip);
    std::thread audioThread(audioStreaming, ip);
    videoThread.join();
    audioThread.join();
    return 0;
}


//FOR X86
// g++ streamer.cpp -o streamer -lturbojpeg `pkg-config --cflags --libs opencv4` -lasound -lopus -lpthread

// FOR ARM
//