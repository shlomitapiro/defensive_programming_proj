#include "SocketWrapper.h"
#include <iostream>

/*SocketWrapper::SocketWrapper(const std::string& serverIp, int serverPort) : sock(INVALID_SOCKET) {
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
}*/

SocketWrapper::SocketWrapper(const std::string& serverIp, int serverPort) : sock(INVALID_SOCKET) {
    // יצירת הסוקט
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create socket!");
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        closesocket(sock);
        throw std::runtime_error("Invalid server IP address: " + serverIp);
    }

    // התחברות לשרת
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
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

/*bool SocketWrapper::sendAll(const std::vector<uint8_t>& data) {
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
}*/

bool SocketWrapper::sendAll(const std::vector<uint8_t>& data) {
    int totalSent = 0;
    int dataSize = static_cast<int>(data.size());
    while (totalSent < dataSize) {
        int sent = send(sock, reinterpret_cast<const char*>(data.data() + totalSent), dataSize - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send data: " + std::to_string(WSAGetLastError()));
        }
        totalSent += sent;
    }
    return true;
}

/*std::vector<uint8_t> SocketWrapper::receiveAll() const
{
    std::vector<uint8_t> response;
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Socket is invalid!" << std::endl;
        return response;
    }

    const size_t CHUNK_SIZE = 1024;
    char buffer[CHUNK_SIZE];

    while (true)
    {
        int bytesRead = recv(sock, buffer, CHUNK_SIZE, 0);
        if (bytesRead == 0)
        {
            // השרת סגר את החיבור בצורה מסודרת
            break;
        }
        if (bytesRead < 0)
        {
            // שגיאה
            std::cerr << "Error in recv!" << std::endl;
            response.clear(); // במקרה שרוצים לסמן שגיאה
            break;
        }
        // הוספת מה שקראנו למערך התגובות
        response.insert(response.end(), buffer, buffer + bytesRead);

        // אם אין עוד מידע כרגע ב-buffer, יתכן שנרצה לבדוק עם select/timeout וכו'.
        // כרגע, לצורך פשטות, נמשיך לנסות לקרוא - ואם אין עוד מידע, recv תחזיר 0 או שגיאה
    }

    return response;
}*/

std::vector<uint8_t> SocketWrapper::receiveAll() const {
    std::vector<uint8_t> response;
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Socket is invalid!");
    }

    const size_t CHUNK_SIZE = 1024;
    char buffer[CHUNK_SIZE];

    while (true) {
        int bytesRead = recv(sock, buffer, CHUNK_SIZE, 0);
        if (bytesRead == 0) {
            // סגירה תקינה של החיבור
            break;
        }
        if (bytesRead < 0) {
            throw std::runtime_error("Error in recv: " + std::to_string(WSAGetLastError()));
        }
        response.insert(response.end(), buffer, buffer + bytesRead);
    }

    return response;
}

void SocketWrapper::closeSocket() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}
