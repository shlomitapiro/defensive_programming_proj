import struct

class Protocol:
    # השתמש בסימן "<" כדי להבטיח סדר little-endian בכל הפעולות.
    REQUEST_HEADER_FORMAT = "<16s B H I"  # 16 בתים ל־Client ID, 1 בית לגרסה, 2 בתים לקוד הבקשה, 4 בתים לגודל ה-payload
    RESPONSE_HEADER_FORMAT = "<B H I"     # 1 בית לגרסה, 2 בתים לקוד התגובה, 4 בתים לגודל ה-payload

    @staticmethod
    def create_request(client_id: str, version: int, request_code: int, payload: bytes) -> bytes:
        """
        יוצר בקשה (Request) לפי הפורמט:
        - Client ID: בדיוק 16 בתים (אם קצר, מתמלא ב-null; אם ארוך, נחתך)
        - Version: 1 בית
        - Request Code: 2 בתים, little-endian
        - Payload Size: 4 בתים, little-endian
        ואז את ה-payload.
        """
        cid = client_id.encode('ascii')
        if len(cid) < 16:
            cid = cid.ljust(16, b'\x00')
        elif len(cid) > 16:
            cid = cid[:16]
        payload_size = len(payload)
        header = struct.pack(Protocol.REQUEST_HEADER_FORMAT, cid, version, request_code, payload_size)
        return header + payload

    @staticmethod
    def parse_request(data: bytes) -> tuple[str, int, int, bytes]:
        """
        מנתח בקשה ומחזיר tuple: (client_id, version, request_code, payload).
        אם הנתונים קצרים מדי, זורק חריגה.
        """
        header_size = struct.calcsize(Protocol.REQUEST_HEADER_FORMAT)
        if len(data) < header_size:
            raise ValueError("Data too short for header")
        cid, version, request_code, payload_size = struct.unpack(Protocol.REQUEST_HEADER_FORMAT, data[:header_size])
        payload = data[header_size:header_size + payload_size]
        return cid.decode('ascii').rstrip('\x00'), version, request_code, payload

    @staticmethod
    def create_response(version: int, response_code: int, payload: bytes) -> bytes:
        """
        יוצר תגובה (Response) לפי הפורמט:
        - Version: 1 בית
        - Response Code: 2 בתים, little-endian
        - Payload Size: 4 בתים, little-endian
        ואז את ה-payload.
        """
        payload_size = len(payload)
        header = struct.pack(Protocol.RESPONSE_HEADER_FORMAT, version, response_code, payload_size)
        return header + payload

    @staticmethod
    def parse_response(data: bytes) -> tuple[int, int, bytes]:
        """
        מנתח תגובה ומחזיר tuple: (version, response_code, payload).
        אם הנתונים קצרים מדי, זורק חריגה.
        """
        header_size = struct.calcsize(Protocol.RESPONSE_HEADER_FORMAT)
        if len(data) < header_size:
            raise ValueError("Data too short for response header")
        version, response_code, payload_size = struct.unpack(Protocol.RESPONSE_HEADER_FORMAT, data[:header_size])
        payload = data[header_size:header_size + payload_size]
        return version, response_code, payload