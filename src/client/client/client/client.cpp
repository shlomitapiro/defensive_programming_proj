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

                try {
                    std::string portStr = line.substr(pos + 1);
                    serverPort = static_cast<unsigned short>(std::stoi(portStr));
                }
                catch (...) {
                    std::cerr << "Warning: invalid port in server.info, using default 1234.\n";
                    serverPort = 1234;
                }
            }
            else {
                std::cerr << "Warning: server.info missing ':' separator, using default 127.0.0.1:1234.\n";
            }
        }
        serverFile.close();
    }
    else {
        std::cerr << "Warning: Could not open " << serverFilePath
            << "Using default 127.0.0.1:1234" << std::endl;
    }
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


// מעדכן את _clientId מתוך תגובת השרת (16 בתים ראשונים)*********************///////////////////***********
bool Client::updateClientIdFromResponse(const std::vector<uint8_t>& response) {
    if (response.size() < 16) {
        return false;
    }
    _clientId = std::string(response.begin(), response.begin() + 16);
    std::cout << "ClientID successfully updated" << "\n";
    return true;
}

// כותב את פרטי הרישום לקובץ me.info
bool Client::writeRegistrationInfoToFile(const std::string& username, const std::string& fileName) {
    std::string exePath = createFileInExeDir(fileName);
    if (exePath.empty()) {
        std::cerr << "Error: Unable to create file: " << fileName << std::endl;
        return false;
    }

    std::ofstream meInfoFile(exePath);
    if (!meInfoFile.is_open()) {
        std::cerr << "Error: Unable to open file: " << fileName << std::endl;
        return false;
    }

    // 1) שם המשתמש
    meInfoFile << username << "\n";

    // 2) מזהה הלקוח כ-Hex לצורך כתיבה לקובץ (לפי דרישות המטלה),
    // אבל _clientId עצמו נשאר 16 בתים raw בזיכרון.
    std::string hexId = bytesToHex(_clientId);
    meInfoFile << hexId << "\n";

    // 3) מפתח פרטי ב-Base64
    std::string privateKeyBase64 = Base64Wrapper::encode(_rsaPrivate.getPrivateKey());
    meInfoFile << privateKeyBase64 << "\n";

    meInfoFile.close();
    return true;
}

// -------------------
// פונקציות ציבוריות
// -------------------

bool Client::registerClient(const std::string& username) {

	// if me.info already exists, return false
	if (!checkMeInfoFileExists()) {
		std::cerr << "Error: me.info already exists! could not add the new user" << std::endl;
		return false;
	}

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
        std::cerr << "server responded with an error: " << e.what() << std::endl;
        return false;
    }

    // בדיקת קוד התגובה:
    if (respCode != 2100) {
        std::cerr << "server responded with an error:"
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
	std::cout << "Registration of a new user ended successfully." << std::endl;

    // שלב 5: כתיבת פרטי הרישום לקובץ my.info
    if (!writeRegistrationInfoToFile(username, "me.info")) {
		return false;
    }

    return true;
}


// בקשת רשימת משתמשים
void Client::requestClientsList() {
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(601, {});
    if (response.empty()) {
        std::cerr << "No response received for clients list!\n";
        return;
    }

    uint8_t version;
    uint16_t code;
    std::vector<uint8_t> payload;
    std::tie(version, code, payload) = Protocol::parseResponse(response);

    if (code != 2101) {
        std::cerr << "Error: server responded with code " << code << "\n";
        return;
    }

    const size_t RECORD_SIZE = 16 + 255; // 271
    size_t count = payload.size() / RECORD_SIZE;

    std::cout << "\nClients list:\n";

    // נניח שהגדרת map גלובלי/חברי במחלקה:
    userMap.clear(); // ריקון המפה הקודמת

    for (size_t i = 0; i < count; i++) {
        size_t offset = i * RECORD_SIZE;
        const uint8_t* recordPtr = payload.data() + offset;

        // 16 בתים עבור ID (נניח בינארי)
        std::string idRaw(reinterpret_cast<const char*>(recordPtr), 16);

        // המרה ל-hex (אורך 32 תווים)
        std::string idHex = bytesToHex(idRaw);

        // 255 בתים לשם
        std::string userName(reinterpret_cast<const char*>(recordPtr + 16), 255);
        userName = userName.c_str();
     
        std::cout << "" << userName << "\n";

        // שמירת המיפוי
        userMap[userName] = idRaw;
    }
}

std::string Client::getPublicKey(const std::string& userName) {
    // אם המפה ריקה, טען אותה אוטומטית
    if (userMap.empty()) {
        updateUserMap();
    }

    auto it = userMap.find(userName);
    if (it == userMap.end()) {
        std::cerr << "No ID found for user: " << userName << "\n";
        return "";
    }
    // קבלת ה-ID כערך raw (16 בתים) ישירות מהמפה
    std::string idBytes = it->second;

    // שלח בקשה 602 עם ה-ID (כ-raw bytes)
    std::vector<uint8_t> requestPayload(idBytes.begin(), idBytes.end());
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(602, requestPayload);
    if (response.empty()) {
        std::cerr << "No response from server for getPublicKey\n";
        return "";
    }

    uint8_t respVersion;
    uint16_t respCode;
    std::vector<uint8_t> respPayload;
    std::tie(respVersion, respCode, respPayload) = Protocol::parseResponse(response);

    if (respCode != 2102) {
        std::cerr << "Server error code: " << respCode << "\n";
        return "";
    }
    // הנחת עבודה: המפתח הציבורי מוחזר כבייטים, נרצה להמיר אותו ל־Base64 להצגה קריאה
    std::string pubKeyBin(reinterpret_cast<char*>(respPayload.data()), respPayload.size());
    return Base64Wrapper::encode(pubKeyBin);
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

// פונקציה לעדכון המפה
void Client::updateUserMap() {
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(601, {});
    if (response.empty()) {
        std::cerr << "Failed to load clients list automatically.\n";
        return;
    }

    uint8_t version;
    uint16_t code;
    std::vector<uint8_t> payload;

    std::tie(version, code, payload) = Protocol::parseResponse(response);

    if (code != 2101) {
        std::cerr << "Error: server responded with code " << code << "\n";
        return;
    }

    const size_t RECORD_SIZE = 16 + 255; // כל רשומה: 16 בתים ID + 255 בתים שם
    size_t count = payload.size() / RECORD_SIZE;

    userMap.clear();
    for (size_t i = 0; i < count; i++) {
        size_t offset = i * RECORD_SIZE;
        const uint8_t* recPtr = payload.data() + offset;

        // קבלת ה־ID כ־16 בתים (raw bytes)
        std::string idBin(reinterpret_cast<const char*>(recPtr), 16);

        // קבלת שם המשתמש (255 בתים), חיתוך עד null-terminator
        std::string userName(reinterpret_cast<const char*>(recPtr + 16), 255);
        userName = userName.c_str();

        // שמירת ה־ID כ־raw bytes ישירות במפה
        userMap[userName] = idBin;

    }
}