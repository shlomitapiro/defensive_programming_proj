import socket
from communication.connection_handler import ConnectionHandler
from data.user_manager import UserManager
from data.message_manager import MessageManager
from encryption.encryption_manager import EncryptionManager
from utils.database import initialize_database

def main():

    #initialize data-base if not exists
    initialize_database()

    # read port from myport.info
    try:
        with open('config/myport.info', 'r') as port_file:
            port = int(port_file.read().strip())
    except FileNotFoundError:
        print("Warning: 'myport.info' not found. Using default port 1357.")
        port = 1357
    
    # create main objects
    user_manager = UserManager()
    message_manager = MessageManager()
    encryption_manager = EncryptionManager()
    
    # create socket
    server_socket = socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('0.0.0.0', port))
    server_socket.listen(5)
    print(f"Server is listening on port {port}")


    # accept connections
    try:
        while True:
            client_socket, client_address = server_socket.accept()
            print(f"Connection from {client_address}")

            # handle client
            connection_handler = ConnectionHandler(client_socket, client_address, user_manager, message_manager, encryption_manager)
            connection_handler.handle()

    except KeyboardInterrupt:
        print("Server is shutting down.")
        server_socket.close()
        print("Server is shut down.")

    if __name__ == "__main__":
        main()