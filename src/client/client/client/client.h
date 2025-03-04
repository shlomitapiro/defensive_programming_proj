#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include "AESWrapper.h"
#include "RSAWrapper.h"
#include "Protocol.h"

class Client {
public:
    Client();
    ~Client();

    std::tuple<std::string, unsigned short> readServerInfo();
    bool registerClient(const std::string& username);
    void requestClientsList();
    std::string getPublicKey(const std::string& recipient);
    void sendSymmetricKey(const std::string& recipient, const std::string& publicKey);
    void sendMessage(const std::string& recipient, const std::string& message);
    void fetchMessages();

    void sendSymmetricKeyRequest(const std::string& recipient);

    std::vector<uint8_t> sendRequestAndReceiveResponse(uint16_t requestCode, const std::vector<uint8_t>& payload);

private:
    // פונקציות עזר לפרוצדורת הרישום
    std::vector<uint8_t> buildRegistrationPayload(const std::string& username);
    bool updateClientIdFromResponse(const std::vector<uint8_t>& response);
    //bool writeRegistrationInfoToFile(const std::string& username, const std::string& filePath);
    void updateUserMap();
    void writeRegistrationInfoToFile(const std::string& username, const std::string& fileName);

    // משתנים פרטיים
    std::tuple<std::string, unsigned short> serverInfo;
    std::string _serverIp;
    int _serverPort;
    std::string _clientId;
    RSAPrivateWrapper _rsaPrivate;
    std::unordered_map<std::string, AESWrapper> _symmetricKeys;
    std::unordered_map<std::string, std::string> userMap;
};
