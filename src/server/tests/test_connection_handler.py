import pytest
import socket
from data.database_manager import DatabaseManager
from data.client_manager import ClientManager
from data.message_manager import MessageManager
from communication.connection_handler import ConnectionHandler

@pytest.fixture
def setup_server():
    db_manager = DatabaseManager("test_defensive.db")
    db_manager.initialize_database()
    client_manager = ClientManager(db_manager)
    message_manager = MessageManager(db_manager, client_manager)

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("127.0.0.1", 1357))
    server_socket.listen(1)
    yield server_socket
    server_socket.close()

def test_client_connection(setup_server):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect(("127.0.0.1", 1357))
    client_socket.close()
