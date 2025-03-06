#pragma once

/**
 * @brief Provides methods to create and parse protocol messages.
 *
 * The Protocol class defines methods for constructing requests and responses
 * according to a specific binary format.
 * The format for a Request is:
 * - Client ID (16 bytes)
 * - Version (1 byte)
 * - Request Code (2 bytes, little-endian)
 * - Payload Size (4 bytes, little-endian)
 * - Payload (variable length)
 *
 * The format for a Response is:
 * - Version (1 byte)
 * - Response Code (2 bytes, little-endian)
 * - Payload Size (4 bytes, little-endian)
 * - Payload (variable length)
 */
class Protocol {
public:
    /**
     * @brief Creates a request message in the specified format.
     *
     * The resulting vector of bytes is constructed as follows:
     * - 16 bytes for the Client ID (if the provided clientId is shorter than 16 bytes, it is padded with '\0'; if longer, it is truncated).
     * - 1 byte for the Version.
     * - 2 bytes for the Request Code, encoded in little-endian order.
     * - 4 bytes for the Payload Size, encoded in little-endian order.
     * - The Payload itself.
     *
     * @param clientId The client identifier as a string.
     * @param version The version number of the client.
     * @param code The request code.
     * @param payload The payload data as a vector of bytes.
     * @return A vector of bytes representing the complete request message.
     */
    static std::vector<uint8_t> createRequest(const std::string& clientId, uint8_t version, uint16_t code, const std::vector<uint8_t>& payload);

    /**
     * @brief Creates a response message in the specified format.
     *
     * The resulting vector of bytes is constructed as follows:
     * - 1 byte for the Version.
     * - 2 bytes for the Response Code, encoded in little-endian order.
     * - 4 bytes for the Payload Size, encoded in little-endian order.
     * - The Payload itself.
     *
     * @param version The version number of the server.
     * @param responseCode The response code.
     * @param payload The payload data as a vector of bytes.
     * @return A vector of bytes representing the complete response message.
     */
    static std::vector<uint8_t> createResponse(uint8_t version, uint16_t responseCode, const std::vector<uint8_t>& payload);

    /**
     * @brief Parses a response message.
     *
     * This function extracts the fields from a response message that is expected to be in the following format:
     * - Version (1 byte)
     * - Response Code (2 bytes, little-endian)
     * - Payload Size (4 bytes, little-endian)
     * - Payload (variable length)
     *
     * @param data The response message as a vector of bytes.
     * @return A tuple containing:
     *         - The version (uint8_t)
     *         - The response code (uint16_t)
     *         - The payload as a vector of bytes.
     *
     * @throws std::runtime_error if the data is too short to contain a valid header.
     */
    static std::tuple<uint8_t, uint16_t, std::vector<uint8_t>> parseResponse(const std::vector<uint8_t>& data);
};