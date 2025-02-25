import socket
from data.client_manager import ClientManager
from data.message_manager import MessageManager
from data.database_manager import DatabaseManager
from communication.connection_handler import ConnectionHandler
from encryption.encryption_manager import EncryptionManager

def main():
    # Initialize the database if it doesn't exist
    db_manager = DatabaseManager()
    db_manager.initialize_database()

    # Read port from myport.info
    try:
        with open('config/myport.info', 'r') as port_file:
            port = int(port_file.read().strip())
    except FileNotFoundError:
        print("Warning: 'myport.info' not found. Using default port 1357.")
        port = 1357

    # Create main objects with db_manager dependency
    client_manager = ClientManager(db_manager)
    message_manager = MessageManager(db_manager, client_manager)
    encryption_manager = EncryptionManager()

    # Create and bind socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('0.0.0.0', port))
    server_socket.listen(5)
    print(f"Server is listening on port {port}")

    # Accept connections
    try:
        while True:
            client_socket, client_address = server_socket.accept()
            print(f"Connection from {client_address}")

            # Handle client
            with client_socket:
                connection_handler = ConnectionHandler(client_socket, client_address, client_manager, message_manager, encryption_manager)
                connection_handler.handle()

    except KeyboardInterrupt:
        print("Server is shutting down.")
    finally:
        server_socket.close()
        print("Server is shut down.")

# Ensure main() runs only when executed directly
if __name__ == "__main__":
    main()
