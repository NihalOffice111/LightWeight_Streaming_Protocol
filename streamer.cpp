// //ORIGINAL CODE
// #include <iostream>
// #include <fstream> // Include for std::ofstream
// #include <fcntl.h>
// #include <unistd.h>
// #include <cstring>
// #include <sys/ioctl.h>
// #include <sys/mman.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <thread>
// #include <vector>
// #include <linux/videodev2.h> //for V4L2
// //#include <alsa/asoundlib.h> //for ALSA audio
// #include <alsa/asoundlib.h>
// #include <turbojpeg.h> //for JPEG compression
// #include <opus/opus.h> // Opus encoder

// #define VIDEO_PORT 5000
// #define AUDIO_PORT 5002
// #define AUDIO_DEVICE "default"
// #define SAMPLE_RATE 48000  //audio Sampling rate
// #define CHANNELS 1 
// #define AUDIO_BUFFER_SIZE 4096 //audio buffer size for audio data
// #define RTP_HEADER_SIZE 12
// #define BUFFER_SIZE 65536 //buffer size for receiving data
// #define IP_ADDRESS "127.0.0.1"
// #define WIDTH 640
// #define HEIGHT 480
// #define MJPEG_HEADER_SIZE 8
// #define MAX_RTP_PACKET 1400

// // RTP header
// struct RTPHeader //12 bytes
// {
//     uint8_t version : 2; //RTP version
//     uint8_t padding : 1; //padding
//     uint8_t extension : 1; //extension header
//     uint8_t csrcCount : 4; //no of contributing Sources
//     uint8_t marker : 1; //marks Significant events
//     uint8_t payloadType : 7; //Identifies the Payload type
//     uint16_t sequenceNumber; //track packet order to detect loss reordering
//     uint32_t timestamp; //sampling time for sync
//     uint32_t ssrc; //identifies the sync source
// };

// struct MJPEGHeader {
//     uint8_t type_specific; // 0 for generic MJPEG
//     uint8_t fragment_offset[3]; // 24-bit offset
//     uint8_t type; // 0 for baseline JPEG
//     uint8_t q; // Quality (0-255)
//     uint8_t width; // Width / 8
//     uint8_t height; // Height / 8
// };

// // #pragma pack(push, 1) //12 bytes
// // struct RTPHeader {
// //     uint8_t version_p_x_cc;   // V=2, P=0, X=0, CC=0 → 0x80
// //     uint8_t m_payloadtype;    // M=0, PT=96 or 97
// //     uint16_t sequenceNumber;  // htons
// //     uint32_t timestamp;       // htonl
// //     uint32_t ssrc;            // htonl
// // };
// // #pragma pack(pop)

// // JPEG Payload Header (per RFC 2435)



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

// void createMJPEGHeader(MJPEGHeader &header, uint32_t offset, uint8_t quality) {
//     header.type_specific = 0;
//     header.fragment_offset[0] = (offset >> 16) & 0xFF;
//     header.fragment_offset[1] = (offset >> 8) & 0xFF;
//     header.fragment_offset[2] = offset & 0xFF;
//     header.type = 0; // Baseline JPEG
//     header.q = quality;
//     header.width = WIDTH / 8;
//     header.height = HEIGHT / 8;
// }



// // void createRTPHeader(RTPHeader &header, uint32_t timestamp, uint16_t sequence, uint8_t payloadType)
// // {
// //     header.version_p_x_cc = 0x80;                  // V=2
// //     header.m_payloadtype = payloadType & 0x7F;     // PT = 96 or 97
// //     header.sequenceNumber = htons(sequence);
// //     header.timestamp = htonl(timestamp);
// //     header.ssrc = htonl(123456);                   // Arbitrary SSRC
// // }




// struct Buffer
// {
//     void *start;
//     size_t length;
// };

// // Utility: Clamp to [0, 255]
// uint8_t clamp(int val)
// {
//     return val < 0 ? 0 : (val > 255 ? 255 : val);
// }

// void videoStreaming(const std::string &ip)
// {
//     int fd = open("/dev/video0", O_RDWR);
//     if (fd < 0)
//     {
//         perror("Video device open failed");
//         return;
//     }
//     std::cout << "[Video] Opened video device /dev/video0\n";
//     std::cout<<"size of RTPHeader: "<<sizeof(RTPHeader)<<"\n";

//     // Configure V4L2
//     v4l2_format fmt{};
//     fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     fmt.fmt.pix.width = WIDTH;
//     fmt.fmt.pix.height = HEIGHT;
//     fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // YUYV = YUV422
//     fmt.fmt.pix.field = V4L2_FIELD_NONE;
//     ioctl(fd, VIDIOC_S_FMT, &fmt);

//     //Request Buffers :- Request memory buffers for video capture
//     v4l2_requestbuffers req{};
//     req.count = 4;              // 4 buffers
//     req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     req.memory = V4L2_MEMORY_MMAP;
//     ioctl(fd, VIDIOC_REQBUFS, &req);


//     //Map buffers: Maps the req buffers to user space and enqueues them
//     std::vector<Buffer> buffers(req.count);
//     for (int i = 0; i < req.count; ++i)
//     {
//         v4l2_buffer buf{};
//         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//         buf.memory = V4L2_MEMORY_MMAP;
//         buf.index = i;
//         ioctl(fd, VIDIOC_QUERYBUF, &buf);
//         buffers[i].length = buf.length;
//         buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
//         ioctl(fd, VIDIOC_QBUF, &buf);
//     }

//     //Starting the video stream
//     v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     ioctl(fd, VIDIOC_STREAMON, &type);

//     //Create UDP socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in client{};
//     client.sin_family = AF_INET;
//     client.sin_port = htons(VIDEO_PORT);
//     client.sin_addr.s_addr = inet_addr(ip.c_str());

//     uint16_t seq = 0;
//     uint32_t ts = 0; //32
//     tjhandle tjInstance = tjInitCompress();

//     const int width = fmt.fmt.pix.width;
//     const int height = fmt.fmt.pix.height;

//     // Allocate Planar buffer sizes
//     const int Y_size = width * height;
//     const int UV_size = (width / 2) * height;

//     std::vector<uint8_t> Y_plane(Y_size);
//     std::vector<uint8_t> U_plane(UV_size);
//     std::vector<uint8_t> V_plane(UV_size);

//     // Plane pointers
//     const unsigned char *planes[3] = {
//         Y_plane.data(),
//         U_plane.data(),
//         V_plane.data()};

//     // Stride for each row (no padding)(row lengths).
//     int strides[3] = {
//         width,     // Y stride
//         width / 2, // U stride
//         width / 2  // V stride
//     };

//     while (true)
//     {
//         v4l2_buffer buf{};
//         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//         buf.memory = V4L2_MEMORY_MMAP;
//         ioctl(fd, VIDIOC_DQBUF, &buf);

//         uint8_t *yuyv = static_cast<uint8_t *>(buffers[buf.index].start);

//         // Convert packed YUYV → YUV422 Planar (4 bytes per 2 pixels)
//         for (int i = 0, j = 0; i < width * height * 2; i += 4, j += 2)
//         {
//             uint8_t y0 = yuyv[i];
//             uint8_t u = yuyv[i + 1];
//             uint8_t y1 = yuyv[i + 2];
//             uint8_t v = yuyv[i + 3];

//             Y_plane[j] = y0;
//             Y_plane[j + 1] = y1;
//             U_plane[j / 2] = u;
//             V_plane[j / 2] = v;
//         }

//         unsigned char *jpegBuf = nullptr;
//         unsigned long jpegSize = 0;
//         int jpegQuality = 80;

//         // Compress using tjCompressFromYUVPlanes (YUV422 Planes to JPEG)
//         if (tjCompressFromYUVPlanes(tjInstance, planes, width, strides, height,
//                                     TJSAMP_422, &jpegBuf, &jpegSize, jpegQuality, TJFLAG_FASTDCT) < 0)
//         {
//             std::cerr << "TurboJPEG compression failed: " << tjGetErrorStr() << "\n";
//             ioctl(fd, VIDIOC_QBUF, &buf);
//             continue;
//         }

//         //create and Send RTP Packet
//         std::vector<uint8_t> packet(RTP_HEADER_SIZE + jpegSize);
//         RTPHeader hdr;
//         createRTPHeader(hdr, ts, seq++, 96);
//         //memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
//         memcpy(packet.data(), reinterpret_cast<void*>(&hdr), sizeof(RTPHeader));
//         memcpy(packet.data() + RTP_HEADER_SIZE, jpegBuf, jpegSize);

//         //Combine RTP header and JPEG data
//         sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&client, sizeof(client));
//         std::cout << "[Video] Sent frame: seq=" << seq - 1 << ", timestamp=" << ts
//                   << ", JPEG size=" << jpegSize << " bytes, total packet=" << packet.size() << " bytes\n";

//         tjFree(jpegBuf);
//         ioctl(fd, VIDIOC_QBUF, &buf);
//         usleep(1000000 / 15);
//         ts += 6000;
//     }

//     tjDestroy(tjInstance);
//     ioctl(fd, VIDIOC_STREAMOFF, &type);
//     close(fd);
//     close(sock);
// }

// // AUDIO THREAD — Using Opus
// void audioStreaming(const std::string &ip)
// {
//     int err;
//     //Initialize Opus encoder for 48kHz
//     OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_AUDIO, &err);
//     if (err != OPUS_OK)
//     {
//         std::cerr << "Opus encoder error: " << opus_strerror(err) << "\n";
//         return;
//     }
//     std::cout << "[Audio] Created Opus encoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";
    
//     //Configured ALSA
//     snd_pcm_t *handle;
//     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
//     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
//                        CHANNELS, SAMPLE_RATE, 1, 500000);

//     //Creating the UDP Socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in client{};
//     client.sin_family = AF_INET;
//     client.sin_port = htons(AUDIO_PORT);
//     client.sin_addr.s_addr = inet_addr(ip.c_str());

//     //Allocate the buffers
//     const int frame_size = 960;                         // 20ms at 48kHz
//     std::vector<int16_t> buffer(frame_size * CHANNELS); // 16-bit samples
//     unsigned char opusData[4000];
//     uint16_t seq = 0;
//     uint32_t ts = 0 ;//32;

//     while (true)
//     {
//         // Read exactly frame_size frames (frame_size * CHANNELS * sizeof(int16_t) bytes)
//         snd_pcm_readi(handle, buffer.data(), frame_size);

//         //Opus encoding :- Encodes PCM audio to Opus format
//         int encodedBytes = opus_encode(encoder, buffer.data(), frame_size, opusData, sizeof(opusData));
//         if (encodedBytes < 0)
//         {
//             std::cerr << "Opus encode error: " << opus_strerror(encodedBytes) << "\n";
//             continue;
//         }

//         //Creates and Send RTP Packet
//         std::vector<uint8_t> packet(RTP_HEADER_SIZE + encodedBytes);
//         RTPHeader hdr;
//         createRTPHeader(hdr, ts, seq++, 97);
//         memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
//         memcpy(packet.data() + RTP_HEADER_SIZE, opusData, encodedBytes);
//         sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&client, sizeof(client));
//         std::cout << "[Audio] Sent frame: seq=" << seq - 1 << ", timestamp=" << ts
//                   << ", Opus size=" << encodedBytes << " bytes, total packet=" << packet.size() << " bytes\n";
//         ts += frame_size; // Increment timestamp by number of samples
//     }

//     close(sock);
//     snd_pcm_close(handle);
//     opus_encoder_destroy(encoder);
// }


// // AUDIO RECEIVER — for Two-Way Audio
// /*
// void audioReceiver()
// {
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(AUDIO_PORT + 10); // Use different port for receiving (5012)
//     addr.sin_addr.s_addr = INADDR_ANY;
//     bind(sock, (sockaddr *)&addr, sizeof(addr));

//     snd_pcm_t *handle;
//     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
//     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
//                        CHANNELS, SAMPLE_RATE, 1, 500000);

//     int err;
//     OpusDecoder *decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
//     if (err != OPUS_OK)
//     {
//         std::cerr << "Opus decoder create error: " << opus_strerror(err) << "\n";
//         return;
//     }
//     std::vector<uint8_t> buffer(BUFFER_SIZE);
//     std::vector<opus_int16> decodedPcm(2048 * CHANNELS);

//     while (true)
//     {
//         ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//         if (len < 12) continue;

//         unsigned char *opusPayload = buffer.data() + 12;
//         int frameSize = opus_decode(decoder, opusPayload, len - 12, decodedPcm.data(), 2048, 0);
//         if (frameSize < 0)
//         {
//             std::cerr << "Opus decode error: " << opus_strerror(frameSize) << "\n";
//             continue;
//         }

//         if (snd_pcm_writei(handle, decodedPcm.data(), frameSize) < 0)
//         {
//             std::cerr << "ALSA playback error\n";
//             snd_pcm_recover(handle, -1, 1);
//         }
//     }

//     close(sock);
//     snd_pcm_close(handle);
//     opus_decoder_destroy(decoder);
// }

// */

// int main()
// {
//     std::string ip = IP_ADDRESS;
//     std::thread videoThread(videoStreaming, ip);
//     std::thread audioThread(audioStreaming, ip);
//    // std::thread audioReceiverThread(audioReceiver);
//     videoThread.join();
//     audioThread.join();
//   //  audioReceiverThread.join();
//     std::cout << "All threads completed.\n";
//     return 0;
// }











// // // FOR X86
// /*
  //g++ streamer.cpp -o streamer -lturbojpeg `pkg-config --cflags --libs opencv4` -lasound -lopus -lpthread
// */

// // // FOR ARM
// // //



#include <iostream>
#include <fstream>
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
#include <opus/opus.h>

#define VIDEO_PORT 5000
#define AUDIO_PORT 5002
#define AUDIO_DEVICE "default"
#define SAMPLE_RATE 48000
#define CHANNELS 1
#define AUDIO_BUFFER_SIZE 4096
#define RTP_HEADER_SIZE 12
#define MJPEG_HEADER_SIZE 8
#define BUFFER_SIZE 65536
#define IP_ADDRESS "127.0.0.1"
//#define IP_ADDRESS "192.168.1.130"
#define WIDTH 640
#define HEIGHT 480
#define MAX_RTP_PACKET 1400
#define FPS 15

struct MJPEGHeader {
    uint8_t type_specific;
    uint8_t fragment_offset[3];
    uint8_t type;
    uint8_t q;
    uint8_t width;
    uint8_t height;
};

void createRTPHeader(uint8_t* header, uint32_t ts, uint16_t seqNum, uint8_t payloadType, bool marker = false) {
    header[0] = 0x80; // Version 2, no padding, no extension, CSRC count 0
    header[1] = payloadType | (marker ? 0x80 : 0x00); // Payload type, marker bit
    header[2] = (seqNum >> 8) & 0xFF; // Sequence number (big-endian)
    header[3] = seqNum & 0xFF;
    header[4] = (ts >> 24) & 0xFF; // Timestamp (big-endian)
    header[5] = (ts >> 16) & 0xFF;
    header[6] = (ts >> 8) & 0xFF;
    header[7] = ts & 0xFF;
    header[8] = (1234 >> 24) & 0xFF; // SSRC (big-endian, fixed to 1234)
    header[9] = (1234 >> 16) & 0xFF;
    header[10] = (1234 >> 8) & 0xFF;
    header[11] = 1234 & 0xFF;
}

void createMJPEGHeader(MJPEGHeader &header, uint32_t offset, uint8_t quality) {
    header.type_specific = 0;
    header.fragment_offset[0] = (offset >> 16) & 0xFF;
    header.fragment_offset[1] = (offset >> 8) & 0xFF;
    header.fragment_offset[2] = offset & 0xFF;
    header.type = 0;
    header.q = quality;
    header.width = WIDTH / 8;
    header.height = HEIGHT / 8;
}

struct Buffer {
    void *start;
    size_t length;
};

uint8_t clamp(int val) {
    return val < 0 ? 0 : (val > 255 ? 255 : val);
}

void videoStreaming(const std::string &ip) {
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0) {
        perror("Video device open failed");
        return;
    }
    std::cout << "[Video] Opened video device /dev/video0\n";

    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("Set format failed");
        close(fd);
        return;
    }

    v4l2_requestbuffers req{};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("Request buffers failed");
        close(fd);
        return;
    }

    std::vector<Buffer> buffers(req.count);
    for (int i = 0; i < req.count; ++i) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("Query buffer failed");
            close(fd);
            return;
        }
        buffers[i].length = buf.length;
        buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("Queue buffer failed");
            close(fd);
            return;
        }
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Stream on failed");
        close(fd);
        return;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        close(fd);
        return;
    }
    std::cout << "[Video] Created UDP socket\n";

    sockaddr_in client{};
    client.sin_family = AF_INET;
    client.sin_port = htons(VIDEO_PORT);
    if (inet_addr(ip.c_str()) == INADDR_NONE) {
        std::cerr << "[Video] Invalid IP address: " << ip << "\n";
        close(sock);
        close(fd);
        return;
    }
    client.sin_addr.s_addr = inet_addr(ip.c_str());
    std::cout << "[Video] Sending to IP: " << ip << ", port: " << VIDEO_PORT << "\n";

    uint16_t seq = 0;
    uint32_t ts = 0;
    tjhandle tjInstance = tjInitCompress();
    if (!tjInstance) {
        std::cerr << "[Video] Failed to initialize TurboJPEG\n";
        close(fd);
        close(sock);
        return;
    }

    const int width = fmt.fmt.pix.width;
    const int height = fmt.fmt.pix.height;

    const int Y_size = width * height;
    const int UV_size = (width / 2) * height;

    std::vector<uint8_t> Y_plane(Y_size);
    std::vector<uint8_t> U_plane(UV_size);
    std::vector<uint8_t> V_plane(UV_size);

    const unsigned char *planes[3] = {
        Y_plane.data(),
        U_plane.data(),
        V_plane.data()};

    int strides[3] = {
        width,
        width / 2,
        width / 2
    };

    while (true) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            perror("Dequeue buffer failed");
            continue;
        }

        uint8_t *yuyv = static_cast<uint8_t *>(buffers[buf.index].start);

        for (int i = 0, j = 0; i < width * height * 2; i += 4, j += 2) {
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
        int jpegQuality = 75;

        if (tjCompressFromYUVPlanes(tjInstance, planes, width, strides, height,
                                    TJSAMP_422, &jpegBuf, &jpegSize, jpegQuality, TJFLAG_FASTDCT) < 0) {
            std::cerr << "[Video] TurboJPEG compression failed: " << tjGetErrorStr() << "\n";
            ioctl(fd, VIDIOC_QBUF, &buf);
            continue;
        }

        size_t offset = 0;
        size_t maxPayload = MAX_RTP_PACKET - RTP_HEADER_SIZE - MJPEG_HEADER_SIZE;
        while (offset < jpegSize) {
            size_t chunkSize = (jpegSize - offset) > maxPayload ? maxPayload : (jpegSize - offset);
            bool marker = (offset + chunkSize >= jpegSize);

            std::vector<uint8_t> packet(RTP_HEADER_SIZE + MJPEG_HEADER_SIZE + chunkSize);
            MJPEGHeader mjpegHdr;
            uint8_t rtpHdr[RTP_HEADER_SIZE];

            createRTPHeader(rtpHdr, ts, seq++, 26, marker);
            createMJPEGHeader(mjpegHdr, offset, jpegQuality);

            memcpy(packet.data(), rtpHdr, RTP_HEADER_SIZE);
            memcpy(packet.data() + RTP_HEADER_SIZE, &mjpegHdr, MJPEG_HEADER_SIZE);
            memcpy(packet.data() + RTP_HEADER_SIZE + MJPEG_HEADER_SIZE, jpegBuf + offset, chunkSize);

            ssize_t sent = sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&client, sizeof(client));
            if (sent < 0) {
                std::cerr << "[Video] Sendto failed: " << strerror(errno) << "\n";
            } else {
                std::cout << "[Video] Sent packet: seq=" << seq - 1 << ", timestamp=" << ts
                          << ", offset=" << offset << ", size=" << chunkSize << ", total=" << sent
                          << ", marker=" << marker << "\n";
            }

            offset += chunkSize;
        }

        tjFree(jpegBuf);
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("Queue buffer failed");
            continue;
        }
        usleep(1000000 / FPS);
        ts += 90000 / FPS; // CHANGED: Ensure timestamp increments correctly
    }

    tjDestroy(tjInstance);
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    close(fd);
    close(sock);
}

void audioStreaming(const std::string &ip) {
    int err;
    OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_AUDIO, &err);
    if (err != OPUS_OK) {
        std::cerr << "Opus encoder error: " << opus_strerror(err) << "\n";
        return;
    }
    std::cout << "[Audio] Created Opus encoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";

    snd_pcm_t *handle;
    if (snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0) < 0) {
        std::cerr << "[Audio] Failed to open ALSA capture device\n";
        opus_encoder_destroy(encoder);
        return;
    }
    snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                       CHANNELS, SAMPLE_RATE, 1, 500000);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[Audio] Socket creation failed");
        snd_pcm_close(handle);
        opus_encoder_destroy(encoder);
        return;
    }

    sockaddr_in client{};
    client.sin_family = AF_INET;
    client.sin_port = htons(AUDIO_PORT);
    client.sin_addr.s_addr = inet_addr(ip.c_str());

    const int frame_size = 960;
    std::vector<int16_t> buffer(frame_size * CHANNELS);
    unsigned char opusData[4000];
    uint16_t seq = 0;
    uint32_t ts = 0;

    while (true) {
        if (snd_pcm_readi(handle, buffer.data(), frame_size) != frame_size) {
            std::cerr << "[Audio] Failed to read audio samples\n";
            snd_pcm_recover(handle, -1, 1);
            continue;
        }

        int encodedBytes = opus_encode(encoder, buffer.data(), frame_size, opusData, sizeof(opusData));
        if (encodedBytes < 0) {
            std::cerr << "[Audio] Opus encode error: " << opus_strerror(encodedBytes) << "\n";
            continue;
        }

        std::vector<uint8_t> packet(RTP_HEADER_SIZE + encodedBytes);
        uint8_t rtpHdr[RTP_HEADER_SIZE];
        createRTPHeader(rtpHdr, ts, seq++, 97);
        memcpy(packet.data(), rtpHdr, RTP_HEADER_SIZE);
        memcpy(packet.data() + RTP_HEADER_SIZE, opusData, encodedBytes);
        ssize_t sent = sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&client, sizeof(client));
        if (sent < 0) {
            std::cerr << "[Audio] Sendto failed: " << strerror(errno) << "\n";
        } else {
            std::cout << "[Audio] Sent frame: seq=" << seq - 1 << ", timestamp=" << ts
                      << ", Opus size=" << encodedBytes << " bytes, total packet=" << packet.size() << " bytes\n";
        }
        ts += frame_size;
    }

    close(sock);
    snd_pcm_close(handle);
    opus_encoder_destroy(encoder);
}

int main() {
    std::string ip = IP_ADDRESS;
    std::thread videoThread(videoStreaming, ip);
    std::thread audioThread(audioStreaming, ip);
    videoThread.join();
    audioThread.join();
    std::cout << "All threads completed.\n";
    return 0;
}