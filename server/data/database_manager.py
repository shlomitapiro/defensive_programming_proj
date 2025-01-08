import sqlite3

class DatabaseManager:
    def __init__(self, db_name="defensive.db"):
        self.db_name = db_name

    def initialize_database(self):
        """Creates the database and required tables if they don't exist."""
        conn = self._connect()
        cursor = conn.cursor()

        # Create clients table
        cursor.execute('''CREATE TABLE IF NOT EXISTS clients (
                            ID TEXT PRIMARY KEY,
                            UserName TEXT NOT NULL UNIQUE,
                            PublicKey BLOB NOT NULL,
                            LastSeen TEXT
                         )''')

        # Create messages table
        cursor.execute('''CREATE TABLE IF NOT EXISTS messages (
                            ID INTEGER PRIMARY KEY AUTOINCREMENT,
                            ToClient TEXT NOT NULL,
                            FromClient TEXT NOT NULL,
                            Type INTEGER NOT NULL,
                            Content BLOB NOT NULL,
                            FOREIGN KEY (ToClient) REFERENCES clients(ID),
                            FOREIGN KEY (FromClient) REFERENCES clients(ID)
                         )''')

        conn.commit()
        conn.close()

    # Helper methods
    def _connect(self):
        """Connects to the database and returns the connection object."""
        return sqlite3.connect(self.db_name)

    def execute_query(self, query, params=()):
        """Executes a given query with optional parameters."""
        conn = self._connect()
        cursor = conn.cursor()
        cursor.execute(query, params)
        conn.commit()
        conn.close()

    def fetch_query(self, query, params=()):
        """Executes a given query and fetches results."""
        conn = self._connect()
        cursor = conn.cursor()
        cursor.execute(query, params)
        results = cursor.fetchall()
        conn.close()
        return results

    # Clients table operations
    def add_client(self, client_id, username, public_key):
        """Adds a new client to the database."""
        query = '''INSERT INTO clients (ID, UserName, PublicKey, LastSeen)
                   VALUES (?, ?, ?, datetime('now'))'''
        params = (client_id, username, public_key)
        try:
            self.execute_query(query, params)
            print(f"Client {username} added successfully.")
        except sqlite3.IntegrityError as e:
            print(f"Error adding client: {e}")

    def get_public_key(self, client_id):
        """Fetches the public key of a client by ID."""
        query = '''SELECT PublicKey FROM clients WHERE ID = ?'''
        params = (client_id,)
        result = self.fetch_query(query, params)
        return result[0][0] if result else None

    def update_last_seen(self, client_id):
        """Updates the last seen time of a client."""
        query = '''UPDATE clients
                   SET LastSeen = datetime('now')
                   WHERE ID = ?'''
        params = (client_id,)
        self.execute_query(query, params)
        print(f"Updated last seen for client {client_id}.")

    def get_all_clients(self):
        """Fetches all clients from the database."""
        query = '''SELECT ID, UserName, PublicKey, LastSeen FROM clients'''
        return self.fetch_query(query)

    # Messages table operations
    def add_message(self, to_client, from_client, message_type, content):
        """Adds a new message to the database."""
        query = '''INSERT INTO messages (ToClient, FromClient, Type, Content)
                   VALUES (?, ?, ?, ?)'''
        params = (to_client, from_client, message_type, content)
        self.execute_query(query, params)
        print(f"Message from {from_client} to {to_client} added successfully.")

    def get_messages_for_client(self, client_id):
        """Fetches all messages waiting for a specific client."""
        query = '''SELECT ID, FromClient, Type, Content
                   FROM messages
                   WHERE ToClient = ?'''
        params = (client_id,)
        return self.fetch_query(query, params)

    def delete_message(self, message_id):
        """Deletes a message from the database."""
        query = '''DELETE FROM messages WHERE ID = ?'''
        params = (message_id,)
        self.execute_query(query, params)
        print(f"Message {message_id} deleted successfully.")