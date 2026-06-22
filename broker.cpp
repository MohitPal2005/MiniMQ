#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <thread>
#include <sstream>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ThreadSafeQueue.h"

#pragma comment(lib, "ws2_32.lib")

// UPGRADE: Maps a topic to a VECTOR of subscriber queues.
std::unordered_map<std::string, std::vector<std::shared_ptr<ThreadSafeQueue>>> topics;
std::mutex topics_mutex;

void handle_client(SOCKET client_socket) {
    char buffer[1024] = {0};
    
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return;
    }

    std::string request(buffer);
    std::stringstream ss(request);
    std::string command, topic;
    ss >> command >> topic;

    if (command == "PUB") {
        std::string message;
        std::getline(ss, message); 
        
        // UPGRADE: Lock the map, and push the message to EVERY subscriber's queue
        std::lock_guard<std::mutex> lock(topics_mutex);
        for (auto& subscriber_queue : topics[topic]) {
            subscriber_queue->push(message);
        }
        
        std::cout << "[BROKER] Broadcasted to '" << topic << "':" << message << "\n";
        closesocket(client_socket); 
    } 
    else if (command == "SUB") {
        std::cout << "[BROKER] New client subscribed to '" << topic << "'\n";
        
        // UPGRADE: Create a dedicated queue for this specific client
        auto my_queue = std::make_shared<ThreadSafeQueue>();
        
        { // Scope the lock so we don't hold it while waiting for messages!
            std::lock_guard<std::mutex> lock(topics_mutex);
            topics[topic].push_back(my_queue);
        }

        while (true) {
            std::string msg = my_queue->wait_and_pop(); 
            msg += "\n";
            
            int send_result = send(client_socket, msg.c_str(), msg.length(), 0);
            if (send_result == SOCKET_ERROR) {
                std::cout << "[BROKER] Subscriber disconnected. Cleaning up...\n";
                break; 
            }
        }

        // UPGRADE: When a client disconnects, safely remove their queue from the topic
        {
            std::lock_guard<std::mutex> lock(topics_mutex);
            auto& subs = topics[topic];
            subs.erase(std::remove(subs.begin(), subs.end(), my_queue), subs.end());
        }
        closesocket(client_socket);
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, SOMAXCONN);
    std::cout << "MiniMQ Broker is running on port 8080...\n";

    while (true) {
        sockaddr_in client_addr;
        int client_size = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_size);

        if (client_socket != INVALID_SOCKET) {
            std::thread(handle_client, client_socket).detach();
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}