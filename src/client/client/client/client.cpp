#include "utils.h"
#include "client.h"

// Constants for fixed field sizes
static const size_t CLIENT_ID_SIZE = 16;
static const size_t USERNAME_SIZE = 255;
static const size_t REGISTRATION_PAYLOAD_SIZE = USERNAME_SIZE + 160; // 415 bytes

// -----------------------------
// Constructor & Destructor
// -----------------------------
Client::Client() : _rsaPrivate() {
    serverInfo = readServerInfo();
    _serverIp = std::get<0>(serverInfo);
    _serverPort = std::get<1>(serverInfo);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

Client::~Client() {
    WSACleanup();
}

// -----------------------------
// Server Information & Registration Helpers
// -----------------------------
std::tuple<std::string, unsigned short> Client::readServerInfo() {
    std::string exeDir = getExeDirectory();
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
    unsigned short port = 0;
    try {
        port = static_cast<unsigned short>(std::stoi(portStr));
    }
    catch (...) {
        throw std::runtime_error("Invalid port in server.info: " + portStr);
    }
    return { ip, port };
}

std::vector<uint8_t> Client::buildRegistrationPayload(const std::string& username) {
    // Retrieve the raw RSA public key (should be 160 bytes for RSA-1024)
    std::string pubKeyRaw = _rsaPrivate.getPublicKey();
    // No Base64 decoding: the key is returned in binary format.
    std::string pubKeyBin = pubKeyRaw;

    if (pubKeyBin.size() < 160) {
        std::cerr << "Error: Public key is too short"<< std::endl;
        return {};
    }
    if (pubKeyBin.size() > 160) {
        pubKeyBin = pubKeyBin.substr(0, 160);
    }

    // Create a 255-byte username buffer, null-padded.
    std::vector<uint8_t> nameBuffer(USERNAME_SIZE, '\0');
    size_t copyLen = std::min(username.size(), static_cast<size_t>(USERNAME_SIZE - 1));
    for (size_t i = 0; i < copyLen; i++) {
        nameBuffer[i] = static_cast<uint8_t>(username[i]);
    }

    // Combine the username and public key to form the registration payload.
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), nameBuffer.begin(), nameBuffer.end());
    payload.insert(payload.end(), pubKeyBin.begin(), pubKeyBin.end());

    if (payload.size() != REGISTRATION_PAYLOAD_SIZE) {
        std::cerr << "Error: Registration payload size is incorrect" << std::endl;
        return {};
    }
    return payload;
}

bool Client::updateClientIdFromResponse(const std::vector<uint8_t>& response) {
    if (response.size() < CLIENT_ID_SIZE)
        return false;
    _clientId = std::string(response.begin(), response.begin() + CLIENT_ID_SIZE);
    std::cout << "ClientID successfully updated" << "\n";
    return true;
}

void Client::writeRegistrationInfoToFile(const std::string& username, const std::string& fileName) {
    std::string filePath = createFileInExeDir(fileName);
    if (filePath.empty()) {
        throw std::runtime_error("Unable to create file: " + fileName);
    }
    std::ofstream meInfoFile(filePath);
    if (!meInfoFile.is_open()) {
        throw std::runtime_error("Unable to open file: " + fileName);
    }
    meInfoFile << username << "\n";
    std::string hexId = bytesToHex(_clientId);
    meInfoFile << hexId << "\n";
    std::string privateKeyBase64 = Base64Wrapper::encode(_rsaPrivate.getPrivateKey());
    meInfoFile << privateKeyBase64 << "\n";
    meInfoFile.close();
}

// -----------------------------
// Public Client Functions
// -----------------------------
bool Client::registerClient(const std::string& username) {
    if (!checkMeInfoFileMissing()) {
        std::cerr << "Error: me.info already exists! Could not add a new user." << std::endl;
        return false;
    }

    std::vector<uint8_t> requestPayload = buildRegistrationPayload(username);
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(600, requestPayload);

    uint8_t respVersion;
    uint16_t respCode;
    std::vector<uint8_t> respPayload;
    try {
        std::tie(respVersion, respCode, respPayload) = Protocol::parseResponse(response);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
    if (respCode != 2100) {
        std::cerr << std::string(respPayload.begin(), respPayload.end()) << std::endl;
        return false;
    }
    if (respPayload.size() < CLIENT_ID_SIZE) {
        std::cerr << "Error: Response payload is too short" << std::endl;
        return false;
    }
    _clientId = std::string(respPayload.begin(), respPayload.begin() + CLIENT_ID_SIZE);
    std::cout << "Registration of a new user ended successfully." << std::endl;
    writeRegistrationInfoToFile(username, "me.info");
    return true;
}

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
    const size_t RECORD_SIZE = CLIENT_ID_SIZE + USERNAME_SIZE; // 16 + 255 = 271 bytes
    size_t count = payload.size() / RECORD_SIZE;
    std::cout << "\nClients list:\n";
    userMap.clear();
    for (size_t i = 0; i < count; i++) {
        size_t offset = i * RECORD_SIZE;
        const uint8_t* recPtr = payload.data() + offset;
        std::string idRaw(reinterpret_cast<const char*>(recPtr), CLIENT_ID_SIZE);
        std::string userName(reinterpret_cast<const char*>(recPtr + CLIENT_ID_SIZE), USERNAME_SIZE);
        // Trim any extra null or whitespace characters.
        userName = trim(userName.c_str());
        std::cout << userName << "\n";
        userMap[userName] = idRaw;
    }
}

std::string Client::getPublicKey(const std::string& userName) {
    if (userMap.empty()) {
        updateUserMap();
    }
    auto it = userMap.find(userName);
    if (it == userMap.end()) {
        std::cerr << "No ID found for user: " << userName << "\n";
        return "";
    }
    std::string idBytes = it->second;
    std::vector<uint8_t> requestPayload(idBytes.begin(), idBytes.end());
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(602, requestPayload);
    if (response.empty()) {
        std::cerr << "No response from server\n";
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
    // Convert the raw public key to Base64 for display.
    std::string pubKeyBin(reinterpret_cast<char*>(respPayload.data()), respPayload.size());
    return Base64Wrapper::encode(pubKeyBin);
}

void Client::sendSymmetricKey(const std::string& recipient, const std::string& publicKey) {
    // Retrieve and adjust the recipient's client ID (16 bytes).
    auto it = userMap.find(recipient);
    if (it == userMap.end()) {
        std::cerr << "Error: Recipient '" << recipient << "' not found in user list.\n";
        return;
    }
    std::string toClientId = adjustToSize(it->second, CLIENT_ID_SIZE);

    // Adjust the sender's client ID.
    std::string fromClientId = adjustToSize(_clientId, CLIENT_ID_SIZE);

    // Decode the provided public key; note that publicKey is expected to be Base64-encoded.
    std::string decodedPub = Base64Wrapper::decode(publicKey);
    if (decodedPub.size() < 160) {
        std::cerr << "Public key is too short\n";
        return;
    }

    std::string encryptedKey;
    AESWrapper aes;
    try {
        RSAPublicWrapper rsaPub(decodedPub);
        // Encrypt the AES key using RSA encryption.
        encryptedKey = rsaPub.encrypt(std::string(reinterpret_cast<char*>(const_cast<unsigned char*>(aes.getKey())), AESWrapper::DEFAULT_KEYLENGTH));
    }
    catch (const CryptoPP::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
    catch (...) {
        std::cerr << "Unknown error in encryption" << std::endl;
        return;
    }

    // Save the AES key for later operations.
    _symmetricKeys[recipient] = aes;

    // Build the payload: [16 bytes toClientId][16 bytes fromClientId][1 byte messageType][4 bytes contentSize][encrypted key]
    uint8_t messageType = 2; // Symmetric key message
    uint32_t contentSize = static_cast<uint32_t>(encryptedKey.size());
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), toClientId.begin(), toClientId.end());
    payload.insert(payload.end(), fromClientId.begin(), fromClientId.end());
    payload.push_back(messageType);
    for (int i = 0; i < 4; i++) {
        payload.push_back((contentSize >> (8 * i)) & 0xFF);
    }
    payload.insert(payload.end(), encryptedKey.begin(), encryptedKey.end());

    std::vector<uint8_t> response = sendRequestAndReceiveResponse(603, payload);
    if (response.empty()) {
        std::cerr << "No response received from server.\n";
        return;
    }
    uint8_t respVersion;
    uint16_t respCode;
    std::vector<uint8_t> respPayload;
    std::tie(respVersion, respCode, respPayload) = Protocol::parseResponse(response);
    if (respCode != 2103) {
        std::cerr << "Error: Server responded with code " << respCode <<"\n";
        if (!respPayload.empty()) {
            std::cerr << "Server message: " << std::string(respPayload.begin(), respPayload.end()) << "\n";
        }
    }
    else {
        std::cout << "Symmetric key sent successfully to '" << recipient << "'.\n";
    }
}


void Client::sendMessage(const std::string& recipient, const std::string& message) {
    // Verify that a symmetric key exists for the recipient.
    auto symIt = _symmetricKeys.find(recipient);
    if (symIt == _symmetricKeys.end()) {
        std::cerr << "No symmetric key for recipient '" << recipient << "'!\n";
        return;
    }

    // Retrieve and adjust the recipient's and sender's client IDs.
    auto it = userMap.find(recipient);
    if (it == userMap.end()) {
        std::cerr << "Error: Recipient '" << recipient << "' not found in userMap.\n";
        return;
    }
    std::string toClientId = adjustToSize(it->second, CLIENT_ID_SIZE);
    std::string fromClientId = adjustToSize(_clientId, CLIENT_ID_SIZE);

    // Encrypt the message using the symmetric AES key.
    AESWrapper& aes = symIt->second;
    std::string encryptedMessage = aes.encrypt(message.c_str(), message.size());
    uint32_t contentSize = static_cast<uint32_t>(encryptedMessage.size());
    uint8_t messageType = 3; // Text message

    // Build the payload: [16 bytes toClientId][16 bytes fromClientId][1 byte messageType][4 bytes contentSize][encrypted message]
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), toClientId.begin(), toClientId.end());
    payload.insert(payload.end(), fromClientId.begin(), fromClientId.end());
    payload.push_back(messageType);
    for (int i = 0; i < 4; i++) {
        payload.push_back((contentSize >> (8 * i)) & 0xFF);
    }
    payload.insert(payload.end(), encryptedMessage.begin(), encryptedMessage.end());

    std::vector<uint8_t> response = sendRequestAndReceiveResponse(603, payload);
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
            std::cerr << "Server message: " << std::string(respPayload.begin(), respPayload.end()) << "\n";
        }
    }
    else {
        std::cout << "Message sent successfully to '" << recipient << "'.\n";
    }
}

void Client::fetchMessages() {
    std::vector<uint8_t> response = sendRequestAndReceiveResponse(604, {});
    if (response.empty()) {
        std::cerr << "No response received (or empty response) from server!\n";
        return;
    }
    uint8_t version;
    uint16_t code;
    std::vector<uint8_t> payload;
    std::tie(version, code, payload) = Protocol::parseResponse(response);
    if (code != 2104) {
        std::cerr << "Error: server responded with code " << code << " instead of 2104.\n";
        return;
    }

    size_t offset = 0;
    // Process each message from the payload
    while (true) {
        // Ensure enough data for the header (16 bytes ID + 4 bytes message ID + 1 byte type + 4 bytes size = 25 bytes)
        if (offset + 25 > payload.size()) break;

        std::string fromClientId(reinterpret_cast<const char*>(&payload[offset]), CLIENT_ID_SIZE);
        offset += CLIENT_ID_SIZE;

        uint32_t messageId = 0;
        for (int i = 0; i < 4; i++) {
            messageId |= (payload[offset + i] << (8 * i));
        }
        offset += 4;

        uint8_t messageType = payload[offset];
        offset += 1;

        uint32_t messageSize = 0;
        for (int i = 0; i < 4; i++) {
            messageSize |= (payload[offset + i] << (8 * i));
        }
        offset += 4;

        if (offset + messageSize > payload.size()) {
            std::cerr << "Error: message size exceeds payload. Possibly corrupted data.\n";
            break;
        }
        std::string content(reinterpret_cast<const char*>(&payload[offset]), messageSize);
        offset += messageSize;

        std::string fromUserName = "Unknown";
        // Find the sender's username using the userMap.
        for (auto& kv : userMap) {
            if (kv.second == fromClientId) {
                fromUserName = kv.first;
                break;
            }
        }

        std::string displayContent;
        switch (messageType) {
        case 1:
            displayContent = "Request for symmetric key";
            break;
        case 2: {
            try {
                std::string decryptedKey = _rsaPrivate.decrypt(content);
                AESWrapper aes((unsigned char*)decryptedKey.data(), decryptedKey.size());
                _symmetricKeys[fromUserName] = aes;
                displayContent = "symmetric key received";
            }
            catch (...) {
                displayContent = "failed to decrypt symmetric key";
            }
            break;
        }
        case 3: {
            auto it = _symmetricKeys.find(fromUserName);
            if (it == _symmetricKeys.end()) {
                displayContent = "can't decrypt message (no symmetric key)";
            }
            else {
                try {
                    std::string plainText = it->second.decrypt(content.c_str(), content.size());
                    displayContent = plainText;
                }
                catch (const CryptoPP::Exception& e) {
                    std::cerr << "Decryption error occurred." << std::endl;
                    displayContent = "can't decrypt message";
                }
                catch (...) {
                    displayContent = "can't decrypt message";
                }
            }
            break;
        }

        default:
            displayContent = "[Unknown message type]";
            break;
        }

        std::cout << "From: " << fromUserName << "\n"
            << "Content:\n" << displayContent << "\n"
            << "-----<EOM>-----\n\n";
    }
}

std::vector<uint8_t> Client::sendRequestAndReceiveResponse(uint16_t requestCode, const std::vector<uint8_t>& payload) {
    SocketWrapper socketWrapper(_serverIp, _serverPort);
    if (!socketWrapper.isValid()) {
        return {};
    }
    std::vector<uint8_t> request = Protocol::createRequest(_clientId, 1, requestCode, payload);
    if (!socketWrapper.sendAll(request)) {
        return {};
    }
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
    const size_t RECORD_SIZE = CLIENT_ID_SIZE + USERNAME_SIZE;
    size_t count = payload.size() / RECORD_SIZE;
    userMap.clear();
    for (size_t i = 0; i < count; i++) {
        size_t offset = i * RECORD_SIZE;
        const uint8_t* recPtr = payload.data() + offset;
        std::string idBin(reinterpret_cast<const char*>(recPtr), CLIENT_ID_SIZE);
        std::string userName(reinterpret_cast<const char*>(recPtr + CLIENT_ID_SIZE), USERNAME_SIZE);
        userName = std::string(userName.c_str());
        userName = trim(userName);
        userMap[userName] = idBin;
    }
}

void Client::sendSymmetricKeyRequest(const std::string& recipient) {
    auto it = userMap.find(recipient);
    if (it == userMap.end()) {
        std::cerr << "Error: Recipient '" << recipient << "' not found in user list.\n";
        return;
    }
    std::string toClientId = adjustToSize(it->second, CLIENT_ID_SIZE);
    std::string fromClientId = adjustToSize(_clientId, CLIENT_ID_SIZE);

    uint8_t messageType = 1; // Request for symmetric key
    std::string requestContent = "symmetric key request";
    uint32_t contentSize = static_cast<uint32_t>(requestContent.size());

    std::vector<uint8_t> payload;
    payload.insert(payload.end(), toClientId.begin(), toClientId.end());
    payload.insert(payload.end(), fromClientId.begin(), fromClientId.end());
    payload.push_back(messageType);
    for (int i = 0; i < 4; i++) {
        payload.push_back((contentSize >> (8 * i)) & 0xFF);
    }
    payload.insert(payload.end(), requestContent.begin(), requestContent.end());

    std::vector<uint8_t> response = sendRequestAndReceiveResponse(603, payload);
    if (response.empty()) {
        std::cerr << "Error: No response received from server.\n";
        return;
    }
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
        std::cerr << "Error: Server responded with code " << respCode <<"\n";
        if (!respPayload.empty()) {
            std::cerr << "Server message: " << std::string(respPayload.begin(), respPayload.end()) << "\n";
        }
        return;
    }
    std::cout << "Symmetric key request sent successfully to '" << recipient << "'.\n";
}