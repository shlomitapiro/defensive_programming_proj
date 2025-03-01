#include "SocketWrapper.h"
#include <iostream>

SocketWrapper::SocketWrapper(const std::string& serverIp, int serverPort) : sock(INVALID_SOCKET) {
    // יצירת סוקט
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket!" << std::endl;
        return;
    }

    // הגדרת כתובת השרת והפורט
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    // התחברות לשרת
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server!" << std::endl;
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

SocketWrapper::~SocketWrapper() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
}

bool SocketWrapper::isValid() const {
    return sock != INVALID_SOCKET;
}

bool SocketWrapper::sendAll(const std::vector<uint8_t>& data) {
    int totalSent = 0;
    int dataSize = static_cast<int>(data.size());
    while (totalSent < dataSize) {
        int sent = send(sock, reinterpret_cast<const char*>(data.data() + totalSent), dataSize - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "Failed to send data!" << std::endl;
            return false;
        }
        totalSent += sent;
    }
    return true;
}

std::vector<uint8_t> SocketWrapper::receiveAll(size_t bufferSize) {
    std::vector<uint8_t> response(bufferSize);
    int bytesRead = recv(sock, reinterpret_cast<char*>(response.data()), static_cast<int>(bufferSize), 0);
    if (bytesRead <= 0) {
        std::cerr << "No response received or error occurred!" << std::endl;
        return std::vector<uint8_t>();
    }
    response.resize(bytesRead);
    return response;
}

void SocketWrapper::closeSocket() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}
