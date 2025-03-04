#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <tuple>
#include <stdexcept>

class Protocol {
public:
    // יוצר בקשה (Request) לפי הפורמט:
    // Client ID (16 בתים), Version (1 בית), Request Code (2 בתים, little-endian),
    // Payload Size (4 בתים, little-endian), ואז ה-payload.
    static std::vector<uint8_t> createRequest(const std::string& clientId, uint8_t version, uint16_t code, const std::vector<uint8_t>& payload);

    // יוצר תגובה (Response) לפי הפורמט:
    // Version (1 בית), Response Code (2 בתים, little-endian),
    // Payload Size (4 בתים, little-endian), ואז ה-payload.
    static std::vector<uint8_t> createResponse(uint8_t version, uint16_t responseCode, const std::vector<uint8_t>& payload);

    // מנתח תגובה ומחזיר tuple של (גרסה, קוד תגובה, payload).
    static std::tuple<uint8_t, uint16_t, std::vector<uint8_t>> parseResponse(const std::vector<uint8_t>& data);
};
