import logging
import socket
import struct
import uuid
from communication.protocol import Protocol
from data.client_manager import ClientManager
from data.message_manager import MessageManager

class ConnectionHandler:
    def __init__(self, client_socket: socket.socket, client_address: tuple[str, int], 
                 client_manager: ClientManager, message_manager: MessageManager) -> None:
        self.client_socket: socket.socket = client_socket
        self.client_address: tuple[str, int] = client_address
        self.client_manager: ClientManager = client_manager
        self.message_manager: MessageManager = message_manager

    def handle(self) -> None:
        try:
            logging.debug("Entering handle() for client %s", self.client_address)
            data: bytes = self.client_socket.recv(1024)

            if not data:
                logging.debug("No data received, returning...")
                return
            
            # שימוש ב-parse_request המעודכן
            client_id, version, request_code, payload = Protocol.parse_request(data)
        
            if client_id and request_code not in [600, 601, 602, 603, 604]:
                return self.send_response(9000, b"Invalid request format")

            if request_code == 600:
                response = self.handle_register(client_id, payload)
            elif request_code == 601:
                response = self.handle_client_list()
            elif request_code == 602:
                response = self.handle_get_public_key(payload)
            elif request_code == 603:
                response = self.handle_send_message(payload)
            elif request_code == 604:
                response = self.handle_fetch_messages(client_id)
            else:
                response = (9000, b"Unknown request code")

            self.send_response(*response)

        except Exception as e:
            logging.exception(f"Exception in handle() for client {self.client_address}: {e}")
            self.send_response(9000, b"Server error in handle()")
        finally:
            self.client_socket.close()


    def handle_register(self, client_id: bytes, payload: bytes) -> tuple[int, bytes]:
        try:
            # בדיקת אורך ה-payload
            if len(payload) != 415:
                print(f"payload length: {len(payload)}")
                return (9000, b"Registration payload must be exactly 415 bytes")

            name_bytes = payload[:255]
 
            pubkey_bytes = payload[255:415]

            name_str = name_bytes.split(b'\0', 1)[0].decode('ascii', errors='ignore')

            if self.client_manager.client_exists_by_username(name_str):
                return (9000, b"Username already taken")
            
            new_id_bytes = uuid.uuid4().bytes  # 16 bytes (random)

            #new_id_str = new_id_bytes.hex()  

            self.client_manager.add_client(new_id_bytes, name_str, pubkey_bytes)
            print(f"public_key_size: {len(pubkey_bytes)}")
            
            return (2100, new_id_bytes)  
          
        except Exception as e:
            return (9000, f"Failed to register: {e}".encode())

    def handle_client_list(self) -> tuple[int, bytes]:
        clients: list[tuple[bytes, str]] = self.client_manager.get_all_clients()

        response = b"".join(
            struct.pack("16s 255s", c[0], c[1].encode())
            for c in clients
        )

        if not response:
            return (9000, b"No clients found")
        return (2101, response)
    
    def handle_get_public_key(self, payload: bytes) -> tuple[int, bytes]:

        try:
            if len(payload) < 16:
                return (9000, b"Invalid ID length")
            
            client_id_bytes = payload[:16]

            public_key = self.client_manager.get_public_key(client_id_bytes)
            print(F"public_key_size: {len(public_key)}")

            if not public_key:
                return (9000, b"Client not found")
            print("Returning public key to client.")
            return (2102, public_key)
        
        except Exception as e:
            return (9000, f"Failed to fetch public key: {e}".encode())

    def handle_send_message(self, payload: bytes) -> tuple[int, bytes]:
        """Handles sending a message (Code 603)."""
        try:
            # 1) פריסת 37 הבתים הראשונים
            # unpack("16s16sBI") -> 16 בתים (to_client), 16 בתים (from_client), 1 בית (message_type), 4 בתים (content_size)
            to_client, from_client, message_type, content_size = struct.unpack("<16s16sBI", payload[:37])
            content = payload[37:]  # שאר התוכן

            # 2) הסרת אפסים (NULL) וקבלת מחרוזת
           # to_client_str = to_client.decode('ascii', errors='ignore').strip('\x00')
           # from_client_str = from_client.decode('ascii', errors='ignore').strip('\x00')

            # 3) הוספת ההודעה למסד הנתונים
            self.message_manager.add_message(
                to_client,
                from_client,
                message_type,
                content
            )

            # 4) מחזירים תשובה 2103
            # כרגע מחזירים "1" כ-Message ID placeholder
            # אם רוצים להחזיר את ה-ID האמיתי, אפשר לאחר הוספה במסד לקרוא אותו.
            return (2103, struct.pack("16sI", to_client, 1))

        except Exception as e:
            return (9000, f"server responded with an error: Failed to send message: {e}".encode())

    def handle_fetch_messages(self, client_id: str) -> tuple[int, bytes]:
        """Handles fetching waiting messages (Code 604)."""
        try:
            if not self.client_manager.client_exists_by_id(client_id):
                return (9000, b"server responded with an error: Client not found")
            messages: list[tuple[int, str, int, bytes]] = self.message_manager.get_messages_for_client(client_id)
            response: bytes = b"".join(struct.pack("<16s I B I", msg[1], msg[0], msg[2], len(msg[3])) + msg[3] for msg in messages)
            for msg in messages:
                self.message_manager.delete_message(msg[0])
            return (2104, response)
        except Exception as e:
            return (9000, f"server responded with an error: Failed to fetch messages: {e}".encode())

    def send_response(self, response_code: int, payload: bytes) -> None:
        """Sends a response using the updated Protocol.create_response."""
        try:
            response: bytes = Protocol.create_response(1, response_code, payload)
            self.client_socket.sendall(response)
        except Exception as e:
            print(f"Error sending response to {self.client_address}: {e}")
    
    def get_public_key_by_id(self, client_id: str) -> bytes | None:
        """Fetches the public key of a client by ID."""
        return self.client_manager.get_public_key(client_id)
