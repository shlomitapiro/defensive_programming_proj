#define NOMINMAX
#include "Client.h"
#include "utils.h"
#include <limits>


static void displayMenu() {
    std::cout << "\nMessageU client at your service.\n"
        << "\n"
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

int main() {
    Client client;
    bool registered = false;
    std::string username;

    while (true) {
        displayMenu();

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cerr << "Invalid input. Please enter a number.\n";
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 0) {
            std::cout << "Exiting...\n";
            break;
        }

        switch (choice) {
        case 110: {
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
                    std::cout << "server responded with an error.\n";
                }
            }
            break;
        }
        case 120:
            client.requestClientsList();
            break;
        case 130: {
            std::cout << "Enter recipient username: ";
            std::string recipient;
            std::getline(std::cin, recipient);
            std::string pubKey = client.getPublicKey(recipient);
            if (pubKey.empty()) {
                std::cout << "Public key not found.\n";
            }
            else {
                std::cout << "Public key for " << recipient << " has been received" << "\n";
            }
            break;
        }

        case 140:
            client.fetchMessages();
            break;
        case 150: {
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
            std::cout << "Enter recipient username for symmetric key request: ";
            std::string recipient;
            std::getline(std::cin, recipient);
            client.sendSymmetricKeyRequest(recipient);
            break;
        }
        case 152: {
            std::cout << "Enter recipient username: ";
            std::string recipient;
            std::getline(std::cin, recipient);
            // קבלת המפתח הציבורי באופן אוטומטי מהשרת:
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
    return 0;
}
