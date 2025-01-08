from database_manager import DatabaseManager

class MessageManager:
    def __init__(self, db_manager):
        """Initializes the MessageManager with a DatabaseManager instance."""
        self.db_manager = db_manager

    def add_message(self, to_client, from_client, message_type, content):
        """Adds a new message to the database."""
        query = '''INSERT INTO messages (ToClient, FromClient, Type, Content)
                   VALUES (?, ?, ?, ?)'''
        params = (to_client, from_client, message_type, content)
        self.db_manager.execute_query(query, params)
        print(f"Message from {from_client} to {to_client} added successfully.")

    def get_messages_for_client(self, client_id):
        """Fetches all messages waiting for a specific client."""
        query = '''SELECT ID, FromClient, Type, Content
                   FROM messages
                   WHERE ToClient = ?'''
        params = (client_id,)
        return self.db_manager.fetch_query(query, params)

    def delete_message(self, message_id):
        """Deletes a message from the database."""
        query = '''DELETE FROM messages WHERE ID = ?'''
        params = (message_id,)
        self.db_manager.execute_query(query, params)
        print(f"Message {message_id} deleted successfully.")