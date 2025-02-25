import struct

class Protocol:
    REQUEST_HEADER_FORMAT = "16s B H I"  # Client ID (16B), Version (1B), Code (2B), Payload Size (4B)
    RESPONSE_HEADER_FORMAT = "B H I"     # Version (1B), Code (2B), Payload Size (4B)

    @staticmethod
    def create_request(client_id, version, request_code, payload):
        """Creates a binary request packet following the protocol format."""
        client_id_packed = client_id.encode().ljust(16, b'\x00')  # Pad or truncate to 16 bytes
        payload_size = len(payload)
        header = struct.pack(Protocol.REQUEST_HEADER_FORMAT, client_id_packed, version, request_code, payload_size)
        return header + payload

    @staticmethod
    def parse_request(data):
        """Parses a binary request packet into its components."""
        try:
            header_size = struct.calcsize(Protocol.REQUEST_HEADER_FORMAT)
            if len(data) < header_size:
                return None, None, None, None

            client_id, version, request_code, payload_size = struct.unpack(Protocol.REQUEST_HEADER_FORMAT, data[:header_size])
            payload = data[header_size:] if payload_size > 0 else b""

            if version != 2:
                print(f"Unsupported protocol version: {version}")
                return None, None, None, None

            return client_id.decode().strip('\x00'), version, request_code, payload
        except Exception as e:
            print(f"Error parsing request: {e}")
            return None, None, None, None

    @staticmethod
    def create_response(version, response_code, payload):
        """Creates a binary response packet following the protocol format."""
        payload_size = len(payload)
        header = struct.pack(Protocol.RESPONSE_HEADER_FORMAT, version, response_code, payload_size)
        return header + payload

    @staticmethod
    def parse_response(data):
        """Parses a binary response packet into its components."""
        try:
            header_size = struct.calcsize(Protocol.RESPONSE_HEADER_FORMAT)
            if len(data) < header_size:
                return None, None, None

            version, response_code, payload_size = struct.unpack(Protocol.RESPONSE_HEADER_FORMAT, data[:header_size])
            payload = data[header_size:] if payload_size > 0 else b""

            return version, response_code, payload
        except Exception as e:
            print(f"Error parsing response: {e}")
            return None, None, None