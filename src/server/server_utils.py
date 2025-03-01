# server_utils.py
import os
import socket
import logging
from data.database_manager import DatabaseManager
from data.client_manager import ClientManager
from data.message_manager import MessageManager
import threading

# הגדרת לוגינג – ניתן לשנות את רמת הלוגינג ואת הפורמט
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def load_port():
    """
    טוען את מספר הפורט מקובץ myport.info או משתמש בפורט ברירת מחדל (1357)
    אם הקובץ אינו קיים.
    """
    port_file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "config/myport.info"))
    logging.info(f"Looking for myport.info at: {port_file_path}")
    try:
        with open(port_file_path, "r") as port_file:
            port = int(port_file.read().strip())
            logging.info(f"Loaded port: {port}")
            return port
    except FileNotFoundError:
        logging.warning("'myport.info' not found. Using default port 1357.")
        return 1357

def init_database():
    """
    מאתחל את מסד הנתונים ומוודא שהטבלאות קיימות.
    """
    db_manager = DatabaseManager()
    db_manager.initialize_database()
    return db_manager

def init_managers(db_manager):
    """
    יוצר את מנהלי הלקוחות וההודעות בהתבסס על מסד הנתונים.
    """
    client_manager = ClientManager(db_manager)
    message_manager = MessageManager(db_manager, client_manager)
    return client_manager, message_manager

def init_server_socket(port: int) -> socket.socket:
    """
    יוצר סוקט שרת, בורד אותו לכל כתובות '0.0.0.0' ומאזין על הפורט הנתון.
    """
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('0.0.0.0', port))
    server_socket.listen(5)
    logging.info(f"Server socket initialized and listening on port {port}")
    return server_socket

def handle_client_connection(client_socket: socket.socket, client_address, client_manager, message_manager):
    """
    יוצר מופע ConnectionHandler ומפעיל את הטיפול בחיבור.
    כל חיבור יקבל thread נפרד.
    """
    from communication.connection_handler import ConnectionHandler
    logging.info(f"Handling connection from {client_address}")
    connection_handler = ConnectionHandler(client_socket, client_address, client_manager, message_manager)
    try:
        connection_handler.handle()  # טיפול סינכרוני בתוך ה-thread
    except Exception as e:
        logging.exception(f"Exception in connection handler for {client_address}: {e}")
    finally:
        client_socket.close()

def accept_connections(server_socket: socket.socket, client_manager, message_manager) -> None:
    """
    לולאה אינסופית המקבלת חיבורים חדשים ומפעילה עבור כל אחד thread נפרד לטיפול בו.
    """
    while True:
        try:
            client_socket, client_address = server_socket.accept()
            logging.info(f"Accepted connection from {client_address}")
            client_socket.setblocking(True)  # אפשר לעבוד עם blocking בתוך ה-thread
            # הפעלת טיפול בחיבור בתוך thread חדש
            thread = threading.Thread(target=handle_client_connection, args=(client_socket, client_address, client_manager, message_manager))
            thread.daemon = True  # כך שה-threads לא ימנעו סגירת התוכנית
            thread.start()
        except Exception as e:
            logging.exception(f"Exception in accept_connections: {e}")
            break  # במקרה של חריגה, יוצאים מהלולאה

def run_server():
    """
    פונקציה ראשית שמאחדת את כל תהליכי האתחול ומפעילה את לולאת קבלת החיבורים.
    """
    port = load_port()
    db_manager = init_database()
    client_manager, message_manager = init_managers(db_manager)
    server_socket = init_server_socket(port)
    logging.info("Server is up and running.")
    try:
        accept_connections(server_socket, client_manager, message_manager)
    except KeyboardInterrupt:
        logging.info("KeyboardInterrupt received. Shutting down server.")
    finally:
        server_socket.close()
        logging.info("Server is shut down.")
