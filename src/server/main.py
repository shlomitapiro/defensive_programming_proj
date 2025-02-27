import selectors
import socket
import os
from data.client_manager import ClientManager
from data.message_manager import MessageManager
from data.database_manager import DatabaseManager
from communication.connection_handler import ConnectionHandler
from encryption.encryption_manager import EncryptionManager

selector: selectors.DefaultSelector = selectors.DefaultSelector()

def accept_connection(server_socket: socket.socket, client_manager: ClientManager, message_manager: MessageManager, encryption_manager: EncryptionManager) -> None:
    """Accepts a new client connection and registers it with the selector."""
    client_socket, client_address = server_socket.accept()
    print(f"Connection from {client_address}")
    client_socket.setblocking(False)

    connection_handler: ConnectionHandler = ConnectionHandler(client_socket, client_address, client_manager, message_manager, encryption_manager)
    selector.register(client_socket, selectors.EVENT_READ, connection_handler.handle)

def load_port():
    """Loads the port number from myport.info or defaults to 1357."""
    port_file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "config/myport.info"))

    print(f"Looking for myport.info at: {port_file_path}")

    try:
        with open(port_file_path, "r") as port_file:
            return int(port_file.read().strip())
    except FileNotFoundError:
        print("Warning: 'myport.info' not found. Using default port 1357.")
        return 1357
    
def main() -> None:
    
    port = load_port()
    print(f"Server will start on port {port}")

    db_manager: DatabaseManager = DatabaseManager()
    db_manager.initialize_database()

    try:
        with open('config/myport.info', 'r') as port_file:
            port = int(port_file.read().strip())
    except FileNotFoundError:
        print("Warning: 'myport.info' not found. Using default port 1357.")
        port = 1357

    client_manager: ClientManager = ClientManager(db_manager)
    message_manager: MessageManager = MessageManager(db_manager, client_manager)
    encryption_manager: EncryptionManager = EncryptionManager()

    server_socket: socket.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('0.0.0.0', port))
    server_socket.listen(5)
    server_socket.setblocking(False)
    
    selector.register(server_socket, selectors.EVENT_READ, lambda sock: accept_connection(sock, client_manager, message_manager, encryption_manager))

    print(f"Server is listening on port {port}")

    try:
        # Main server loop waits for events and calls from clients
        while True:
            events = selector.select(timeout=None)
            for key, _ in events:
                callback = key.data
                callback(key.fileobj)

    except KeyboardInterrupt:
        print("Server is shutting down.")
    finally:
        selector.close()
        server_socket.close()
        print("Server is shut down.")

if __name__ == "__main__":
    main()