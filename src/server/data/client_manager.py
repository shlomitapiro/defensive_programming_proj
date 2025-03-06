from data.database_manager import DatabaseManager
import sqlite3

''' ClientManager class is responsible for managing clients in the database.
    It provides methods for adding clients, getting public keys, updating last seen time,
    getting all clients, and checking if a client exists by ID or username. 
'''

class ClientManager:

    def __init__(self, db_manager: DatabaseManager):
        self.db_manager: DatabaseManager = db_manager


    def add_client(self, client_id: str, username: str, public_key: bytes):
        if self.client_exists_by_username(username):
            raise ValueError(f"Client with username '{username}' already exists.")
        
        query = '''INSERT INTO clients (ID, UserName, PublicKey, LastSeen)
                   VALUES (?, ?, ?, datetime('now'))'''
        params = (client_id, username, public_key)

        try:
            self.db_manager.execute_query(query, params)
            print(f"Client {username} added successfully.")
        except sqlite3.DatabaseError as e:
            raise Exception(f"Database error while adding client {username}: {e}")
        

    def get_public_key(self, client_id_hex: str) -> bytes | None:
        query = '''SELECT PublicKey FROM clients WHERE ID = ?'''
        result = self.db_manager.fetch_query(query, (client_id_hex,))
        return result[0][0] if result else None
    

    def get_all_clients(self):
        query = '''SELECT ID, UserName FROM clients'''
        rows = self.db_manager.fetch_query(query)
        print(f"Got {len(rows)} clients from the database.")
        clients = []
        for row in rows:
            id_val = row[0]
            user_name = row[1]

            if isinstance(id_val, str):
                id_bytes = bytes.fromhex(id_val)
            else:
                id_bytes = id_val
                
            if len(id_bytes) != 16:
                print(f"Warning: ID length is not 16, got {len(id_bytes)}. ID: {id_bytes}")
            
            clients.append((id_bytes, user_name))

        return clients
    
    
    def client_exists_by_id(self, client_id) -> bool:
        query = '''SELECT 1 FROM clients WHERE ID = ?'''
        params = (client_id,)
        return bool(self.db_manager.fetch_query(query, params))
    

    def client_exists_by_username(self, username) -> bool:
        query = '''SELECT 1 FROM clients WHERE UserName = ?'''
        params = (username,)
        return bool(self.db_manager.fetch_query(query, params))