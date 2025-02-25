from data.client_manager import ClientManager
from data.database_manager import DatabaseManager

class MessageManager:
    def __init__(self, db_manager: DatabaseManager, client_manager: ClientManager):
        self.db_manager: DatabaseManager = db_manager
        self.client_manager: ClientManager = client_manager

    def add_message(self, to_client: str, from_client: str, message_type: int, content: bytes) -> None:
        """Adds a new message to the database after verifying both clients exist."""
        if not self.client_manager.client_exists_by_id(to_client):
            raise ValueError(f"Recipient client {to_client} does not exist.")
        if not self.client_manager.client_exists_by_id(from_client):
            raise ValueError(f"Sender client {from_client} does not exist.")

        query = '''INSERT INTO messages (ToClient, FromClient, Type, Content)
                   VALUES (?, ?, ?, ?)'''
        try:
            params = (to_client, from_client, message_type, content)
            self.db_manager.execute_query(query, params)
            print(f"Message from {from_client} to {to_client} added successfully.")
        except Exception as e:
            print(f"Error adding message: {e}")
        
    def get_messages_for_client(self, client_id) -> list[tuple]:

        query = '''SELECT ID, FromClient, Type, Content
                   FROM messages
                   WHERE ToClient = ?'''
        try:
            message = self.db_manager.fetch_query(query, (client_id,))
            print(f"Messages for client {client_id} fetched successfully.")
            return message if message else []
        except Exception as e:
            print(f"Error fetching messages for client: {e}")
            return []

    def delete_message(self, message_id: int) -> None:

        query_check = '''SELECT ID FROM messages WHERE ID = ?'''
        query_delete = '''DELETE FROM messages WHERE ID = ?'''

        if not self.db_manager.fetch_query(query_check, (message_id,)):
            print(f"Message {message_id} does not exist.")
            return
        try:
            self.db_manager.execute_query(query_delete, (message_id,))
            print(f"Message {message_id} deleted successfully.")

            remaining_messages = self.db_manager.fetch_query(query_check, (message_id,))
            if remaining_messages:
                print(f"Warning: Message {message_id} was not deleted successfully.")
            else:
                print(f"Confirmed: Message {message_id} no longer exists.")
        except Exception as e:
            print(f"Error deleting message: {e}")