import pytest
from data.database_manager import DatabaseManager
from data.client_manager import ClientManager

@pytest.fixture
def client_manager():
    db_manager = DatabaseManager("test_defensive.db")
    db_manager.initialize_database()
    return ClientManager(db_manager)

def test_add_client(client_manager: ClientManager):
    client_manager.add_client("123", "Alice", b"public_key")
    assert client_manager.client_exists_by_id("123") is True

def test_duplicate_client(client_manager: ClientManager):
    client_manager.add_client("124", "Bob", b"public_key")
    with pytest.raises(Exception):
        client_manager.add_client("125", "Bob", b"public_key")