#!/usr/bin/env python3
"""
测试存储引擎功能
"""
import socket
import struct

def send_mysql_packet(sock, sequence_id, payload):
    """发送MySQL包"""
    length = len(payload)
    header = struct.pack('<I', length)[0:3] + struct.pack('B', sequence_id)
    sock.send(header + payload)

def recv_mysql_packet(sock):
    """接收MySQL包"""
    header = sock.recv(4)
    if len(header) < 4:
        return None

    length = struct.unpack('<I', header[0:3] + b'\x00')[0]
    sequence_id = header[3]

    payload = b''
    while len(payload) < length:
        chunk = sock.recv(length - len(payload))
        if not chunk:
            break
        payload += chunk

    return payload

def test_storage():
    """测试存储引擎"""
    print("=" * 60)
    print("Tiny-SQL Storage Engine Test")
    print("=" * 60)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 13306))
    print("✅ Connected to server")

    # 接收握手包
    handshake = recv_mysql_packet(sock)
    print(f"✅ Received handshake")

    # 发送认证响应
    capabilities = 0x0000f7cf
    max_packet = 16777216
    charset = 33

    auth_response = struct.pack('<I', capabilities)
    auth_response += struct.pack('<I', max_packet)
    auth_response += struct.pack('<B', charset)
    auth_response += b'\x00' * 23
    auth_response += b'root\x00'
    auth_response += b'\x00'
    auth_response += b'mysql_native_password\x00'

    send_mysql_packet(sock, 1, auth_response)
    auth_result = recv_mysql_packet(sock)
    if auth_result and auth_result[0] == 0:
        print("✅ Authentication successful")
    else:
        print("❌ Authentication failed")
        sock.close()
        return

    # 测试序列
    test_queries = [
        ("USE test", "Switch to test database"),
        ("CREATE TABLE users (id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(50), age INT)", "Create users table"),
        ("SHOW TABLES", "List tables"),
        ("INSERT INTO users (name, age) VALUES ('Alice', 25)", "Insert Alice"),
        ("INSERT INTO users (name, age) VALUES ('Bob', 30)", "Insert Bob"),
        ("INSERT INTO users (name, age) VALUES ('Charlie', 35)", "Insert Charlie"),
        ("SHOW DATABASES", "List databases"),
    ]

    for query, description in test_queries:
        print(f"\n{'-' * 60}")
        print(f"Test: {description}")
        print(f"SQL:  {query}")
        print(f"{'-' * 60}")

        # 发送查询
        query_payload = struct.pack('B', 0x03) + query.encode('utf-8')
        send_mysql_packet(sock, 0, query_payload)

        # 接收响应
        response = recv_mysql_packet(sock)

        if response:
            packet_type = response[0]
            if packet_type == 0x00:  # OK packet
                print(f"✅ Success")
                if len(response) > 7:
                    try:
                        msg = response[7:].decode('utf-8', errors='ignore')
                        if msg:
                            print(f"   {msg}")
                    except:
                        pass
            elif packet_type == 0xff:  # ERR packet
                print(f"❌ Error")
                if len(response) > 9:
                    error_msg = response[9:].decode('utf-8', errors='ignore')
                    print(f"   {error_msg}")
            else:
                print(f"⚠ Response type: 0x{packet_type:02x}")
        else:
            print("❌ No response")

    sock.close()
    print(f"\n{'=' * 60}")
    print("Test completed!")
    print(f"{'=' * 60}")

if __name__ == "__main__":
    try:
        test_storage()
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
