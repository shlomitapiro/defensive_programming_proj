import sqlite3
import os

''' DatabaseManager class for managing the SQLite database.
    It provides methods for executing and fetching queries.
'''

class DatabaseManager:

    def __init__(self, db_name="defensive.db"):
        self.db_name = db_name
        self._ensure_database_file_exists()


    def initialize_database(self):
        with sqlite3.connect(self.db_name) as conn:
            cursor = conn.cursor()

            cursor.execute('''CREATE TABLE IF NOT EXISTS clients (
                                ID BOLD PRIMARY KEY,
                                UserName TEXT NOT NULL UNIQUE,
                                PublicKey BLOB NOT NULL,
                                LastSeen TEXT
                            )''')

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


    def execute_query(self, query: str, params=()):
        try:
            with sqlite3.connect(self.db_name) as conn:
                cursor = conn.cursor()
                cursor.execute(query, params)
                conn.commit()
        except sqlite3.Error as e:
            raise sqlite3.DatabaseError(f"Error executing query: {e}")
        

    def fetch_query(self, query: str, params=()):
        try:
            with sqlite3.connect(self.db_name) as conn:
                cursor = conn.cursor()
                cursor.execute(query, params)
                return cursor.fetchall()
        except sqlite3.Error as e:
            raise sqlite3.DatabaseError(f"Error fetching query: {e}")


    def _ensure_database_file_exists(self):
        if not os.path.exists(self.db_name):
            print(f"Database file '{self.db_name}' not found. Creating it...")
            open(self.db_name, 'w').close()