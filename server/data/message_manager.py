from database_manager import DatabaseManager
from client_manager import ClientManager

class MessageManager:
    def __init__(self, db_manager, client_manager):
        self.db_manager = db_manager
        self.client_manager = client_manager

    def add_message(self, to_client, from_client, message_type, content):
        """Adds a new message to the database after verifying both clients exist."""
        if not self.client_manager.client_exists(to_client):
            raise ValueError(f"Recipient client {to_client} does not exist.")
        if not self.client_manager.client_exists(from_client):
            raise ValueError(f"Sender client {from_client} does not exist.")

        query = '''INSERT INTO messages (ToClient, FromClient, Type, Content)
                   VALUES (?, ?, ?, ?)'''
        try:
            params = (to_client, from_client, message_type, content)
            self.db_manager.execute_query(query, params)
            print(f"Message from {from_client} to {to_client} added successfully.")
        except Exception as e:
            print(f"Error adding message: {e}")
        
    def get_messages_for_client(self, client_id):

        query = '''SELECT ID, FromClient, Type, Content
                   FROM messages
                   WHERE ToClient = ?'''
        try:
            self.db_manager.fetch_query(query, (client_id,))
            print(f"Messages for client {client_id} fetched successfully.")
        except Exception as e:
            print(f"Error fetching messages for client: {e}")

    def delete_message(self, message_id):
        query = '''DELETE FROM messages WHERE ID = ?'''
        if not self.db_manager.fetch_query(query, (message_id,)):
            print(f"Message {message_id} does not exist.")
            return
        try:
            self.db_manager.execute_query(query, (message_id,))
            print(f"Message {message_id} deleted successfully.")
        except Exception as e:
            print(f"Error deleting message: {e}")