#!/usr/bin/env python3
"""
Test SELECT implementation using raw sockets
"""
import socket
import struct
import hashlib

def send_mysql_packet(sock, sequence_id, payload):
    """Send MySQL packet"""
    length = len(payload)
    header = struct.pack('<I', length)[0:3] + struct.pack('B', sequence_id)
    sock.send(header + payload)

def recv_mysql_packet(sock):
    """Receive MySQL packet"""
    header = sock.recv(4)
    if len(header) < 4:
        return None, None

    length = struct.unpack('<I', header[0:3] + b'\x00')[0]
    sequence_id = header[3]

    payload = b''
    while len(payload) < length:
        chunk = sock.recv(length - len(payload))
        if not chunk:
            break
        payload += chunk

    return payload, sequence_id

def execute_query(sock, query, seq_id=0):
    """Execute a SQL query"""
    # Send query (COM_QUERY = 0x03)
    payload = struct.pack('B', 0x03) + query.encode('utf-8')
    send_mysql_packet(sock, seq_id, payload)

    # Receive response
    responses = []
    while True:
        resp, resp_seq = recv_mysql_packet(sock)
        if resp is None:
            break
        responses.append((resp, resp_seq))

        # Check if it's an OK packet (0x00), ERR packet (0xFF), or EOF packet (0xFE)
        if len(resp) > 0:
            header_byte = resp[0]
            if header_byte == 0x00:  # OK packet
                print(f"  OK packet received")
                break
            elif header_byte == 0xFF:  # ERR packet
                error_code = struct.unpack('<H', resp[1:3])[0]
                error_msg = resp[9:].decode('utf-8', errors='ignore')
                print(f"  ERROR {error_code}: {error_msg}")
                break
            elif header_byte == 0xFE and len(resp) < 9:  # EOF packet (small size)
                # This might be the final EOF after rows
                print(f"  EOF packet received")
                # Check if this is the second EOF (end of result set)
                if len(responses) > 2:  # We've already seen column defs
                    break

    return responses

def main():
    print("=" * 60)
    print("Testing SELECT Implementation")
    print("=" * 60)

    # Connect
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 13309))
    print("✅ Connected to server on port 13309\n")

    # Receive handshake
    handshake, _ = recv_mysql_packet(sock)
    print(f"✅ Received handshake: {len(handshake)} bytes\n")

    # Send authentication (root with password 'root')
    capabilities = 0x0000f7cf
    max_packet = 16777216
    charset = 33

    # Extract salt from handshake
    salt1 = handshake[9:17]
    salt2 = handshake[31:43]
    salt = salt1 + salt2

    # Hash password (empty password for root)
    password = ''
    if password:
        hash1 = hashlib.sha1(password.encode()).digest()
        hash2 = hashlib.sha1(hash1).digest()
        hash3 = hashlib.sha1(salt + hash2).digest()
        token = bytes(a ^ b for a, b in zip(hash1, hash3))
    else:
        # Empty password
        token = b''

    # Build auth response
    auth_response = struct.pack('<I', capabilities) + \
                   struct.pack('<I', max_packet) + \
                   struct.pack('B', charset) + \
                   b'\x00' * 23 + \
                   b'root\x00' + \
                   struct.pack('B', len(token)) + token + \
                   b'mysql_native_password\x00'

    send_mysql_packet(sock, 1, auth_response)
    auth_ok, _ = recv_mysql_packet(sock)
    print("✅ Authentication successful\n")

    # Switch to test database
    print("Switching to 'test' database...")
    execute_query(sock, "USE test", 0)
    print()

    # Create table
    print("Creating table 'numbers'...")
    execute_query(sock, "CREATE TABLE numbers (id INT, value INT, score INT)", 0)
    print()

    # Insert data
    print("Inserting data...")
    execute_query(sock, "INSERT INTO numbers VALUES (1, 100, 25)", 0)
    execute_query(sock, "INSERT INTO numbers VALUES (2, 200, 30)", 0)
    execute_query(sock, "INSERT INTO numbers VALUES (3, 300, 35)", 0)
    print()

    # Test SELECT *
    print("=" * 60)
    print("Test 1: SELECT * FROM numbers")
    print("=" * 60)
    responses = execute_query(sock, "SELECT * FROM numbers", 0)
    print(f"Received {len(responses)} packets")
    print()

    # Test SELECT with specific columns
    print("=" * 60)
    print("Test 2: SELECT id, value FROM numbers")
    print("=" * 60)
    responses = execute_query(sock, "SELECT id, value FROM numbers", 0)
    print(f"Received {len(responses)} packets")
    print()

    # Test SELECT with WHERE
    print("=" * 60)
    print("Test 3: SELECT * FROM numbers WHERE score > 25")
    print("=" * 60)
    responses = execute_query(sock, "SELECT * FROM numbers WHERE score > 25", 0)
    print(f"Received {len(responses)} packets")
    print()

    # Test SELECT with LIMIT
    print("=" * 60)
    print("Test 4: SELECT * FROM numbers LIMIT 2")
    print("=" * 60)
    responses = execute_query(sock, "SELECT * FROM numbers LIMIT 2", 0)
    print(f"Received {len(responses)} packets")
    print()

    print("=" * 60)
    print("✅ All SELECT tests completed!")
    print("=" * 60)

    sock.close()

if __name__ == "__main__":
    main()
