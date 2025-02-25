from database_manager import DatabaseManager

class ClientManager:
    def __init__(self, db_manager):
        self.db_manager = db_manager

    def add_client(self, client_id, username, public_key):
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
    
    def client_exists_by_id(self, client_id):
        """Checks if a client exists in the database by ID."""
        query = '''SELECT 1 FROM clients WHERE ID = ?'''
        params = (client_id,)
        return bool(self.db_manager.fetch_query(query, params))

    def client_exists_by_username(self, username):
        """Checks if a client exists in the database by username."""
        query = '''SELECT 1 FROM clients WHERE UserName = ?'''
        params = (username,)
        return bool(self.db_manager.fetch_query(query, params))