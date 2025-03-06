#include "utils.h"
#include "protocol.h"


std::vector<uint8_t> Protocol::createRequest(const std::string& clientId, uint8_t version, uint16_t code, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> request;

    // Append the adjusted 16-byte Client ID.
    std::string cid = adjustToSize(clientId, 16);
    request.insert(request.end(), cid.begin(), cid.end());

    // Append version (1 byte).
    request.push_back(version);

    // Append Request Code (2 bytes, little-endian).
    request.push_back(code & 0xFF);
    request.push_back((code >> 8) & 0xFF);

    // Append Payload Size (4 bytes, little-endian).
    uint32_t payloadSize = static_cast<uint32_t>(payload.size());
    for (int i = 0; i < 4; i++) {
        request.push_back((payloadSize >> (8 * i)) & 0xFF);
    }

    // Append the actual payload.
    request.insert(request.end(), payload.begin(), payload.end());

    return request;
}

std::vector<uint8_t> Protocol::createResponse(uint8_t version, uint16_t responseCode, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> response;

    // Append version (1 byte).
    response.push_back(version);

    // Append Response Code (2 bytes, little-endian).
    response.push_back(responseCode & 0xFF);
    response.push_back((responseCode >> 8) & 0xFF);

    // Append Payload Size (4 bytes, little-endian).
    uint32_t payloadSize = static_cast<uint32_t>(payload.size());
    for (int i = 0; i < 4; i++) {
        response.push_back((payloadSize >> (8 * i)) & 0xFF);
    }

    // Append the payload.
    response.insert(response.end(), payload.begin(), payload.end());

    return response;
}

std::tuple<uint8_t, uint16_t, std::vector<uint8_t>> Protocol::parseResponse(const std::vector<uint8_t>& data) {
    const size_t headerSize = 7; // 1 byte for version, 2 bytes for response code, 4 bytes for payload size.
    if (data.size() < headerSize) {
        throw std::runtime_error("Data too short for response header");
    }

    // Extract the version.
    uint8_t version = data[0];

    // Extract the response code (little-endian).
    uint16_t responseCode = data[1] | (data[2] << 8);

    // Extract the payload size (little-endian).
    uint32_t payloadSize = 0;
    for (int i = 0; i < 4; i++) {
        payloadSize |= (data[3 + i] << (8 * i));
    }

    // Validate that the data contains the full payload.
    if (data.size() < headerSize + payloadSize) {
        std::ostringstream err;
        err << "Incomplete response: Expected payload size " << payloadSize
            << ", but got " << (data.size() - headerSize);
        throw std::runtime_error(err.str());
    }

    // Copy the payload.
    std::vector<uint8_t> payload(data.begin() + headerSize, data.begin() + headerSize + payloadSize);

    return std::make_tuple(version, responseCode, payload);
}