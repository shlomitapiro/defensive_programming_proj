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
        """Handles a single client connection."""
        self.client_socket: socket.socket = client_socket
        self.client_address: tuple[str, int] = client_address
        self.client_manager: ClientManager = client_manager
        self.message_manager: MessageManager = message_manager

    def handle(self) -> None:
        """Handles incoming requests from the client."""
        try:
            logging.debug("Entering handle() for client %s", self.client_address)
            data: bytes = self.client_socket.recv(1024)

            if not data:
                logging.debug("No data received, returning...")
                return
            
            # שימוש ב-parse_request המעודכן
            client_id, version, request_code, payload = Protocol.parse_request(data)

            if client_id and request_code != 600:
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
                response = (9000, b"server responded with an error: Unknown request code")

            self.send_response(*response)

        except Exception as e:
            logging.exception(f"Exception in handle() for client {self.client_address}: {e}")
            self.send_response(9000, b"server responded with an error: Server error in handle()")
        finally:
            self.client_socket.close()


    def handle_register(self, client_id: str, payload: bytes) -> tuple[int, bytes]:
        """Handles client registration (Code 600) with a fixed-size payload of 415 bytes:
        255 bytes for the username (null-terminated ASCII), and 160 bytes for the public key (binary)."""
        try:
            # בדיקת אורך ה-payload
            if len(payload) > 415:
                return (9000, b"server responded with an error: Registration payload too short")

            name_bytes = payload[:255]
 
            pubkey_bytes = payload[255:415]

            name_str = name_bytes.split(b'\0', 1)[0].decode('ascii', errors='ignore')

            if self.client_manager.client_exists_by_username(name_str):
                return (9000, b"server responded with an error: Username already taken")
            
            new_id_bytes = uuid.uuid4().bytes  # 16 bytes (random)

            #new_id_str = new_id_bytes.hex()  

            self.client_manager.add_client(new_id_bytes, name_str, pubkey_bytes)
            
            return (2100, new_id_bytes)  
          
        except Exception as e:
            return (9000, f"server responded with an error: Failed to register: {e}".encode())

    def handle_client_list(self) -> tuple[int, bytes]:
        """
        Handles client list request (Code 601).
        Each record: 16 bytes for ID (raw) + 255 bytes for username.
        """
        clients: list[tuple[bytes, str]] = self.client_manager.get_all_clients()
        # clients: list of (id_bytes, user_name)
        print(f"Sending client list: {clients}")
        # כעת c[0] -> bytes(16), c[1] -> str
        # struct.pack("16s255s", c[0], c[1].encode()) -> מחזיר 16 + 255 בתים
        response = b"".join(
            struct.pack("16s 255s", c[0], c[1].encode())
            for c in clients
        )
        return (2101, response)
    
    def handle_get_public_key(self, payload: bytes) -> tuple[int, bytes]:
        """
        Handles fetching a client's public key (Code 602).
        Expects a 16-byte payload (raw bytes).
        """
        try:
            if len(payload) < 16:
                return (9000, b"server responded with an error: Invalid ID length")
            
            client_id_bytes = payload[:16]  # 16 בתים גולמיים
            print(f"Fetching public key for client {client_id_bytes}...")

            public_key = self.client_manager.get_public_key(client_id_bytes)

            print(f"Public key for client {client_id_bytes}: {public_key}")
            if not public_key:
                return (9000, b"server responded with an error: Client not found")
            
            # מחזירים 2102, רק את המפתח הציבורי (או struct.pack אם תרצה לצרף עוד שדות)
            return (2102, public_key)
        
        except Exception as e:
            return (9000, f"server responded with an error: Failed to fetch public key: {e}".encode())

    def handle_send_message(self, payload: bytes) -> tuple[int, bytes]:
        """Handles sending a message (Code 603)."""
        try:
            to_client: str
            from_client: str
            message_type: int
            content_size: int
            to_client, from_client, message_type, content_size = struct.unpack("16s 16s B I", payload[:37])
            content: bytes = payload[37:]
            self.message_manager.add_message(to_client.decode().strip('\x00'),
                                             from_client.decode().strip('\x00'),
                                             message_type, content)
            return (2103, struct.pack("16s I", to_client.encode(), 1))  # Message ID placeholder
        except Exception as e:
            return (9000, f"server responded with an error: Failed to send message: {e}".encode())

    def handle_fetch_messages(self, client_id: str) -> tuple[int, bytes]:
        """Handles fetching waiting messages (Code 604)."""
        try:
            if not self.client_manager.client_exists_by_id(client_id):
                return (9000, b"server responded with an error: Client not found")
            messages: list[tuple[int, str, int, bytes]] = self.message_manager.get_messages_for_client(client_id)
            response: bytes = b"".join(struct.pack("16s I B I", msg[1].encode(), msg[0], msg[2], len(msg[3])) + msg[3] for msg in messages)
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
