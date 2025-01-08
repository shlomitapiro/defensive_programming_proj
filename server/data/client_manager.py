
class ClientManager:
    def __init__(self, db_manager):
        """Initializes the UserManager with a DatabaseManager instance."""
        self.db_manager = db_manager

    def add_client(self, client_id, username, public_key):
        """Adds a new client to the database."""
        query = '''INSERT INTO clients (ID, UserName, PublicKey, LastSeen)
                   VALUES (?, ?, ?, datetime('now'))'''
        params = (client_id, username, public_key)
        try:
            self.db_manager.execute_query(query, params)
            print(f"Client {username} added successfully.")
        except Exception as e:
            print(f"Error adding client: {e}")

    def get_public_key(self, client_id):
        """Fetches the public key of a client by ID."""
        query = '''SELECT PublicKey FROM clients WHERE ID = ?'''
        params = (client_id,)
        result = self.db_manager.fetch_query(query, params)
        return result[0][0] if result else None

    def update_last_seen(self, client_id):
        """Updates the last seen time of a client."""
        query = '''UPDATE clients
                   SET LastSeen = datetime('now')
                   WHERE ID = ?'''
        params = (client_id,)
        self.db_manager.execute_query(query, params)
        print(f"Updated last seen for client {client_id}.")

    def get_all_clients(self):
        """Fetches all clients from the database."""
        query = '''SELECT ID, UserName, PublicKey, LastSeen FROM clients'''
        return self.db_manager.fetch_query(query)
