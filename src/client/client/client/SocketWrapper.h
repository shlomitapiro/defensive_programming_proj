#pragma once

#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class SocketWrapper {
public:
    // בונה את הסוקט ומבצע התחברות לכתובת ולפורט הנתונים
    SocketWrapper(const std::string& serverIp, int serverPort);
    ~SocketWrapper();

    // בודק אם הסוקט תקין (התחברות הצליחה)
    bool isValid() const;

    // שולח את כל הנתונים שבוקט הנתון ומוודא שליחה מלאה
    bool sendAll(const std::vector<uint8_t>& data);

    // מקבל נתונים מהסוקט (עם גודל ברירת מחדל של 1024 בתים)
    std::vector<uint8_t> receiveAll(size_t bufferSize = 1024);

    // סוגר את הסוקט במידה ונדרש
    void closeSocket();

private:
    SOCKET sock;
};

