#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const std::string DEFAULT_FILE = "index.html";
const int PORT = 8080;
const int BUFFER_SIZE = 1024;

void handle_client(SOCKET client_socket) {
    try {
        char buffer[BUFFER_SIZE];
        int received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (received <= 0) {
            closesocket(client_socket);
            return;
        }
        buffer[received] = '\0';

        // Log the request
        std::string request(buffer);
        std::cout << "Request received:\n" << request << std::endl;

        // Check if the file exists
        std::ifstream file(DEFAULT_FILE);
        if (!file) {
            // Respond with 404 Not Found if the file is missing
            std::string response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 13\r\n\r\n"
                "404 Not Found";
            send(client_socket, response.c_str(), response.size(), 0);
            closesocket(client_socket);
            return;
        }

        // Read the file content
        std::stringstream file_content;
        file_content << file.rdbuf();
        std::string body = file_content.str();

        // Prepare the HTTP response
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << body.size() << "\r\n\r\n"
            << body;

        // Send the response
        send(client_socket, response.str().c_str(), response.str().size(), 0);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << std::endl;
    }

    // Close the client socket
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind to port " << PORT << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket." << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is running at http://127.0.0.1:" << PORT << std::endl;

    while (true) {
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        // Handle the client in a new thread
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
