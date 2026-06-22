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

    std::string topic, message;
    std::cout << "Enter topic to publish to: ";
    std::cin >> topic;
    std::cin.ignore(); // clear the newline from buffer
    
    std::cout << "Enter message: ";
    std::getline(std::cin, message);

    // Format: "PUB topic message"
    std::string payload = "PUB " + topic + " " + message;
    send(sock, payload.c_str(), payload.length(), 0);
    
    std::cout << "Message sent!\n";

    closesocket(sock);
    WSACleanup();
    return 0;
}