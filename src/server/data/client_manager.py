from data.database_manager import DatabaseManager

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
        except Exception as e:
            raise Exception(f"Database error while adding client {username}: {e}") 

    def get_public_key(self, client_id_hex: str) -> bytes | None:
        """Fetches the public key of a client by ID."""
        query = '''SELECT PublicKey FROM clients WHERE ID = ?'''
        result = self.db_manager.fetch_query(query, (client_id_hex,))
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
        """
        Fetches all clients from the database, ensuring that ID is returned as bytes (16 bytes).
        Returns a list of tuples (id_bytes, user_name).
        """
        query = '''SELECT ID, UserName FROM clients'''
        rows = self.db_manager.fetch_query(query)
        print(f"Got {len(rows)} clients from the database.")
        clients = []
        for row in rows:
            # row[0] -> יכול להיות bytes כבר, או string, תלוי בהגדרת העמודה ב-DB.
            id_val = row[0]
            user_name = row[1]

            # אם העמודה ב-DB היא BLOB, fetch_query כנראה מחזיר bytes (16 בתים).
            # אבל אם משום מה מוגדר כ-TEXT, ייתכן שתקבל string. 
            # נוודא שזה bytes:
            if isinstance(id_val, str):
                # אם בטעות נשמר כטקסט, כנראה זה hex. נמיר ל-bytes (16 בתים).
                id_bytes = bytes.fromhex(id_val)
            else:
                # מניחים שזה כבר bytes באורך 16
                id_bytes = id_val

            # אפשר לבדוק אם len(id_bytes) == 16, ואם לא -> שגיאה
            print(f"Got client {user_name} with ID: {id_bytes}")
            print(f"ID type: {type(id_bytes)}")
            print(f"ID length: {len(id_bytes)}")
            if len(id_bytes) != 16:
                print(f"Warning: ID length is not 16, got {len(id_bytes)}. ID: {id_bytes}")
            
            clients.append((id_bytes, user_name))
            print(f"c: {clients}")

        return clients
    
    def client_exists_by_id(self, client_id) -> bool:
        """Checks if a client exists in the database by ID."""
        query = '''SELECT 1 FROM clients WHERE ID = ?'''
        params = (client_id,)
        return bool(self.db_manager.fetch_query(query, params))

    def client_exists_by_username(self, username) -> bool:
        """Checks if a client exists in the database by username."""
        query = '''SELECT 1 FROM clients WHERE UserName = ?'''
        params = (username,)
        return bool(self.db_manager.fetch_query(query, params))