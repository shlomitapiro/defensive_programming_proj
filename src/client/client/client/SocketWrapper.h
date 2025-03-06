#pragma once
#include "utils.h"


/**
 * @brief A wrapper class for TCP socket operations.
 *
 * This class encapsulates a WinSock TCP socket, providing functionality to create the socket,
 * connect to a server using an IP address and port, send and receive data reliably, and close the socket.
 */
class SocketWrapper {
public:
    /**
     * @brief Constructs a new SocketWrapper and connects to the specified server.
     *
     * This constructor creates a TCP socket and attempts to connect it to the server identified
     * by the provided IP address and port.
     *
     * @param serverIp The IP address of the server.
     * @param serverPort The port number of the server.
     */
    SocketWrapper(const std::string& serverIp, unsigned short serverPort);

    /**
     * @brief Destroys the SocketWrapper.
     *
     * If the socket is still open, it will be closed.
     */
    ~SocketWrapper();

    /**
     * @brief Checks if the socket is valid.
     *
     * This method returns true if the socket was created and the connection was successful.
     *
     * @return true if the socket is valid, false otherwise.
     */
    bool isValid() const;

    /**
     * @brief Sends all data over the socket.
     *
     * This method ensures that the entire data vector is sent over the socket, even if multiple
     * send operations are required.
     *
     * @param data A vector of bytes representing the data to send.
     * @return true if all data was sent successfully, false otherwise.
     */
    bool sendAll(const std::vector<uint8_t>& data) const;

    /**
     * @brief Receives data from the socket.
     *
     * This method receives data from the socket using a default buffer size of 1024 bytes and
     * returns the data in a vector. If the connection is closed gracefully, an empty vector is returned.
     *
     * @return A vector of bytes containing the received data.
     */
    std::vector<uint8_t> receiveAll() const;

    /**
     * @brief Closes the socket.
     *
     * Closes the underlying socket if it is open.
     */
    void closeSocket();

private:
    SOCKET sock; ///< The underlying WinSock socket.
};