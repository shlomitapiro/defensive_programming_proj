#pragma once
#include "AESWrapper.h"
#include "Base64Wrapper.h"
#include "RSAWrapper.h"
#include "protocol.h"
#include "SocketWrapper.h"


/**
 * @brief The Client class manages the client-side operations of the messaging application.
 *
 * The Client class handles registration, sending and receiving messages, and key exchange
 * with the server. It uses RSA for asymmetric operations and AES for symmetric encryption.
 * It also manages a mapping of user names to client IDs and stores symmetric keys for
 * secure communication.
 */
class Client {
public:
    /**
     * @brief Constructs a new Client object.
     *
     * Initializes the client by reading the server information from a configuration file
     * and setting up the network (Winsock). It also initializes the RSA private key.
     */
    Client();

    /**
     * @brief Destroys the Client object.
     *
     * Cleans up resources such as the Winsock library.
     */
    ~Client();

    /**
     * @brief Reads the server information (IP and port) from a configuration file.
     *
     * The function retrieves the server's IP address and port from a file (e.g., "server.info")
     * located in the same directory as the executable.
     *
     * @return A tuple containing the server IP as a string and the server port as an unsigned short.
     */
    std::tuple<std::string, unsigned short> readServerInfo();

    /**
     * @brief Registers a new client with the server.
     *
     * Sends a registration request to the server with the client's username and public key.
     * On successful registration, the client receives a unique client ID which is stored locally.
     *
     * @param username The username of the client.
     * @return true if registration was successful, false otherwise.
     */
    bool registerClient(const std::string& username);

    /**
     * @brief Requests the list of registered clients from the server.
     *
     * Sends a request to the server to retrieve the list of clients. The response is used
     * to update an internal user map mapping usernames to their unique client IDs.
     */
    void requestClientsList();

    /**
     * @brief Retrieves the public key of a specified recipient.
     *
     * Sends a request to the server using the recipient's client ID to get the corresponding
     * public key. The public key is returned as a Base64-encoded string.
     *
     * @param recipient The username of the recipient.
     * @return The recipient's public key as a Base64-encoded string, or an empty string on failure.
     */
    std::string getPublicKey(const std::string& recipient);

    /**
     * @brief Sends the symmetric key to a specified recipient.
     *
     * Encrypts the symmetric key using the recipient's public key and sends it to the server.
     * The message is built according to the protocol format (16 bytes recipient ID, 16 bytes sender ID,
     * 1 byte message type, 4 bytes content size, and the encrypted symmetric key as content).
     *
     * @param recipient The username of the recipient.
     * @param publicKey The recipient's public key (Base64-encoded).
     */
    void sendSymmetricKey(const std::string& recipient, const std::string& publicKey);

    /**
     * @brief Sends a text message to a specified recipient.
     *
     * Encrypts the message using the symmetric key shared with the recipient and sends it to the server.
     * The message is built according to the protocol format.
     *
     * @param recipient The username of the recipient.
     * @param message The text message to send.
     */
    void sendMessage(const std::string& recipient, const std::string& message);

    /**
     * @brief Fetches waiting messages from the server.
     *
     * Sends a request to the server to retrieve all pending messages for this client.
     * The messages are then parsed and displayed in a specified format.
     */
    void fetchMessages();

    /**
     * @brief Sends a symmetric key request to a specified recipient.
     *
     * This function builds and sends a request message (using the appropriate protocol format)
     * to ask the recipient for a symmetric key.
     *
     * @param recipient The username of the recipient.
     */
    void sendSymmetricKeyRequest(const std::string& recipient);

    /**
     * @brief Sends a request to the server and receives its response.
     *
     * Constructs a complete protocol message using the current client ID, version, request code,
     * and payload. It then sends the message using a TCP socket and returns the server's response.
     *
     * @param requestCode The request code as defined by the protocol.
     * @param payload The payload data as a vector of bytes.
     * @return A vector of bytes containing the server's response.
     */
    std::vector<uint8_t> sendRequestAndReceiveResponse(uint16_t requestCode, const std::vector<uint8_t>& payload);

private:
    /**
     * @brief Builds the registration payload for the client.
     *
     * Constructs a payload consisting of 255 bytes for the username (null-padded) and 160 bytes for the
     * RSA public key in raw binary format.
     *
     * @param username The username of the client.
     * @return A vector of bytes representing the registration payload.
     */
    std::vector<uint8_t> buildRegistrationPayload(const std::string& username);

    /**
     * @brief Updates the client ID from the server's response.
     *
     * Extracts the first 16 bytes from the response payload and stores it as the client's unique ID.
     *
     * @param response The response payload from the server.
     * @return true if the client ID was updated successfully, false otherwise.
     */
    bool updateClientIdFromResponse(const std::vector<uint8_t>& response);

    /**
     * @brief Updates the local user map by requesting the list of clients from the server.
     *
     * The user map is used to associate usernames with their raw 16-byte client IDs.
     */
    void updateUserMap();

    /**
     * @brief Writes the registration information to a file.
     *
     * Saves the username, client ID (in hexadecimal representation), and private key (Base64-encoded)
     * to a file (e.g., "me.info") in the executable's directory.
     *
     * @param username The username of the client.
     * @param fileName The name of the file to which the information will be written.
     */
    void writeRegistrationInfoToFile(const std::string& username, const std::string& fileName);

    // Private member variables:

    std::tuple<std::string, unsigned short> serverInfo; ///< Tuple holding the server IP and port.
    std::string _serverIp;        ///< Server IP address.
    unsigned short _serverPort;              ///< Server port number.
    std::string _clientId;        ///< Client's unique ID (16 raw bytes).
    RSAPrivateWrapper _rsaPrivate;///< RSA private key wrapper for this client.
    std::unordered_map<std::string, AESWrapper> _symmetricKeys; ///< Map of recipient usernames to symmetric keys.
    std::unordered_map<std::string, std::string> userMap; ///< Map of usernames to their raw 16-byte client IDs.
};