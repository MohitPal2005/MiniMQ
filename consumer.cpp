#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to broker.\n";
        return 1;
    }

    std::string topic;
    std::cout << "Enter topic to subscribe to: ";
    std::cin >> topic;

    // Format: "SUB topic"
    std::string payload = "SUB " + topic;
    send(sock, payload.c_str(), payload.length(), 0);
    
    std::cout << "Subscribed to '" << topic << "'. Waiting for messages...\n";

    // Loop forever waiting for messages from the broker
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            std::cout << "Disconnected from broker.\n";
            break;
        }
        
        std::cout << "[NEW MESSAGE]: " << buffer;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}