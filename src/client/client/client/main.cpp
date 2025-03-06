#define NOMINMAX
#include "utils.h"
#include "client.h"
#include "Base64Wrapper.h"


/**
 * @brief Displays the client menu.
 *
 * This function prints the available options for the client to the console.
 */
static void displayMenu() {
    std::cout << "\nMessageU client at your service.\n\n"
        << "110) Register\n"
        << "120) Request for clients list\n"
        << "130) Request for public key\n"
        << "140) Fetch waiting messages\n"
        << "150) Send a text message\n"
        << "151) Send a request for symmetric key\n"
        << "152) Send your symmetric key\n"
        << "0) Exit client\n"
        << "? \n";
}

/**
 * @brief Main entry point of the client application.
 *
 * This function creates a Client instance and then enters a loop where it displays a menu and
 * processes user input to perform various client operations (registration, message sending,
 * key exchange, etc.).
 *
 * @return int Returns 0 upon successful execution.
 */
int main() {
    try{
    Client client;
    bool registered = false;
    std::string username;

    while (true) {
        displayMenu();

        int choice;
        if (!(std::cin >> choice)) {
            // Clear the error state and ignore invalid input
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cerr << "Invalid input. Please enter a number.\n";
            continue;
        }

        // Remove the newline character after reading the integer.
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 0) {
            std::cout << "Exiting...\n";
            break;
        }

        switch (choice) {
        case 110: {
            // Registration: Only allow one registration per client instance.
            if (registered) {
                std::cout << "Error: You already registered.\n";
            }
            else {
                std::cout << "Enter your username: ";
                std::getline(std::cin, username);
                if (client.registerClient(username)) {
                    registered = true;
                }
                else {
                    std::cout << "Server responded with an error during registration.\n";
                }
            }
            break;
        }
        case 120:
            // Request the list of clients from the server.
            client.requestClientsList();
            break;
        case 130: {
            // Request public key for a specific recipient.
            std::cout << "Enter recipient username: ";
            std::string recipient;
            std::getline(std::cin, recipient);
            std::string pubKey = client.getPublicKey(recipient);
            if (pubKey.empty()) {
                std::cout << "Public key not found.\n";
            }
            else {
                std::cout << "Public key for " << recipient << " has been received.\n";
            }
            break;
        }
        case 140:
            // Fetch waiting messages from the server.
            client.fetchMessages();
            break;
        case 150: {
            // Send a text message to a recipient.
            std::cout << "Enter recipient username: ";
            std::string recipient;
            std::getline(std::cin, recipient);
            std::cout << "Enter your message: ";
            std::string message;
            std::getline(std::cin, message);
            client.sendMessage(recipient, message);
            break;
        }
        case 151: {
            // Send a request for a symmetric key to a recipient.
            std::cout << "Enter recipient username for symmetric key request: ";
            std::string recipient;
            std::getline(std::cin, recipient);
            client.sendSymmetricKeyRequest(recipient);
            break;
        }
        case 152: {
            // Send your symmetric key to a recipient.
            std::cout << "Enter recipient username: ";
            std::string recipient;
            std::getline(std::cin, recipient);
            // Automatically obtain the recipient's public key
            std::string recipientPubKey = client.getPublicKey(recipient);
            if (recipientPubKey.empty()) {
                std::cout << "Public key for recipient not found. Please try again.\n";
                break;
            }
            client.sendSymmetricKey(recipient, recipientPubKey);
            break;
        }
        default:
            std::cout << "Invalid choice. Please try again.\n";
            break;
        }
	}
	}
	catch (const std::exception& e) {
		std::cerr << "An error occurred: " << e.what() << '\n';
	}
	catch (...) {
		std::cerr << "An unknown error occurred.\n";
	}
    return 0;
}