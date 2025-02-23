#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>  // Include the nlohmann json library

// Constants
constexpr int SERVER_PORT = 3000;
constexpr int PACKET_SIZE = 17;
constexpr int BUFFER_SIZE = 4096;
constexpr int MAX_RETRIES = 3;
constexpr int CONNECTION_TIMEOUT_MS = 500;

// Structure to hold packet information
struct alignas(64) Packet {
    std::string symbol;
    char indicator;
    uint32_t quantity;
    uint32_t price;
    uint32_t sequence;
};

// Packed structure for direct network parsing
#pragma pack(push, 1)
struct PacketData {
    char symbol[4];
    char indicator;
    uint32_t quantity;
    uint32_t price;
    uint32_t sequence;
};
#pragma pack(pop)

// Function to bind client to a specific CPU core
void setCpuAffinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

// Establishes a reliable connection
int createConnection(const std::string &host, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    struct sockaddr_in serv_addr{}; 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr);
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

// Streams all packets from the server
std::map<uint32_t, Packet> streamAllPackets(int sockfd, std::vector<uint32_t>& missingPackets) {
    std::map<uint32_t, Packet> packets;
    missingPackets.clear();
    
    uint8_t callType = 1;
    send(sockfd, &callType, sizeof(callType), 0);

    alignas(64) uint8_t buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    uint32_t lastSequence = 0;

    while ((bytesRead = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        size_t numPackets = bytesRead / PACKET_SIZE;
        for (size_t i = 0; i < numPackets; i++) {
            PacketData* pktData = reinterpret_cast<PacketData*>(&buffer[i * PACKET_SIZE]);
            Packet pkt{ std::string(pktData->symbol, 4), pktData->indicator, ntohl(pktData->quantity), ntohl(pktData->price), ntohl(pktData->sequence) };
            packets[pkt.sequence] = pkt;
            if (lastSequence && pkt.sequence > lastSequence + 1) {
                for (uint32_t seq = lastSequence + 1; seq < pkt.sequence; ++seq) {
                    missingPackets.push_back(seq);
                }
            }
            lastSequence = pkt.sequence;
        }
    }
    return packets;
}

// Resends missing packets
Packet resendPacket(int sockfd, uint8_t seq) {
    Packet pkt;
    uint8_t payload[2] = {2, seq};
    send(sockfd, payload, sizeof(payload), 0);
    uint8_t packetData[PACKET_SIZE];
    recv(sockfd, packetData, PACKET_SIZE, 0);
    PacketData* pktData = reinterpret_cast<PacketData*>(packetData);
    return { std::string(pktData->symbol, 4), pktData->indicator, ntohl(pktData->quantity), ntohl(pktData->price), ntohl(pktData->sequence) };
}

// Function to save packets to JSON
void saveToJson(const std::map<uint32_t, Packet>& packets) {
    nlohmann::json jsonOutput;

    // Convert the packets map into a JSON array
    for (const auto& [seq, pkt] : packets) {
        jsonOutput["order_book"].push_back({
            {"sequence", pkt.sequence},
            {"symbol", pkt.symbol},
            {"indicator", std::string(1, pkt.indicator)},
            {"quantity", pkt.quantity},
            {"price", pkt.price}
        });
    }

    // Write the JSON to a file
    std::ofstream outputFile("output.json");
    if (outputFile.is_open()) {
        outputFile << jsonOutput.dump(4);  // Pretty print with indentation of 4 spaces
        outputFile.close();
        std::cout << "Order book saved to output.json\n";
    } else {
        std::cerr << "Error: Unable to open output.json for writing.\n";
    }
}

void displayMenu() {
    std::cout << "\n=== ABX Mock Exchange Client ===\n";
    std::cout << "1. Stream All Packets (Retrieve Full Order Book)\n";
    std::cout << "2. Resend Specific Packet (Request Missing Data)\n";
    std::cout << "3. Exit\n";
    std::cout << "Enter your choice: ";
}

int main() {
    setCpuAffinity(1);
    std::string serverHost = "127.0.0.1";
    std::map<uint32_t, Packet> packets;
    std::vector<uint32_t> missingPackets;
    
    while (true) {
        displayMenu();
        int choice;
        std::cin >> choice;
        
        if (choice == 3) {
            std::cout << "Exiting client.\n";
            break;
        }
        
        int sockfd = createConnection(serverHost, SERVER_PORT);
        if (sockfd < 0) continue;
        
        if (choice == 1) {
            packets = streamAllPackets(sockfd, missingPackets);
            close(sockfd);

            // Save the order book to output.json
            saveToJson(packets);
            
            // Print missing packets to terminal
    if (!missingPackets.empty()) {
        std::cout << "Missing packets:\n";
        for (const auto& seq : missingPackets) {
            std::cout << "Sequence: " << seq << "\n";
        }
    } else {
        std::cout << "No missing packets.\n";
    }
        } else if (choice == 2 && !missingPackets.empty()) {
            uint32_t seq;
    std::cout << "Enter missing sequence number to request: ";
    std::cin >> seq;

    if (std::find(missingPackets.begin(), missingPackets.end(), seq) != missingPackets.end()) {
        // Resend the missing packet
        Packet missingPkt = resendPacket(sockfd, seq);
        close(sockfd);

        // Add the retrieved packet to the order book
        packets[seq] = missingPkt;

        // Remove from missing packets list
        missingPackets.erase(std::remove(missingPackets.begin(), missingPackets.end(), seq), missingPackets.end());

        // Save the updated order book to output.json
        saveToJson(packets);

        // Print updated order book to terminal
        std::cout << "Updated order book after adding missing packet:\n";
        for (const auto& [seq, pkt] : packets) {
            std::cout << "Sequence: " << pkt.sequence << ", Symbol: " << pkt.symbol
                      << ", Indicator: " << pkt.indicator << ", Quantity: " << pkt.quantity
                      << ", Price: " << pkt.price << "\n";
        }
    } else {
        std::cout << "Invalid sequence number.\n";
    }
    close(sockfd);
        }
    }
    return 0;
}
