import pytest
from data.database_manager import DatabaseManager
from data.client_manager import ClientManager
from data.message_manager import MessageManager

@pytest.fixture
def message_manager():
    db_manager = DatabaseManager("test_defensive.db")
    db_manager.initialize_database()
    client_manager = ClientManager(db_manager)
    return MessageManager(db_manager, client_manager)

def test_add_message(message_manager: MessageManager):
    message_manager.add_message("123", "124", 1, b"Hello!")
    messages = message_manager.get_messages_for_client("123")
    assert len(messages) == 1
    assert messages[0][3] == b"Hello!"

def test_delete_message(message_manager: MessageManager):
    message_manager.add_message("123", "124", 1, b"Test Delete")
    messages = message_manager.get_messages_for_client("123")
    print(f"Retrieved messages before deletion: {messages}")

    for message in messages:
        message_manager.delete_message(message[0])

    remaining_messages = message_manager.get_messages_for_client("123")
    print(f"Retrieved messages after deletion: {remaining_messages}")

    assert len(remaining_messages) == 0 