#include "Client.h"
#include "Base64Wrapper.h"
#include "SocketWrapper.h"
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
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));

    std::string serverFilePath = exeDir + "\\server.info";
    std::ifstream serverFile(serverFilePath);
    if (!serverFile.is_open()) {
        throw std::runtime_error("Cannot open server.info file: " + serverFilePath);
    }

    std::string line;
    if (!std::getline(serverFile, line)) {
        throw std::runtime_error("server.info file is empty: " + serverFilePath);
    }
    serverFile.close();

    auto pos = line.find(':');
    if (pos == std::string::npos) {
        throw std::runtime_error("server.info format error: missing ':' in file: " + serverFilePath);
    }

    std::string ip = line.substr(0, pos);
    std::string portStr = line.substr(pos + 1);
    unsigned short port;
    try {
        port = static_cast<unsigned short>(std::stoi(portStr));
    }
    catch (...) {
        throw std::runtime_error("Invalid port in server.info: " + portStr);
    }

    return { ip, port };
}

// -------------------
// פונקציות עזר לרישום
// -------------------

std::vector<uint8_t> Client::buildRegistrationPayload(const std::string& username) {

    std::string pubKeyRaw = _rsaPrivate.getPublicKey();
    std::string pubKeyBin = pubKeyRaw; 

    if (pubKeyBin.size() < 160) {
		std::cerr << "Error: Public key is too short: " << pubKeyBin.size() << " bytes." << std::endl;
		return {};
     }

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

// writeRegistrationInfoToFile() - גרסה שמשתמשת בחריגה
void Client::writeRegistrationInfoToFile(const std::string& username, const std::string& fileName) {
    std::string filePath = createFileInExeDir(fileName);
    if (filePath.empty()) {
        throw std::runtime_error("Unable to create file: " + fileName);
    }

    std::ofstream meInfoFile(filePath);
    if (!meInfoFile.is_open()) {
        throw std::runtime_error("Unable to open file: " + fileName);
    }

    // כתיבת המידע לקובץ
    meInfoFile << username << "\n";
    std::string hexId = bytesToHex(_clientId);
    meInfoFile << hexId << "\n";
    std::string privateKeyBase64 = Base64Wrapper::encode(_rsaPrivate.getPrivateKey());
    meInfoFile << privateKeyBase64 << "\n";
    meInfoFile.close();
}


// -------------------
// פונקציות ציבוריות
// -------------------

bool Client::registerClient(const std::string& username) {

	// if me.info already exists, return false
	if (!checkMeInfoFileMissing()) {
		std::cerr << "Error: me.info already exists! could not add a new user" << std::endl;
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
        //std::cerr << "server responded with an error: " << e.what() << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    // בדיקת קוד התגובה:
    if (respCode != 2100) {
        //std::cerr << "server responded with an error:"
        std::cerr
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

	writeRegistrationInfoToFile(username, "me.info");

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


void Client::sendSymmetricKey(const std::string& recipient, const std::string& publicKey) {
    // 1) איתור מזהה הלקוח של הנמען (16 בתים) מתוך userMap
    auto it = userMap.find(recipient);
    if (it == userMap.end()) {
        std::cerr << "Error: Recipient '" << recipient << "' not found in user list.\n";
        return;
    }
    std::string toClientId = it->second;
    // וידוא גודל בדיוק 16
    if (toClientId.size() < 16) {
        toClientId.resize(16, '\0');
    }
    else if (toClientId.size() > 16) {
        toClientId = toClientId.substr(0, 16);
    }

    // 2) מזהה השולח (16 בתים)
    std::string fromClientId = _clientId;
    if (fromClientId.size() < 16) {
        fromClientId.resize(16, '\0');
    }
    else if (fromClientId.size() > 16) {
        fromClientId = fromClientId.substr(0, 16);
    }

    // 3) יצירת מפתח סימטרי והצפנתו במפתח הציבורי של הנמען
    AESWrapper aes;
    std::string decodedPub = Base64Wrapper::decode(publicKey);

    if (decodedPub.size() < 160)
    {
		std::cerr << "Publick key is too short: " << decodedPub.size() << " bytes Aborting...\n";
		return;
    }

	std::string encryptedKey;
    try {
        RSAPublicWrapper rsaPub(decodedPub);
        encryptedKey = rsaPub.encrypt(std::string((char*)aes.getKey(), AESWrapper::DEFAULT_KEYLENGTH));
    }
    catch (const CryptoPP::Exception& e) {
        std::cerr << "Crypto++ error: " << e.what() << std::endl;
        return;
    }
    catch (...) {
        std::cerr << "Unknown error in RSA encryption" << std::endl;
        return;
    }

    // שומרים את ה-AES בצד הלקוח (כך שנוכל להצפין הודעות בהמשך)
    _symmetricKeys[recipient] = aes;

    // 4) הגדרת סוג ההודעה = 2 (שליחת מפתח סימטרי)
    uint8_t messageType = 2;
    uint32_t contentSize = static_cast<uint32_t>(encryptedKey.size());

    // 5) בניית ה־payload
    //    16 בתים toClientId + 16 בתים fromClientId + 1 בית messageType + 4 בתים contentSize + תוכן
    std::vector<uint8_t> payload;
    // toClientId (16 בתים)
    payload.insert(payload.end(), toClientId.begin(), toClientId.end());
    // fromClientId (16 בתים)
    payload.insert(payload.end(), fromClientId.begin(), fromClientId.end());
    // messageType (1 בית)
    payload.push_back(messageType);
    // contentSize (4 בתים little-endian)
    for (int i = 0; i < 4; i++) {
        payload.push_back((contentSize >> (8 * i)) & 0xFF);
    }
    // התוכן (המפתח הסימטרי המוצפן ב־RSA)
    payload.insert(payload.end(), encryptedKey.begin(), encryptedKey.end());

    // 6) שליחת הבקשה לשרת (Request Code = 603)
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(603, payload);

    // (אופציונלי) עיבוד התשובה
    if (response.empty()) {
        std::cerr << "No response received from server for sendSymmetricKey.\n";
        return;
    }

    uint8_t respVersion;
    uint16_t respCode;
    std::vector<uint8_t> respPayload;
    std::tie(respVersion, respCode, respPayload) = Protocol::parseResponse(response);
    if (respCode != 2103) {
        std::cerr << "Error: Server responded with code " << respCode << " for sendSymmetricKey.\n";
        if (!respPayload.empty()) {
            std::cerr << "Server message: "
                << std::string(respPayload.begin(), respPayload.end()) << "\n";
        }
    }
    else {
        std::cout << "Symmetric key sent successfully to '" << recipient << "'.\n";
    }
}


void Client::sendMessage(const std::string& recipient, const std::string& message) {
    // 1) בודקים אם יש לנו מפתח סימטרי עבור הנמען
    auto symIt = _symmetricKeys.find(recipient);
    if (symIt == _symmetricKeys.end()) {
        std::cerr << "No symmetric key for recipient '" << recipient << "'!\n";
        return;
    }

    // 2) מאתרים את מזהה הלקוח (16 בתים) של הנמען
    auto it = userMap.find(recipient);
    if (it == userMap.end()) {
        std::cerr << "Error: Recipient '" << recipient << "' not found in userMap.\n";
        return;
    }
    std::string toClientId = it->second;
    if (toClientId.size() < 16) {
        toClientId.resize(16, '\0');
    }
    else if (toClientId.size() > 16) {
        toClientId = toClientId.substr(0, 16);
    }

    // 3) מזהה השולח (16 בתים)
    std::string fromClientId = _clientId;
    if (fromClientId.size() < 16) {
        fromClientId.resize(16, '\0');
    }
    else if (fromClientId.size() > 16) {
        fromClientId = fromClientId.substr(0, 16);
    }

    // 4) הצפנת ההודעה ב-AES
    AESWrapper& aes = symIt->second; // המפתח הסימטרי ששמרנו
    std::string encryptedMessage = aes.encrypt(message.c_str(), message.size());
    uint32_t contentSize = static_cast<uint32_t>(encryptedMessage.size());

    // 5) הגדרת סוג ההודעה = 3 (שליחת הודעת טקסט)
    uint8_t messageType = 3;

    // 6) בניית ה־payload
    //    16 בתים toClient + 16 בתים fromClient + 1 בית type + 4 בתים contentSize + תוכן
    std::vector<uint8_t> payload;
    // toClientId
    payload.insert(payload.end(), toClientId.begin(), toClientId.end());
    // fromClientId
    payload.insert(payload.end(), fromClientId.begin(), fromClientId.end());
    // messageType
    payload.push_back(messageType);
    // contentSize (little-endian)
    for (int i = 0; i < 4; i++) {
        payload.push_back((contentSize >> (8 * i)) & 0xFF);
    }
    // התוכן המוצפן
    payload.insert(payload.end(), encryptedMessage.begin(), encryptedMessage.end());

    // 7) שליחת הבקשה (Request Code = 603)
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(603, payload);

    // (אופציונלי) עיבוד התשובה
    if (response.empty()) {
        std::cerr << "No response from server for sendMessage.\n";
        return;
    }

    uint8_t respVersion;
    uint16_t respCode;
    std::vector<uint8_t> respPayload;
    std::tie(respVersion, respCode, respPayload) = Protocol::parseResponse(response);

    if (respCode != 2103) {
        std::cerr << "Error: Server responded with code " << respCode << " for sendMessage.\n";
        if (!respPayload.empty()) {
            std::cerr << "Server message: "
                << std::string(respPayload.begin(), respPayload.end()) << "\n";
        }
    }
    else {
        std::cout << "Message sent successfully to '" << recipient << "'.\n";
    }
}


void Client::fetchMessages() {
    // 1) שולחים בקשה 604 לשרת (ללא payload).
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(604, {});
    if (response.empty()) {
        std::cerr << "No response received (or empty response) from server!\n";
        return;
    }

    // 2) מפענחים את התגובה בעזרת Protocol::parseResponse
    uint8_t version;
    uint16_t code;
    std::vector<uint8_t> payload;
    std::tie(version, code, payload) = Protocol::parseResponse(response);

    // 3) בודקים שקיבלנו תשובת 2104
    if (code != 2104) {
        std::cerr << "Error: server responded with code " << code << " instead of 2104.\n";
        // אפשר להציג את respPayload כטקסט במידת הצורך
        return;
    }

    // 4) מפרקים את ה־payload להודעות מרובות (ייתכן שיש יותר מהודעה אחת)
    size_t offset = 0;
    while (true) {
        // נוודא שיש מקום ל-16+4+1+4 = 25 בתים לפני התוכן
        if (offset + 16 + 4 + 1 + 4 > payload.size()) {
            // אין מקום לעוד הודעה – יוצאים מהלולאה
            break;
        }

        // 4.1) קוראים 16 בתים עבור מזהה השולח
        std::string fromClientId(reinterpret_cast<const char*>(&payload[offset]), 16);
        offset += 16;

        // 4.2) קוראים 4 בתים עבור מזהה הודעה (Message ID)
        uint32_t messageId = 0;
        for (int i = 0; i < 4; i++) {
            messageId |= (payload[offset + i] << (8 * i));
        }
        offset += 4;

        // 4.3) קוראים 1 בית עבור סוג ההודעה (Message Type)
        uint8_t messageType = payload[offset];
        offset += 1;

        // 4.4) קוראים 4 בתים עבור גודל ההודעה (Message Size)
        uint32_t messageSize = 0;
        for (int i = 0; i < 4; i++) {
            messageSize |= (payload[offset + i] << (8 * i));
        }
        offset += 4;

        // 4.5) בדיקה שיש מספיק מקום ב־payload לתוכן ההודעה
        if (offset + messageSize > payload.size()) {
            std::cerr << "Error: message size exceeds payload. Possibly corrupted data.\n";
            break; // או return
        }

        // 4.6) קוראים את תוכן ההודעה
        std::string content(reinterpret_cast<const char*>(&payload[offset]), messageSize);
        offset += messageSize;

        // 5) מוצאים את שם המשתמש של השולח לפי fromClientId
        std::string fromUserName = "Unknown";
        for (auto& kv : userMap) {
            if (kv.second == fromClientId) {
                fromUserName = kv.first;
                break;
            }
        }

        // 6) מציגים/מעבדים לפי סוג ההודעה (Message Type)
        std::string displayContent;
        switch (messageType) {
        case 1: { // בקשת מפתח סימטרי
            // המטלה: “Request for symmetric key”
            displayContent = "Request for symmetric key";
            break;
        }
        case 2: { // שליחת מפתח סימטרי
            // מוצפן ב-RSA, צריך לפענח באמצעות המפתח הפרטי שלנו
            try {
                std::string decryptedKey = _rsaPrivate.decrypt(content);
                AESWrapper aes((unsigned char*)decryptedKey.data(), decryptedKey.size());
                // שומרים את ה-AES במפה לשימוש עתידי (שליחת/קבלת הודעות)
                _symmetricKeys[fromUserName] = aes;
                displayContent = "symmetric key received";
            }
            catch (...) {
                displayContent = "failed to decrypt symmetric key";
            }
            break;
        }
        case 3: { // הודעת טקסט
            // מוצפן ב-AES. אם אין לנו מפתח סימטרי, לא נוכל לפענח
            auto it = _symmetricKeys.find(fromUserName);
            if (it == _symmetricKeys.end()) {
                displayContent = "can't decrypt message (no symmetric key)";
            }
            else {
                try {
                    std::string plainText = it->second.decrypt(content.c_str(), content.size());
                    displayContent = plainText;
                }
                catch (...) {
                    displayContent = "can't decrypt message (decryption error)";
                }
            }
            break;
        }
        case 4: {
            // בונוס: שליחת קובץ
            // לדוגמה, מפענחים ומנסים לשמור את הקובץ, או מציגים הודעה למשתמש
            displayContent = "[File received] (not implemented)";
            break;
        }
        default:
            displayContent = "[Unknown message type]";
            break;
        }

        // 7) מציגים את ההודעה בפורמט הנדרש
        std::cout << "From: " << fromUserName << "\n"
            << "Content:\n" << displayContent << "\n"
            << "-----<EOM>-----\n\n";
    } // end while
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

    const size_t RECORD_SIZE = 16 + 255; // 16 בתים ID + 255 בתים שם
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
        userName = trim(userName); // הסרת רווחים מיותרים

        // שמירת המיפוי: משתמש במפתח שמייצג את שם המשתמש המדויק
        userMap[userName] = idBin;
    }
}


void Client::sendSymmetricKeyRequest(const std::string& recipient) {
    // בדיקה – האם היעד קיים במפת המשתמשים
    auto it = userMap.find(recipient);
    if (it == userMap.end()) {
        std::cerr << "Error: Recipient '" << recipient << "' not found in user list.\n";
        return;
    }
    std::string toClientId = it->second;
    // וודא שהמזהה של היעד הוא בדיוק 16 בתים
    if (toClientId.size() < 16) {
        toClientId.resize(16, '\0');
    }
    else if (toClientId.size() > 16) {
        toClientId = toClientId.substr(0, 16);
    }

    // גם מזהה השולח צריך להיות בדיוק 16 בתים
    std::string fromClientId = _clientId;
    if (fromClientId.size() < 16) {
        fromClientId.resize(16, '\0');
    }
    else if (fromClientId.size() > 16) {
        fromClientId = fromClientId.substr(0, 16);
    }

    // סוג ההודעה לבקשת מפתח סימטרי – 1
    uint8_t messageType = 1;
    // התוכן הקבוע עבור בקשת מפתח סימטרי
    std::string requestContent = "symmetric key request";
    uint32_t contentSize = static_cast<uint32_t>(requestContent.size());

    // בניית ה-payload
    std::vector<uint8_t> payload;
    // 1. מזהה היעד – 16 בתים
    payload.insert(payload.end(), toClientId.begin(), toClientId.end());
    // 2. מזהה השולח – 16 בתים
    payload.insert(payload.end(), fromClientId.begin(), fromClientId.end());
    // 3. סוג ההודעה – 1 בית
    payload.push_back(messageType);
    // 4. גודל התוכן – 4 בתים (little-endian)
    for (int i = 0; i < 4; i++) {
        payload.push_back((contentSize >> (8 * i)) & 0xFF);
    }
    // 5. תוכן ההודעה – מחרוזת קבועה
    payload.insert(payload.end(), requestContent.begin(), requestContent.end());

    // שליחת הבקשה לשרת – משתמשים ב-Request Code 603 (כפי שעושים עם הודעות)
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(603, payload);
    if (response.empty()) {
        std::cerr << "Error: No response received from server for symmetric key request.\n";
        return;
    }

    // ניתוח התשובה
    uint8_t respVersion;
    uint16_t respCode;
    std::vector<uint8_t> respPayload;
    try {
        std::tie(respVersion, respCode, respPayload) = Protocol::parseResponse(response);
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing response: " << e.what() << "\n";
        return;
    }

    if (respCode != 2103) {
        std::cerr << "Error: Server responded with code " << respCode << " for symmetric key request.\n";
        if (!respPayload.empty()) {
            std::cerr << "Server message: " << std::string(respPayload.begin(), respPayload.end()) << "\n";
        }
        return;
    }
    std::cout << "Symmetric key request sent successfully to '" << recipient << "'.\n";
}
