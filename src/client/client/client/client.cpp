#include "Client.h"
#include "Base64Wrapper.h"
#include "SocketWrapper.h"  // יש לכלול את מחלקת SocketWrapper
#include "Protocol.h"
#include "utils.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstring>
#include <tuple>
#include <algorithm>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

Client::Client()
    : _rsaPrivate() {

    serverInfo = readServerInfo();
    std::string serverIp = std::get<0>(serverInfo);
    int serverPort = std::get<1>(serverInfo);

    _serverIp = serverIp;
    _serverPort = serverPort;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

Client::~Client() {
    WSACleanup();
}

std::tuple<std::string, unsigned short> Client::readServerInfo() {
    // מקבל את הנתיב לתיקייה שבה נמצא הקובץ exe
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));

    std::string serverFilePath = exeDir + "\\server.info";
    std::ifstream serverFile(serverFilePath);

    std::string serverIp;
    unsigned short serverPort;

    if (serverFile.is_open()) {
        // קורא שורת טקסט אחת
        std::string line;
        if (std::getline(serverFile, line)) {
            // מחפש את התו ':'
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                // חלק לפני הנקודתיים
                serverIp = line.substr(0, pos);
                // חלק אחרי הנקודתיים
                std::string portStr = line.substr(pos + 1);
                try {
                    serverPort = static_cast<unsigned short>(std::stoi(portStr));
                }
                catch (...) {
                    std::cerr << "⚠ Warning: invalid port in server.info, using default 1234.\n";
                    serverPort = 1234;
                }
            }
            else {
                std::cerr << "⚠ Warning: server.info missing ':' separator, using default 127.0.0.1:1234.\n";
            }
        }
        serverFile.close();
    }
    else {
        std::cerr << "⚠ Warning: Could not open " << serverFilePath
            << "! Using default 127.0.0.1:1234" << std::endl;
    }

    std::cout << "Server IP: " << serverIp << ", Port: " << serverPort << std::endl;
    return { serverIp, serverPort };
}


// -------------------
// פונקציות עזר לרישום
// -------------------

std::vector<uint8_t> Client::buildRegistrationPayload(const std::string& username) {
    // קבלת המפתח הציבורי (ב-base64) מה-RSAPrivateWrapper
    std::string pubKeyBase64 = _rsaPrivate.getPublicKey();
    // פענוח המפתח הציבורי לבינארי
    std::string pubKeyBin = Base64Wrapper::decode(pubKeyBase64);

    // ודא שאורך המפתח הציבורי לא עולה על 160 בתים – אם עולה, קצץ אותו.
    if (pubKeyBin.size() > 160) {
        pubKeyBin = pubKeyBin.substr(0, 160);
    }
    // אם הוא קצר, אפשר להשאיר אותו כפי שהוא – או למלא, תלוי בדרישה המדויקת.

    // יצירת buffer עבור שם המשתמש בגודל 255 בתים
    std::vector<uint8_t> nameBuffer(255, '\0');  // אתחול ב-'\0'

    // נעתיק את התווים של username עד לאורכו או עד 254 תווים
    size_t copyLen = min(username.size(), size_t(254));
    for (size_t i = 0; i < copyLen; i++) {
        nameBuffer[i] = static_cast<uint8_t>(username[i]);
    }
    // ודא שהתו האחרון (בתא 254) הוא '\0' – במקרה זה כבר מאופס

    // איחוד ה-buffers: 255 בתים לשם + 160 בתים למפתח הציבורי
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), nameBuffer.begin(), nameBuffer.end());
    payload.insert(payload.end(), pubKeyBin.begin(), pubKeyBin.end());

    // ודא שה-payload הכולל הוא 415 בתים (255+160)
    if (payload.size() > 415) {
        std::cerr << "Error: Registration payload size is incorrect: " << payload.size() << " bytes." << std::endl;
        return {};
    }
    return payload;
}


// מעדכן את _clientId מתוך תגובת השרת (16 בתים ראשונים)
bool Client::updateClientIdFromResponse(const std::vector<uint8_t>& response) {
    if (response.size() < 16) {
        return false;
    }
    _clientId = std::string(response.begin(), response.begin() + 16);
    std::cout << "Registration successful. New client ID: " << _clientId << std::endl;
    return true;
}

// כותב את פרטי הרישום לקובץ my.info
bool Client::writeRegistrationInfoToFile(const std::string& username) {

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));

    std::string meInfoFilePath = exeDir + "\\me.info";

    // שלב 3: אם הקובץ כבר קיים, החזר שגיאה (לפי דרישות המטלה)
    if (std::filesystem::exists(meInfoFilePath)) {
        std::cerr << "Error: my.info already exists!" << std::endl;
        return false;
    }

    // שלב 4: פתיחת הקובץ לכתיבה
    std::ofstream meInfoFile(meInfoFilePath);
    if (!meInfoFile.is_open()) {
        std::cerr << "Error: Unable to open my.info for writing at: "
            << meInfoFilePath << std::endl;
        return false;
    }

    // שלב 5: כתיבת 3 השורות לפי דרישות המטלה
    // 1) שם המשתמש
    meInfoFile << username << "\n";

    // 2) מזהה הלקוח (למשל, אם _clientId כבר מחרוזת ב-16 בתים או ב-Hex)
    //    במידה וצריך לייצג 16 בתים ב-Hex, בצע כאן המרה ל-Hex
	_clientId = bytesToHex(_clientId);
    meInfoFile << _clientId << "\n";

    // 3) המפתח הפרטי ב-Base64
    std::string privateKeyBase64 = Base64Wrapper::encode(_rsaPrivate.getPrivateKey());
    meInfoFile << privateKeyBase64 << "\n";

    // שלב 6: סגירת הקובץ
    meInfoFile.close();

    return true;
}

// -------------------
// פונקציות ציבוריות
// -------------------

bool Client::registerClient(const std::string& username) {
    // שלב 1: בניית ה-payload לרישום
    std::vector<uint8_t> requestPayload = buildRegistrationPayload(username);

    // שלב 2: שליחת הבקשה וקבלת התגובה באמצעות SocketWrapper
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(600, requestPayload);

    // שלב 3: ניתוח התגובה בעזרת Protocol::parseResponse
    uint8_t respVersion;
    uint16_t respCode;
    std::vector<uint8_t> respPayload;
    try {
        std::tie(respVersion, respCode, respPayload) = Protocol::parseResponse(response);
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing response: " << e.what() << std::endl;
        return false;
    }

    // בדיקת קוד התגובה:
    if (respCode != 2100) {
        std::cerr << "Registration failed: "
            << std::string(respPayload.begin(), respPayload.end())
            << std::endl;
        return false;
    }

    // בדיקה שה-payload מכיל 16 בתים (המזהה)
    if (respPayload.size() < 16) {
        std::cerr << "Error: Response payload is too short!" << std::endl;
        return false;
    }

    // שלב 4: עדכון מזהה הלקוח _clientId מתוך 16 הבתים הראשונים של ה-payload
    _clientId = std::string(respPayload.begin(), respPayload.begin() + 16);
	std::cout << "Registration of new client : " << username << " ended successfully." << std::endl;

    // שלב 5: כתיבת פרטי הרישום לקובץ my.info
    if (!writeRegistrationInfoToFile(username)) {
        return;
    }

    return true;
}


// בקשת רשימת משתמשים
void Client::requestClientsList() {
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(601, {});
    if (response.empty()) {
        std::cerr << "Failed to request clients list!" << std::endl;
        return;
    }
    std::cout << "Clients list:\n" << std::string(response.begin(), response.end()) << std::endl;
}

// בקשת מפתח ציבורי ממשתמש אחר
std::string Client::getPublicKey(const std::string& recipient) {
    std::vector<uint8_t> requestPayload(recipient.begin(), recipient.end());
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(602, requestPayload);
    return response.empty() ? "" : std::string(response.begin(), response.end());
}

// שליחת מפתח סימטרי למשתמש אחר
void Client::sendSymmetricKey(const std::string& recipient, const std::string& publicKey) {
    AESWrapper aes;
    RSAPublicWrapper rsaPub(Base64Wrapper::decode(publicKey));
    std::string encryptedKey = rsaPub.encrypt(std::string((char*)aes.getKey(), AESWrapper::DEFAULT_KEYLENGTH));
    _symmetricKeys[recipient] = aes;
    std::vector<uint8_t> payload(recipient.begin(), recipient.end());
    payload.push_back('|');
    payload.insert(payload.end(), encryptedKey.begin(), encryptedKey.end());
    sendRequestAndReceiveResponse(152, payload);
}

// שליחת הודעת טקסט
void Client::sendMessage(const std::string& recipient, const std::string& message) {
    if (_symmetricKeys.find(recipient) == _symmetricKeys.end()) {
        std::cerr << "No symmetric key for recipient!" << std::endl;
        return;
    }
    std::string encryptedMessage = _symmetricKeys[recipient].encrypt(message.c_str(), message.size());
    std::vector<uint8_t> payload(recipient.begin(), recipient.end());
    payload.push_back('|');
    payload.insert(payload.end(), encryptedMessage.begin(), encryptedMessage.end());
    sendRequestAndReceiveResponse(603, payload);
}

// שליפת הודעות
void Client::fetchMessages() {
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(604, {});
    if (response.empty()) {
        std::cerr << "No messages received!" << std::endl;
        return;
    }
    std::cout << "Messages:\n" << std::string(response.begin(), response.end()) << std::endl;
}

// פונקציה המאחדת את פעולות השליחה והקבלה באמצעות SocketWrapper (חיבור TCP יחיד)
std::vector<uint8_t> Client::sendRequestAndReceiveResponse(uint16_t requestCode, const std::vector<uint8_t>& payload) {
    SocketWrapper socketWrapper(_serverIp, _serverPort);
    if (!socketWrapper.isValid()) {
        return {};
    }

    // יצירת הבקשה לפי הפרוטוקול (כולל _clientId, גרסה, קוד הבקשה ו-payload)
    std::vector<uint8_t> request = Protocol::createRequest(_clientId, 1, requestCode, payload);

    // שליחת הבקשה
    if (!socketWrapper.sendAll(request)) {
        return {};
    }

    // קבלת התגובה והחזרתה
    return socketWrapper.receiveAll();
}

// Implement the << operator for Client class
std::ostream& operator<<(std::ostream& os, const Client& client) {
    os << "Client ID: " << client._clientId << "\n";
    os << "Server IP: " << client._serverIp << "\n";
    os << "Server Port: " << client._serverPort << "\n";
    return os;
}