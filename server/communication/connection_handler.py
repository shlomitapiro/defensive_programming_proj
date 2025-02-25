import socket
import struct
from communication.protocol import Protocol
from data.client_manager import ClientManager
from data.message_manager import MessageManager
from encryption.encryption_manager import EncryptionManager

class ConnectionHandler:
    def __init__(self, client_socket: socket.socket, client_address: tuple[str, int], 
                 client_manager: ClientManager, message_manager: MessageManager, 
                 encryption_manager: EncryptionManager) -> None:
        """Handles a single client connection."""
        self.client_socket: socket.socket = client_socket
        self.client_address: tuple[str, int] = client_address
        self.client_manager: ClientManager = client_manager
        self.message_manager: MessageManager = message_manager
        self.encryption_manager: EncryptionManager = encryption_manager

    def handle(self) -> None:
        """Handles incoming requests from the client."""
        try:
            data: bytes = self.client_socket.recv(1024)
            if not data:
                return

            client_id: str | None
            version: int | None
            request_code: int | None
            payload: bytes | None
            client_id, version, request_code, payload = Protocol.parse_request(data)

            if not client_id:
                return self.send_response(9000, b"Invalid request format")

            response: tuple[int, bytes]
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
            print(f"Error handling client {self.client_address}: {e}")
            self.send_response(9000, b"Server error")

        finally:
            self.client_socket.close()

    def handle_register(self, client_id: str, payload: bytes) -> tuple[int, bytes]:
        """Handles client registration (Code 600)."""
        try:
            name: str
            public_key: str
            name, public_key = payload.decode().split("|")
            if self.client_manager.client_exists_by_username(name):
                return (9000, b"Username already taken")

            self.client_manager.add_client(client_id, name, public_key.encode())
            return (2100, struct.pack("16s", client_id.encode()))
        except Exception as e:
            return (9000, f"Failed to register: {e}".encode())

    def handle_client_list(self) -> tuple[int, bytes]:
        """Handles client list request (Code 601)."""
        clients: list[tuple[str, str]] = self.client_manager.get_all_clients()
        response: bytes = b"".join(struct.pack("16s 255s", c[0].encode(), c[1].encode()) for c in clients)
        return (2101, response)

    def handle_get_public_key(self, payload: bytes) -> tuple[int, bytes]:
        """Handles fetching a client's public key (Code 602)."""
        try:
            client_id: str = payload.decode()
            public_key: bytes | None = self.client_manager.get_public_key(client_id)
            if not public_key:
                return (9000, b"Client not found")
            return (2102, struct.pack("16s 160s", client_id.encode(), public_key))
        except Exception as e:
            return (9000, f"Failed to fetch public key: {e}".encode())

    def handle_send_message(self, payload: bytes) -> tuple[int, bytes]:
        """Handles sending a message (Code 603)."""
        try:
            to_client: str
            from_client: str
            message_type: int
            content_size: int
            to_client, from_client, message_type, content_size = struct.unpack("16s 16s B I", payload[:37])
            content: bytes = payload[37:]

            self.message_manager.add_message(to_client.decode().strip('\x00'), from_client.decode().strip('\x00'), message_type, content)
            return (2103, struct.pack("16s I", to_client.encode(), 1))  # Message ID placeholder
        except Exception as e:
            return (9000, f"Failed to send message: {e}".encode())

    def handle_fetch_messages(self, client_id: str) -> tuple[int, bytes]:
        """Handles fetching waiting messages (Code 604)."""
        try:
            if not self.client_manager.client_exists_by_id(client_id):
                return (9000, b"Client not found")

            messages: list[tuple[int, str, int, bytes]] = self.message_manager.get_messages_for_client(client_id)
            response: bytes = b"".join(struct.pack("16s I B I", msg[1].encode(), msg[0], msg[2], len(msg[3])) + msg[3] for msg in messages)

            for msg in messages:
                self.message_manager.delete_message(msg[0])

            return (2104, response)
        except Exception as e:
            return (9000, f"Failed to fetch messages: {e}".encode())

    def send_response(self, response_code: int, payload: bytes) -> None:
        """Sends a response using the Protocol class."""
        try:
            response: bytes = Protocol.create_response(1, response_code, payload)
            self.client_socket.sendall(response)
        except Exception as e:
            print(f"Error sending response to {self.client_address}: {e}")