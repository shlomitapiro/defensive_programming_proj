# MessageU – Defensive Systems Programming Project

Welcome to MessageU, a secure client-server messaging application developed for an academic exercise in defensive systems programming. Below is an overview of the system’s architecture, setup instructions, and usage guidelines.

## 1. Introduction
MessageU is a simple end-to-end encrypted messaging platform demonstrating secure message exchange between a Python-based server and multiple C++ clients. The application is designed to:

Register new users (storing public keys on the server).

Exchange public keys so that clients can share a symmetric key (AES) securely.

Send and receive encrypted messages (and optionally files) via the server, which acts as a message relay without seeing any plaintext.

## 2. System Architecture
Client-Server Model:

- A central server receives requests, holds messages in storage, and forwards them to the correct recipients.

- Each client connects to the server to register, send messages, or fetch waiting messages.

- Messages are encrypted end-to-end, preventing the server from reading them in plaintext.

End-to-End Encryption:

- RSA (asymmetric) is used for exchanging keys: one client encrypts a newly generated AES key with the other client’s public RSA key.

- AES (symmetric) is used to encrypt actual text messages with a shared key known only to the two clients involved.

Data Storage:

- The project demonstrates using an SQLite database file (e.g. defensive.db) for persistent storage.

## 3. Protocol Overview
Communication between client and server follows a custom binary protocol over TCP.

Request (client → server):

Header:

- Client ID: 16 bytes

- Version: 1 byte

- Request Code: 2 bytes

- Payload Size: 4 bytes (little endian)

- Payload: variable length, depends on the request code

Response (server → client):

Header:

- Version: 1 byte

- Response Code: 2 bytes

- Payload Size: 4 bytes (little endian)

- Payload: variable length, depends on the response code

Main Request Codes:

- 600: Register

- 601: Request Clients List

- 602: Request Public Key

- 603: Send Message (any message type)

- 604: Fetch Waiting Messages

Main Response Codes:

- 2100: Registration successful (includes new Client ID)

- 2101: Clients list

- 2102: Public key

- 2103: Acknowledgment of storing a new message

- 2104: Delivery of waiting messages

- 9000: General error response

## 4. Encryption Details
RSA (1024-bit):

Used for exchanging the symmetric key.
Each client holds a private key and corresponding public key.
The client’s public key is stored on the server for other clients to request.

AES (CBC mode):

Used for encrypting messages and files.
128-bit keys (16 bytes).
For simplicity, the IV is set to zero in this exercise (not recommended for production).

Security Flow:

- Client A obtains Client B’s public RSA key from the server.

- Client A creates an AES symmetric key and encrypts it with B’s public key; server stores/delivers it to B.

- Client B uses its private RSA key to decrypt and retrieve the AES key.

- Subsequent messages use that AES key for end-to-end encryption.

## 5. Installation & Setup

Server Setup:

Requirements:

- Python 3.x

- Standard libraries (socket, sqlite3, logging, etc.)

- Port Configuration

- By default, the server reads the port from a file named myport.info.

- If that file doesn’t exist or is invalid, the server uses port 1357.

Running the Server:

    python main.py

The server will start listening for connections and log status messages to the console.
    
Client Setup:

Requirements:

- A C++11 (or later) compiler (e.g., Visual Studio, g++, MinGW).

- The Crypto++ library (for RSA and AES).

Server Address:

- Put the server IP and port in server.info, e.g. 127.0.0.1:1357.

Building the Client:

- Include all .cpp and .h files in the client folder in your project.

- Link against Crypto++.

Running the Client:

    ./MessageUClient.exe


The client provides an interactive menu in the console.

## 6. Usage
    Client Menu:

    When you run the client, you will see the following options:

        MessageU client at your service.

        110) Register
        120) Request for clients list
        130) Request for public key
        140) Fetch waiting messages
        150) Send a text message
        151) Send a request for symmetric key
        152) Send your symmetric key
        0) Exit client
        ?

    (110) Register: Prompts for a username, generates an RSA key pair locally, and sends the public key to the server. Saves the client’s ID and private key in me.info.

    (120) Request for clients list: Asks the server for all registered users (IDs and names).

    (130) Request for public key: Fetches another user’s public key from the server by username.

    (140) Fetch waiting messages: Retrieves pending messages from the server, decrypts them if possible.

    (150) Send a text message: Encrypts a message with a shared AES key and sends it to the recipient.

    (151) Request for symmetric key: Sends a request asking the other user to share a symmetric key.

    (152) Send your symmetric key: Generates an AES key, encrypts it with the recipient’s RSA public key, and sends it so both can share the same key.

    (0) Exit client: Closes the client application.


## 7. Troubleshooting & Tips
Ensure Configuration Files Exist:

- server.info on the client side must contain <ip>:<port>.

- myport.info on the server side is optional (defaults to 1357 if missing).

Check Logs:

- The server prints logs (via Python’s logging) to help diagnose connection or database issues.

Key Management:

- The private RSA key is saved in me.info as Base64.

- Public keys are stored on the server.

- If you cannot decrypt a message, ensure you have correctly exchanged symmetric keys or requested them properly.