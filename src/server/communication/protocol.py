import struct

''' Protocol class is used to create and parse request from client to server and response from server to client.
    The Protocol class is used by both client and server to create and parse request and response.
    The request and response are in the following format:

    Request:
    - Client ID: 16 bytes
    - Version: 1 byte
    - Request Code: 2 bytes
    - Payload Size: 4 bytes
    - Payload: variable length

    Response:
    - Version: 1 byte
    - Response Code: 2 bytes
    - Payload Size: 4 bytes
    - Payload: variable length
    
    The create_request method takes client_id, version, request_code and payload as input and returns serialized request.
    The parse_request method takes serialized request as input and returns client_id, version, request_code and payload.
    The create_response method takes version, response_code and payload as input and returns serialized response.
    The parse_response method takes serialized response as input and returns version, response_code and payload.  
'''

class Protocol:

    REQUEST_HEADER_FORMAT = "<16s B H I"
    RESPONSE_HEADER_FORMAT = "<B H I"

    @staticmethod
    def create_request(client_id: bytes, version: int, request_code: int, payload: bytes) -> bytes:
        cid = client_id
        if len(cid) < 16:
            cid = cid.ljust(16, b'\x00')
        elif len(cid) > 16:
            cid = cid[:16]
        payload_size = len(payload)
        header = struct.pack(Protocol.REQUEST_HEADER_FORMAT, cid, version, request_code, payload_size)
        return header + payload

    @staticmethod
    def parse_request(data: bytes) -> tuple[bytes, int, int, bytes]:
        header_size = struct.calcsize(Protocol.REQUEST_HEADER_FORMAT)
        if len(data) < header_size:
            raise ValueError("Data too short for declared payload")
        
        cid, version, request_code, payload_size = struct.unpack(Protocol.REQUEST_HEADER_FORMAT, data[:header_size])
        
        if len(data) < header_size + payload_size:
            raise ValueError("Data too short for declared payload")
        
        payload = data[header_size:header_size + payload_size]
        return cid, version, request_code, payload

    @staticmethod
    def create_response(version: int, response_code: int, payload: bytes) -> bytes:
        payload_size = len(payload)
        header = struct.pack(Protocol.RESPONSE_HEADER_FORMAT, version, response_code, payload_size)
        return header + payload

    @staticmethod
    def parse_response(data: bytes) -> tuple[int, int, bytes]:
        header_size = struct.calcsize(Protocol.RESPONSE_HEADER_FORMAT)
        if len(data) < header_size:
            raise ValueError("Data too short for response header")
        version, response_code, payload_size = struct.unpack(Protocol.RESPONSE_HEADER_FORMAT, data[:header_size])
        if len(data) < header_size + payload_size:
            raise ValueError("Data too short for declared payload")
        payload = data[header_size:header_size + payload_size]
        return version, response_code, payload