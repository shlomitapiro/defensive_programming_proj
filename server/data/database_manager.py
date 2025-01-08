import sqlite3

class DatabaseManager:
    def __init__(self, db_name="defensive.db"):
        """Initializes the database manager with the given database name."""
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

    def _connect(self):
        """Establishes a connection to the database."""
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