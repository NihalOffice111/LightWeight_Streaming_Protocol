// #include <iostream>
// #include <cstring>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <opencv2/opencv.hpp>

// #define PORT 5000
// #define BUFFER_SIZE 65536
// #define HEADER_MAGIC 0xDEADBEEF

// #pragma pack(push, 1)
// struct FrameHeader {
//     uint32_t magic;
//     uint32_t frame_id;
//     uint64_t timestamp;
//     uint32_t frame_size;
// };
// #pragma pack(pop)

// int main() {
//     // Create UDP socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         return 1;
//     }

//     sockaddr_in server_addr{};
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
//     server_addr.sin_addr.s_addr = INADDR_ANY;

//     if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         perror("Bind failed");
//         return 1;
//     }

//     std::cout << "Listening for UDP MJPEG frames on port " << PORT << "...\n";

//     uint8_t buffer[BUFFER_SIZE];

//     while (true) {
//         ssize_t recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
//         if (recv_len < (ssize_t)sizeof(FrameHeader)) {
//             std::cerr << "Packet too small\n";
//             continue;
//         }

//         FrameHeader header;
//         memcpy(&header, buffer, sizeof(FrameHeader));

//         if (ntohl(header.magic) != HEADER_MAGIC) {
//             std::cerr << "Invalid magic header\n";
//             continue;
//         }

//         uint32_t frame_id = ntohl(header.frame_id);
//         uint64_t timestamp = be64toh(header.timestamp);
//         uint32_t frame_size = ntohl(header.frame_size);

//         if (frame_size != (recv_len - sizeof(FrameHeader))) {
//             std::cerr << "Mismatch in frame size\n";
//             continue;
//         }

//         std::vector<uchar> jpeg_data(buffer + sizeof(FrameHeader), buffer + recv_len);
//         cv::Mat frame = cv::imdecode(jpeg_data, cv::IMREAD_COLOR);
//         if (frame.empty()) {
//             std::cerr << "Failed to decode frame\n";
//             continue;
//         }

//         cv::imshow("MJPEG Stream", frame);
//         if (cv::waitKey(1) == 27) break; // ESC to quit
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
// #define BUFFER_SIZE 65536

// // Helper function to extract RTP header from the buffer
// RTPHeader extractRTPHeader(const uint8_t* data) {
//     RTPHeader header;
//     memcpy(&header, data, sizeof(RTPHeader));
//     return header;
// }

// int main() {
//     // Prepare UDP socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         return 1;
//     }

//     sockaddr_in serverAddr{};
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_port = htons(PORT);
//     serverAddr.sin_addr.s_addr = INADDR_ANY;

//     if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
//         perror("Bind failed");
//         return 1;
//     }

//     std::vector<uint8_t> buffer(BUFFER_SIZE);

//     while (true) {
//         // Receive RTP packet
//         ssize_t recvLen = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//         if (recvLen < sizeof(RTPHeader)) {
//             std::cerr << "Invalid RTP packet\n";
//             continue;
//         }

//         RTPHeader rtpHeader = extractRTPHeader(buffer.data());

//         // Extract JPEG data (after RTP header)
//         std::vector<uchar> jpegData(buffer.begin() + sizeof(RTPHeader), buffer.begin() + recvLen);
//         cv::Mat frame = cv::imdecode(jpegData, cv::IMREAD_COLOR);

//         if (!frame.empty()) {
//             cv::imshow("RTP Stream", frame);
//             if (cv::waitKey(1) == 27) {
//                 break; // ESC to quit
//             }
//         }
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
// #define BUFFER_SIZE 65536

// // Helper function to extract RTP header from the buffer
// RTPHeader extractRTPHeader(const uint8_t* data) {
//     RTPHeader header;
//     memcpy(&header, data, sizeof(RTPHeader));
//     return header;
// }

// int main() {
//     // Prepare UDP socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         return 1;
//     }

//     sockaddr_in serverAddr{};
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_port = htons(PORT);
//     serverAddr.sin_addr.s_addr = INADDR_ANY;

//     if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
//         perror("Bind failed");
//         return 1;
//     }

//     std::vector<uint8_t> buffer(BUFFER_SIZE);

//     while (true) {
//         // Receive RTP packet
//         ssize_t recvLen = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//         if (recvLen < sizeof(RTPHeader)) {
//             std::cerr << "Invalid RTP packet\n";
//             continue;
//         }

//         RTPHeader rtpHeader = extractRTPHeader(buffer.data());

//         // Extract JPEG data (after RTP header)
//         std::vector<uchar> jpegData(buffer.begin() + sizeof(RTPHeader), buffer.begin() + recvLen);
//         cv::Mat frame = cv::imdecode(jpegData, cv::IMREAD_COLOR);

//         if (!frame.empty()) {
//             cv::imshow("RTP Stream", frame);
//             if (cv::waitKey(1) == 27) {
//                 break; // ESC to quit
//             }
//         }
//     }

//     close(sock);
//     return 0;
// }




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

#define VIDEO_PORT 5000
#define AUDIO_PORT 5002
#define BUFFER_SIZE 65536
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_DEVICE "default"
#define SAMPLE_RATE 44100
#define CHANNELS 1

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
        std::vector<uchar> jpeg(buffer.begin() + 12, buffer.begin() + len);
        cv::Mat frame = cv::imdecode(jpeg, cv::IMREAD_COLOR);
        if (frame.empty()) {
            std::cerr << "Failed to decode JPEG frame\n";
        } else {
            std::cout << "Decoded frame received\n";
        }
        
        if (!frame.empty()) {
            cv::imshow("Video", frame);
            if (cv::waitKey(1) == 27) break;
        }
    }

    close(sock);
}

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

    std::vector<uint8_t> buffer(BUFFER_SIZE);

    while (true) {
        ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (len < 12) continue;
        snd_pcm_writei(handle, buffer.data() + 12, AUDIO_BUFFER_SIZE / 2);
    }

    close(sock);
    snd_pcm_close(handle);
}

int main() {
    std::thread videoThread(videoReceiver);
    std::thread audioThread(audioReceiver);
    videoThread.join();
    audioThread.join();
    return 0;
}
