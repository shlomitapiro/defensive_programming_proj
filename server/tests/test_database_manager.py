import os
import pytest
import shutil
from data.database_manager import DatabaseManager

@pytest.fixture
def db_manager():
    test_db = "test_defensive.db"
    db_manager = DatabaseManager(test_db)
    db_manager.initialize_database()
    yield db_manager
    shutil.rmtree(test_db, ignore_errors=True)

def test_database_creation(db_manager):
    assert os.path.exists("test_defensive.db")

def test_insert_and_fetch_client(db_manager):
    db_manager.execute_query("INSERT INTO clients (ID, UserName, PublicKey, LastSeen) VALUES (?, ?, ?, datetime('now'))",
                             ("123", "Alice", b"public_key"))
    result = db_manager.fetch_query("SELECT * FROM clients WHERE ID = ?", ("123",))
    assert len(result) == 1
    assert result[0][1] == "Alice"
