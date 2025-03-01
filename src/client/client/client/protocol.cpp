#include "Protocol.h"
#include <cstring>

// יוצר בקשה לפי הפורמט.
// המחרוזת clientId מתוקנת כך שתהיה בדיוק 16 בתים – אם קצרה, מתמלאת בתווים null, ואם ארוכה, נחתכת.
std::vector<uint8_t> Protocol::createRequest(const std::string& clientId, uint8_t version, uint16_t code, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> request;

    // תיקון Client ID ל-16 בתים
    std::string cid = clientId;
    if (cid.size() < 16) {
        cid.append(16 - cid.size(), '\0');
    }
    else if (cid.size() > 16) {
        cid = cid.substr(0, 16);
    }
    request.insert(request.end(), cid.begin(), cid.end());

    // גרסה (1 בית)
    request.push_back(version);

    // Request Code (2 בתים) – little-endian
    request.push_back(code & 0xFF);
    request.push_back((code >> 8) & 0xFF);

    // Payload Size (4 בתים) – little-endian
    uint32_t payloadSize = payload.size();
    for (int i = 0; i < 4; i++) {
        request.push_back((payloadSize >> (i * 8)) & 0xFF);
    }

    // הוספת ה-payload
    request.insert(request.end(), payload.begin(), payload.end());

    return request;
}

// יוצר תגובה לפי הפורמט.
std::vector<uint8_t> Protocol::createResponse(uint8_t version, uint16_t responseCode, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> response;

    // גרסה (1 בית)
    response.push_back(version);

    // Response Code (2 בתים) – little-endian
    response.push_back(responseCode & 0xFF);
    response.push_back((responseCode >> 8) & 0xFF);

    // Payload Size (4 בתים) – little-endian
    uint32_t payloadSize = payload.size();
    for (int i = 0; i < 4; i++) {
        response.push_back((payloadSize >> (i * 8)) & 0xFF);
    }

    // הוספת ה-payload
    response.insert(response.end(), payload.begin(), payload.end());

    return response;
}

// מנתח תגובה ומחזיר tuple: (גרסה, קוד תגובה, payload).
std::tuple<uint8_t, uint16_t, std::vector<uint8_t>> Protocol::parseResponse(const std::vector<uint8_t>& data) {
    uint8_t version = 0;
    uint16_t responseCode = 0;
    std::vector<uint8_t> payload;

    const size_t headerSize = 7; // 1 + 2 + 4
    if (data.size() < headerSize) {
        return std::make_tuple(version, responseCode, payload);
    }

    version = data[0];
    responseCode = data[1] | (data[2] << 8);
    uint32_t payloadSize = 0;
    for (int i = 0; i < 4; i++) {
        payloadSize |= (data[3 + i] << (8 * i));
    }

    if (data.size() < headerSize + payloadSize) {
        return std::make_tuple(version, responseCode, payload);
    }

    payload.insert(payload.end(), data.begin() + headerSize, data.begin() + headerSize + payloadSize);
    return std::make_tuple(version, responseCode, payload);
}
