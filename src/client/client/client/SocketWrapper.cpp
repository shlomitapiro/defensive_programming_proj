#include "SocketWrapper.h"
#include "utils.h"


// Constructor: creates a TCP socket and connects to the server at the specified IP and port.
SocketWrapper::SocketWrapper(const std::string& serverIp, unsigned short serverPort) : sock(INVALID_SOCKET) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create socket!");
    }

    // Set up the server address structure.
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    // Convert the server IP from string to binary form.
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        closesocket(sock);
        throw std::runtime_error("Invalid server IP address: " + serverIp);
    }

    // Connect to the server.
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        throw std::runtime_error("Failed to connect to server " + serverIp + ":" + std::to_string(serverPort));
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

// Sends all data from the provided vector, ensuring the entire payload is transmitted.
bool SocketWrapper::sendAll(const std::vector<uint8_t>& data) const{
    int totalSent = 0;
    int dataSize = static_cast<int>(data.size());

    // Loop until all bytes are sent.
    while (totalSent < dataSize) {
        int sent = send(sock, reinterpret_cast<const char*>(data.data() + totalSent), dataSize - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send data: " + std::to_string(WSAGetLastError()));
        }
        totalSent += sent;
    }
    return true;
}

// Receives data from the socket in chunks and returns the complete response.
std::vector<uint8_t> SocketWrapper::receiveAll() const {
    std::vector<uint8_t> response;
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Socket is invalid!");
    }

    const size_t CHUNK_SIZE = 1024;
    char buffer[CHUNK_SIZE];

    // Read data until the server closes the connection.
    while (true) {
        int bytesRead = recv(sock, buffer, CHUNK_SIZE, 0);
        if (bytesRead == 0) {
            // The server closed the connection gracefully.
            break;
        }
        if (bytesRead < 0) {
            throw std::runtime_error("Error in recv: " + std::to_string(WSAGetLastError()));
        }
        if (bytesRead > CHUNK_SIZE) {
            throw std::runtime_error("recv returned too large value");
        }

        response.insert(response.end(), buffer, buffer + bytesRead);
    }

    return response;
}

// Closes the socket and marks it as invalid.
void SocketWrapper::closeSocket() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}