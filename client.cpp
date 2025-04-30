// // // Video and audio Receiving client
// #include <iostream>
// #include <opencv2/opencv.hpp>
// #include <cstring>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <thread>
// #include <vector>
// #include <arpa/inet.h>
// #include <alsa/asoundlib.h>
// #include <opus/opus.h>  // Opus decoder

// #define VIDEO_PORT 5000
// #define AUDIO_PORT 5002
// #define BUFFER_SIZE 65536
// #define AUDIO_BUFFER_SIZE 4096
// #define AUDIO_DEVICE "default"
// #define SAMPLE_RATE 48000
// #define CHANNELS 1
// #define RTP_HEADER_SIZE 12  

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


// // VIDEO RECEIVER 
// void videoReceiver() {
//     //Creates the UDP Socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(VIDEO_PORT);
//     addr.sin_addr.s_addr = INADDR_ANY;
//     bind(sock, (sockaddr*)&addr, sizeof(addr));


//     //Receive and Decode the video stream
//     std::vector<uint8_t> buffer(BUFFER_SIZE);
//     while (true) {
//         ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//         if (len < 12) continue;

//         std::cout<<"[Video] Received frame: size=" << len << " bytes\n";

//         // Skip RTP header
//         std::vector<uchar> jpeg(buffer.begin() + 12, buffer.begin() + len);
//         cv::Mat frame = cv::imdecode(jpeg, cv::IMREAD_COLOR);
//         if (frame.empty()) {
//             std::cerr << "Failed to decode JPEG frame\n";
//         } else {
//             cv::imshow("Video", frame);
//             std::cout<<"[Video] Decoded and displayed frame\n";
//             if (cv::waitKey(1) == 27) break;  // ESC to exit
//         }
//     }

//     close(sock);
// }

// // AUDIO RECEIVER — with Opus decoding
// // void audioReceiver() {
// //     int sock = socket(AF_INET, SOCK_DGRAM, 0);
// //     sockaddr_in addr{};
// //     addr.sin_family = AF_INET;
// //     addr.sin_port = htons(AUDIO_PORT);
// //     addr.sin_addr.s_addr = INADDR_ANY;
// //     bind(sock, (sockaddr*)&addr, sizeof(addr));

// //     snd_pcm_t *handle;
// //     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
// //     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
// //                        CHANNELS, SAMPLE_RATE, 1, 500000);

// //     int err;
// //     OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
// //     if (err != OPUS_OK) {
// //         std::cerr << "Failed to create Opus decoder: " << opus_strerror(err) << "\n";
// //         return;
// //     }
// //     std::cout << "Creating Opus decoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";

// //     std::vector<uint8_t> buffer(BUFFER_SIZE);
// //     std::vector<opus_int16> decodedPcm(2048 * CHANNELS); // Enough for 20ms at 48kHz

// //     while (true) {
// //         ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
// //         if (len < 12) continue;

// //         std::cout<<"Audio Decoding\n";
// //         unsigned char* opusPayload = buffer.data() + 128;
// //         int frameSize = opus_decode(decoder, opusPayload, len - 12, decodedPcm.data(), 2048, 0);
// //         if (frameSize < 0) {
// //             std::cerr << "Opus decode error: " << opus_strerror(frameSize) << "\n";
// //             continue;
// //         }

// //         snd_pcm_writei(handle, decodedPcm.data(), frameSize);
// //     }

// //     close(sock);
// //     snd_pcm_close(handle);
// //     opus_decoder_destroy(decoder);
// // }


// void audioReceiver() {
//     //Creates the UDP Socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(AUDIO_PORT);
//     addr.sin_addr.s_addr = INADDR_ANY;
//     bind(sock, (sockaddr*)&addr, sizeof(addr));

//     //Configured ALSA
//     snd_pcm_t *handle;
//     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
//     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
//                        CHANNELS, SAMPLE_RATE, 1, 500000);
    
//     // Intialize Opus Decoder
//     int err;
//     OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
//     if (err != OPUS_OK) {
//         std::cerr << "Failed to create Opus decoder: " << opus_strerror(err) << "\n";
//         return;
//     }
//     std::cout << "Creating Opus decoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";

//     //Allocate the buffers
//     std::vector<uint8_t> buffer(BUFFER_SIZE);
//     std::vector<opus_int16> decodedPcm(2048 * CHANNELS);

//     while (true) {
//         ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//         if (len < 12) continue;

//         std::cout << "[Audio] Received packet: size=" <<len<<"\n";
//         unsigned char* opusPayload = buffer.data() + 12; // Fixed: Use correct RTP header size
//         int frameSize = opus_decode(decoder, opusPayload, len - 12, decodedPcm.data(), 2048, 0);
//         if (frameSize < 0) {
//             std::cerr << "Opus decode error: " << opus_strerror(frameSize) << "\n";
//             continue;
//         }
//         std::cout << "[Audio] Decoded PCM: frameSize=" << frameSize << "\n";
//         if (snd_pcm_writei(handle, decodedPcm.data(), frameSize) < 0) {
//             std::cerr << "ALSA playback error\n";
//             snd_pcm_recover(handle, -1, 1); // Attempt recovery
//         }
//     }

//     close(sock);
//     snd_pcm_close(handle);
//     opus_decoder_destroy(decoder);
// }

// // AUDIO SENDER — for Two-Way Audio
// void audioStreaming()
// {
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in server{};
//     server.sin_family = AF_INET;
//     server.sin_port = htons(AUDIO_PORT + 10); // Send to server's new port 5012
//     server.sin_addr.s_addr = inet_addr("127.0.0.1"); // or server IP

//     snd_pcm_t *handle;
//     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
//     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
//                        CHANNELS, SAMPLE_RATE, 1, 500000);

//     int err;
//     OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_AUDIO, &err);
//     if (err != OPUS_OK)
//     {
//         std::cerr << "Opus encoder create error: " << opus_strerror(err) << "\n";
//         return;
//     }
//     std::vector<int16_t> buffer(960 * CHANNELS);
//     unsigned char opusData[4000];
//     uint16_t seq = 0;
//     uint32_t ts = 0;

//     while (true)
//     {
//         snd_pcm_readi(handle, buffer.data(), 960);
//         std::cout << "[Audio] Captured 960 samples from microphone\n";

//         int encodedBytes = opus_encode(encoder, buffer.data(), 960, opusData, sizeof(opusData));
//         if (encodedBytes < 0)
//         {
//             std::cerr << "Opus encode error: " << opus_strerror(encodedBytes) << "\n";
//             continue;
//         }

//         std::vector<uint8_t> packet(RTP_HEADER_SIZE + encodedBytes);
//         RTPHeader hdr;
//         hdr.version = 2;
//         hdr.padding = 0;
//         hdr.extension = 0;
//         hdr.csrcCount = 0;
//         hdr.marker = 0;
//         hdr.payloadType = 97;
//         hdr.sequenceNumber = htons(seq++);
//         hdr.timestamp = htonl(ts);
//         hdr.ssrc = htonl(1234);

//         memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
//         memcpy(packet.data() + RTP_HEADER_SIZE, opusData, encodedBytes);
//         sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&server, sizeof(server));
//         ts += 960;
//         std::cout << "[Audio] Sent packet: size=" << packet.size() << " bytes\n";

//     }

//     close(sock);
//     snd_pcm_close(handle);
//     opus_encoder_destroy(encoder);
// }


// // Main: run both threads
// int main() {
//     std::thread videoThread(videoReceiver);
//     std::thread audioThread(audioReceiver);
//     std::thread audioSenderThread(audioStreaming);
//     videoThread.join();
//     audioThread.join();
//     audioSenderThread.join();
//     return 0;
// }










// /*
// g++ client.cpp -o client `pkg-config --cflags --libs opencv4` -lopus -lasound -lpthread
// */

// // //USING THE CLIENT AS THE FFPLAY

// // //ffplay -v trace -i udp://127.0.0.1:5000

// //ffplay -loglevel debug -protocol_whitelist file,udp,rtp -i audio.sdp


// #include <iostream>
// #include <opencv2/opencv.hpp>
// #include <cstring>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <thread>
// #include <vector>
// #include <arpa/inet.h>
// #include <alsa/asoundlib.h>
// #include <opus/opus.h>  // Opus decoder

// #define VIDEO_PORT 5000
// #define AUDIO_PORT 5002
// #define BUFFER_SIZE 65536
// #define AUDIO_BUFFER_SIZE 4096
// #define AUDIO_DEVICE "default"
// #define SAMPLE_RATE 48000
// #define CHANNELS 1
// #define RTP_HEADER_SIZE 12  
// #define MJPEG_HEADER_SIZE 8 // ADDED: MJPEG header size per RFC 2435

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

// // ADDED: MJPEG header structure
// struct MJPEGHeader {
//     uint8_t type_specific;
//     uint8_t fragment_offset[3]; // 24-bit offset
//     uint8_t type;
//     uint8_t q;
//     uint8_t width;
//     uint8_t height;
// };

// // VIDEO RECEIVER 
// void videoReceiver() {
//     //Creates the UDP Socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) { // ADDED: Check socket creation
//         perror("Socket creation failed");
//         return;
//     }
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(VIDEO_PORT);
//     addr.sin_addr.s_addr = INADDR_ANY;
//     if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { // ADDED: Check bind
//         perror("Bind failed");
//         close(sock);
//         return;
//     }

//     // ADDED: Buffer to reassemble JPEG frame
//     std::vector<uint8_t> jpegBuffer;
//     uint32_t lastTimestamp = 0;

//     //Receive and Decode the video stream
//     std::vector<uint8_t> buffer(BUFFER_SIZE);
//     while (true) {
//         ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//         if (len < RTP_HEADER_SIZE + MJPEG_HEADER_SIZE) { // CHANGED: Check for RTP + MJPEG headers
//             std::cerr << "[Video] Packet too small: " << len << " bytes\n";
//             continue;
//         }

//         // ADDED: Parse RTP header
//         RTPHeader* hdr = reinterpret_cast<RTPHeader*>(buffer.data());
//         uint16_t seq = ntohs(hdr->sequenceNumber);
//         uint32_t timestamp = ntohl(hdr->timestamp);
//         bool marker = hdr->marker;
//         uint8_t payloadType = hdr->payloadType;

//         if (payloadType != 26) { // ADDED: Check for MJPEG payload type
//             std::cerr << "[Video] Unexpected payload type: " << (int)payloadType << "\n";
//             continue;
//         }

//         // ADDED: Parse MJPEG header
//         MJPEGHeader* mjpegHdr = reinterpret_cast<MJPEGHeader*>(buffer.data() + RTP_HEADER_SIZE);
//         uint32_t offset = (mjpegHdr->fragment_offset[0] << 16) |
//                           (mjpegHdr->fragment_offset[1] << 8) |
//                           mjpegHdr->fragment_offset[2];

//         // ADDED: Extract JPEG payload
//         size_t payloadSize = len - RTP_HEADER_SIZE - MJPEG_HEADER_SIZE;
//         uint8_t* payload = buffer.data() + RTP_HEADER_SIZE + MJPEG_HEADER_SIZE;

//         std::cout << "[Video] Received packet: seq=" << seq << ", timestamp=" << timestamp
//                   << ", offset=" << offset << ", size=" << payloadSize << ", marker=" << marker << "\n";

//         // ADDED: Reassemble JPEG frame
//         if (timestamp != lastTimestamp && !jpegBuffer.empty()) {
//             // New frame started, but old frame incomplete; discard
//             std::cerr << "[Video] Incomplete frame discarded\n";
//             jpegBuffer.clear();
//         }

//         if (offset == 0) { // Start of new frame
//             jpegBuffer.clear();
//         }

//         // Append payload to buffer
//         jpegBuffer.insert(jpegBuffer.end(), payload, payload + payloadSize);
//         lastTimestamp = timestamp;

//         // Process frame when marker bit is set (end of frame)
//         if (marker) {
//             cv::Mat frame = cv::imdecode(jpegBuffer, cv::IMREAD_COLOR);
//             if (frame.empty()) {
//                 std::cerr << "[Video] Failed to decode JPEG frame: size=" << jpegBuffer.size() << "\n";
//             } else {
//                 cv::imshow("Video", frame);
//                 std::cout << "[Video] Decoded and displayed frame: size=" << jpegBuffer.size() << "\n";
//                 if (cv::waitKey(1) == 27) break; // ESC to exit
//             }
//             jpegBuffer.clear();
//         }
//     }

//     // REMOVED: Original processing code
//     // std::vector<uint8_t> buffer(BUFFER_SIZE);
//     // while (true) {
//     //     ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//     //     if (len < 12) continue;
//     //     std::cout<<"[Video] Received frame: size=" << len << " bytes\n";
//     //     std::vector<uchar> jpeg(buffer.begin() + 12, buffer.begin() + len);
//     //     cv::Mat frame = cv::imdecode(jpeg, cv::IMREAD_COLOR);
//     //     if (frame.empty()) {
//     //         std::cerr << "Failed to decode JPEG frame\n";
//     //     } else {
//     //         cv::imshow("Video", frame);
//     //         std::cout<<"[Video] Decoded and displayed frame\n";
//     //         if (cv::waitKey(1) == 27) break;  // ESC to exit
//     //     }
//     // }

//     close(sock);
//     cv::destroyAllWindows(); // ADDED: Clean up OpenCV windows
// }

// // AUDIO RECEIVER 
// void audioReceiver() {
//     //Creates the UDP Socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(AUDIO_PORT);
//     addr.sin_addr.s_addr = INADDR_ANY;
//     bind(sock, (sockaddr*)&addr, sizeof(addr));

//     //Configured ALSA
//     snd_pcm_t *handle;
//     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
//     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
//                        CHANNELS, SAMPLE_RATE, 1, 500000);
    
//     // Intialize Opus Decoder
//     int err;
//     OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
//     if (err != OPUS_OK) {
//         std::cerr << "Failed to create Opus decoder: " << opus_strerror(err) << "\n";
//         return;
//     }
//     std::cout << "Creating Opus decoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";

//     //Allocate the buffers
//     std::vector<uint8_t> buffer(BUFFER_SIZE);
//     std::vector<opus_int16> decodedPcm(2048 * CHANNELS);

//     while (true) {
//         ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
//         if (len < 12) continue;

//         std::cout << "[Audio] Received packet: size=" <<len<<"\n";
//         unsigned char* opusPayload = buffer.data() + 12; // Fixed: Use correct RTP header size
//         int frameSize = opus_decode(decoder, opusPayload, len - 12, decodedPcm.data(), 2048, 0);
//         if (frameSize < 0) {
//             std::cerr << "Opus decode error: " << opus_strerror(frameSize) << "\n";
//             continue;
//         }
//         std::cout << "[Audio] Decoded PCM: frameSize=" << frameSize << "\n";
//         if (snd_pcm_writei(handle, decodedPcm.data(), frameSize) < 0) {
//             std::cerr << "ALSA playback error\n";
//             snd_pcm_recover(handle, -1, 1); // Attempt recovery
//         }
//     }

//     close(sock);
//     snd_pcm_close(handle);
//     opus_decoder_destroy(decoder);
// }

// // AUDIO SENDER — for Two-Way Audio
// void audioStreaming()
// {
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr_in server{};
//     server.sin_family = AF_INET;
//     server.sin_port = htons(AUDIO_PORT + 10); // Send to server's new port 5012
//     server.sin_addr.s_addr = inet_addr("127.0.0.1"); // or server IP

//     snd_pcm_t *handle;
//     snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
//     snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
//                        CHANNELS, SAMPLE_RATE, 1, 500000);

//     int err;
//     OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_AUDIO, &err);
//     if (err != OPUS_OK)
//     {
//         std::cerr << "Opus encoder create error: " << opus_strerror(err) << "\n";
//         return;
//     }
//     std::vector<int16_t> buffer(960 * CHANNELS);
//     unsigned char opusData[4000];
//     uint16_t seq = 0;
//     uint32_t ts = 0;

//     while (true)
//     {
//         snd_pcm_readi(handle, buffer.data(), 960);
//         std::cout << "[Audio] Captured 960 samples from microphone\n";

//         int encodedBytes = opus_encode(encoder, buffer.data(), 960, opusData, sizeof(opusData));
//         if (encodedBytes < 0)
//         {
//             std::cerr << "Opus encode error: " << opus_strerror(encodedBytes) << "\n";
//             continue;
//         }

//         std::vector<uint8_t> packet(RTP_HEADER_SIZE + encodedBytes);
//         RTPHeader hdr;
//         hdr.version = 2;
//         hdr.padding = 0;
//         hdr.extension = 0;
//         hdr.csrcCount = 0;
//         hdr.marker = 0;
//         hdr.payloadType = 97;
//         hdr.sequenceNumber = htons(seq++);
//         hdr.timestamp = htonl(ts);
//         hdr.ssrc = htonl(1234);

//         memcpy(packet.data(), &hdr, RTP_HEADER_SIZE);
//         memcpy(packet.data() + RTP_HEADER_SIZE, opusData, encodedBytes);
//         sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&server, sizeof(server));
//         ts += 960;
//         std::cout << "[Audio] Sent packet: size=" << packet.size() << " bytes\n";

//     }

//     close(sock);
//     snd_pcm_close(handle);
//     opus_encoder_destroy(encoder);
// }

// // Main: run both threads
// int main() {
//     std::thread videoThread(videoReceiver);
//     std::thread audioThread(audioReceiver);
//     std::thread audioSenderThread(audioStreaming);
//     videoThread.join();
//     audioThread.join();
//     audioSenderThread.join();
//     return 0;
// }

// /*
// g++ client.cpp -o client `pkg-config --cflags --libs opencv4` -lopus -lasound -lpthread
// */


#include <iostream>
#include <opencv2/opencv.hpp>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <alsa/asoundlib.h>
#include <opus/opus.h>

#define VIDEO_PORT 5000
#define AUDIO_PORT 5002
#define BUFFER_SIZE 65536
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_DEVICE "default"
#define SAMPLE_RATE 48000
#define CHANNELS 1
#define RTP_HEADER_SIZE 12
#define MJPEG_HEADER_SIZE 8

struct MJPEGHeader {
    uint8_t type_specific;
    uint8_t fragment_offset[3];
    uint8_t type;
    uint8_t q;
    uint8_t width;
    uint8_t height;
};

// CHANGED: Added function to create RTP header for outgoing packets
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

// CHANGED: Function to parse RTP header from incoming packets
void parseRTPHeader(const uint8_t* header, uint8_t &payloadType, bool &marker, uint16_t &sequenceNumber, uint32_t &timestamp) {
    payloadType = header[1] & 0x7F; // Lower 7 bits
    marker = (header[1] & 0x80) != 0; // Marker bit
    sequenceNumber = (header[2] << 8) | header[3]; // Big-endian
    timestamp = (header[4] << 24) | (header[5] << 16) | (header[6] << 8) | header[7]; // Big-endian
}

// CHANGED: Function to log raw RTP header bytes for debugging
void logRTPHeader(const uint8_t* header) {
    std::cerr << "[Video] Raw RTP header: ";
    for (int i = 0; i < RTP_HEADER_SIZE; ++i) {
        std::cerr << std::hex << (int)header[i] << " ";
    }
    std::cerr << std::dec << "\n";
}

void videoReceiver() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(VIDEO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return;
    }
    std::cout << "[Video] Bound to port " << VIDEO_PORT << "\n";

    std::vector<uint8_t> jpegBuffer;
    uint32_t lastTimestamp = 0;
    uint16_t lastSeq = 65535; // Initialize to detect sequence gaps

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    while (true) {
        ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (len < RTP_HEADER_SIZE + MJPEG_HEADER_SIZE) {
            std::cerr << "[Video] Packet too small: " << len << " bytes\n";
            continue;
        }

        // Parse RTP header
        uint8_t payloadType;
        bool marker;
        uint16_t seq;
        uint32_t timestamp;
        parseRTPHeader(buffer.data(), payloadType, marker, seq, timestamp);

        // Log raw header for debugging
        logRTPHeader(buffer.data());

        if (payloadType != 26) {
            std::cerr << "[Video] Unexpected payload type: " << (int)payloadType << "\n";
            continue;
        }

        // Check for sequence number gaps
        if (lastSeq != 65535 && seq != (lastSeq + 1)) {
            std::cerr << "[Video] Sequence gap detected: expected " << (lastSeq + 1) << ", got " << seq << "\n";
        }
        lastSeq = seq;

        MJPEGHeader* mjpegHdr = reinterpret_cast<MJPEGHeader*>(buffer.data() + RTP_HEADER_SIZE);
        uint32_t offset = (mjpegHdr->fragment_offset[0] << 16) |
                          (mjpegHdr->fragment_offset[1] << 8) |
                          mjpegHdr->fragment_offset[2];

        size_t payloadSize = len - RTP_HEADER_SIZE - MJPEG_HEADER_SIZE;
        uint8_t* payload = buffer.data() + RTP_HEADER_SIZE + MJPEG_HEADER_SIZE;

        std::cout << "[Video] Received packet: seq=" << seq << ", timestamp=" << timestamp
                  << ", offset=" << offset << ", size=" << payloadSize << ", marker=" << marker << "\n";

        if (timestamp != lastTimestamp && !jpegBuffer.empty()) {
            std::cerr << "[Video] Incomplete frame discarded: size=" << jpegBuffer.size() << "\n";
            jpegBuffer.clear();
        }

        if (offset == 0) {
            jpegBuffer.clear();
        }

        jpegBuffer.insert(jpegBuffer.end(), payload, payload + payloadSize);
        lastTimestamp = timestamp;

        if (marker) {
            cv::Mat frame = cv::imdecode(jpegBuffer, cv::IMREAD_COLOR);
            if (frame.empty()) {
                std::cerr << "[Video] Failed to decode JPEG frame: size=" << jpegBuffer.size() << "\n";
            } else {
                cv::imshow("Video", frame);
                std::cout << "[Video] Decoded and displayed frame: size=" << jpegBuffer.size() << "\n";
                if (cv::waitKey(1) == 27) break;
            }
            jpegBuffer.clear();
        }
    }

    close(sock);
    cv::destroyAllWindows();
}

void audioReceiver() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[Audio] Socket creation failed");
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(AUDIO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[Audio] Bind failed");
        close(sock);
        return;
    }

    snd_pcm_t *handle;
    if (snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        std::cerr << "[Audio] Failed to open ALSA playback device\n";
        close(sock);
        return;
    }
    snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                       CHANNELS, SAMPLE_RATE, 1, 500000);

    int err;
    OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
    if (err != OPUS_OK) {
        std::cerr << "[Audio] Failed to create Opus decoder: " << opus_strerror(err) << "\n";
        close(sock);
        snd_pcm_close(handle);
        return;
    }
    std::cout << "[Audio] Created Opus decoder: rate=" << SAMPLE_RATE << ", channels=" << CHANNELS << "\n";

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    std::vector<opus_int16> decodedPcm(2048 * CHANNELS);

    while (true) {
        ssize_t len = recvfrom(sock, buffer.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (len < RTP_HEADER_SIZE) continue;

        // Parse RTP header for audio
        uint8_t payloadType;
        bool marker;
        uint16_t seq;
        uint32_t timestamp;
        parseRTPHeader(buffer.data(), payloadType, marker, seq, timestamp);

        if (payloadType != 97) {
            std::cerr << "[Audio] Unexpected payload type: " << (int)payloadType << "\n";
            continue;
        }

        std::cout << "[Audio] Received packet: seq=" << seq << ", size=" << len << "\n";
        unsigned char* opusPayload = buffer.data() + RTP_HEADER_SIZE;
        int frameSize = opus_decode(decoder, opusPayload, len - RTP_HEADER_SIZE, decodedPcm.data(), 2048, 0);
        if (frameSize < 0) {
            std::cerr << "[Audio] Opus decode error: " << opus_strerror(frameSize) << "\n";
            continue;
        }
        std::cout << "[Audio] Decoded PCM: frameSize=" << frameSize << "\n";
        if (snd_pcm_writei(handle, decodedPcm.data(), frameSize) < 0) {
            std::cerr << "[Audio] ALSA playback error\n";
            snd_pcm_recover(handle, -1, 1);
        }
    }

    close(sock);
    snd_pcm_close(handle);
    opus_decoder_destroy(decoder);
}

void audioStreaming() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[AudioSender] Socket creation failed");
        return;
    }
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(AUDIO_PORT + 10);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    snd_pcm_t *handle;
    if (snd_pcm_open(&handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0) < 0) {
        std::cerr << "[AudioSender] Failed to open ALSA capture device\n";
        close(sock);
        return;
    }
    snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                       CHANNELS, SAMPLE_RATE, 1, 500000);

    int err;
    OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_AUDIO, &err);
    if (err != OPUS_OK) {
        std::cerr << "[AudioSender] Opus encoder create error: " << opus_strerror(err) << "\n";
        close(sock);
        snd_pcm_close(handle);
        return;
    }

    std::vector<int16_t> buffer(960 * CHANNELS);
    unsigned char opusData[4000];
    uint16_t seq = 0;
    uint32_t ts = 0;

    while (true) {
        if (snd_pcm_readi(handle, buffer.data(), 960) != 960) {
            std::cerr << "[AudioSender] Failed to capture 960 samples\n";
            continue;
        }
        std::cout << "[AudioSender] Captured 960 samples from microphone\n";

        int encodedBytes = opus_encode(encoder, buffer.data(), 960, opusData, sizeof(opusData));
        if (encodedBytes < 0) {
            std::cerr << "[AudioSender] Opus encode error: " << opus_strerror(encodedBytes) << "\n";
            continue;
        }

        std::vector<uint8_t> packet(RTP_HEADER_SIZE + encodedBytes);
        uint8_t rtpHdr[RTP_HEADER_SIZE];
        // CHANGED: Use createRTPHeader instead of parseRTPHeader
        createRTPHeader(rtpHdr, ts, seq++, 97);
        memcpy(packet.data(), rtpHdr, RTP_HEADER_SIZE);
        memcpy(packet.data() + RTP_HEADER_SIZE, opusData, encodedBytes);
        ssize_t sent = sendto(sock, packet.data(), packet.size(), 0, (sockaddr *)&server, sizeof(server));
        if (sent < 0) {
            std::cerr << "[AudioSender] Sendto failed: " << strerror(errno) << "\n";
        } else {
            std::cout << "[AudioSender] Sent packet: size=" << packet.size() << " bytes\n";
        }
        ts += 960;
    }

    close(sock);
    snd_pcm_close(handle);
    opus_encoder_destroy(encoder);
}

int main() {
    std::thread videoThread(videoReceiver);
    std::thread audioThread(audioReceiver);
    std::thread audioSenderThread(audioStreaming);
    videoThread.join();
    audioThread.join();
    audioSenderThread.join();
    return 0;
}

/*
g++ client.cpp -o client `pkg-config --cflags --libs opencv4` -lopus -lasound -lpthread
*/