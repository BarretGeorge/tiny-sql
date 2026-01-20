#!/usr/bin/env python3
"""
Integration test for Tiny-SQL with parser
"""
import socket
import struct
import time

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

def test_connection():
    """测试连接和SQL解析器"""
    print("=" * 60)
    print("Tiny-SQL Integration Test")
    print("=" * 60)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 13306))
    print("✅ Connected to server")

    # 接收握手包
    handshake = recv_mysql_packet(sock)
    print(f"✅ Received handshake: {len(handshake)} bytes")

    # 发送认证响应（root用户，空密码）
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

    # 接收认证结果
    auth_result = recv_mysql_packet(sock)
    if auth_result and auth_result[0] == 0:
        print("✅ Authentication successful")
    else:
        print("❌ Authentication failed")
        sock.close()
        return

    # 测试各种SQL语句
    test_queries = [
        "SELECT * FROM users",
        "SHOW TABLES",
        "SHOW DATABASES",
        "USE test",
        "DROP TABLE users",
        "CREATE TABLE products (id INT AUTO_INCREMENT PRIMARY KEY, name TEXT NOT NULL)",
    ]

    for query in test_queries:
        print(f"\n{'-' * 60}")
        print(f"Testing: {query}")
        print(f"{'-' * 60}")

        # 发送查询 (COM_QUERY = 0x03)
        query_payload = struct.pack('B', 0x03) + query.encode('utf-8')
        send_mysql_packet(sock, 0, query_payload)

        # 接收响应
        response = recv_mysql_packet(sock)

        if response:
            packet_type = response[0]
            if packet_type == 0x00:  # OK packet
                print(f"✅ OK response: {len(response)} bytes")
                # 尝试读取消息
                if len(response) > 7:
                    try:
                        msg = response[7:].decode('utf-8', errors='ignore')
                        if msg:
                            print(f"   Message: {msg}")
                    except:
                        pass
            elif packet_type == 0xff:  # ERR packet
                print(f"❌ Error response: {len(response)} bytes")
                if len(response) > 9:
                    error_msg = response[9:].decode('utf-8', errors='ignore')
                    print(f"   Error: {error_msg}")
            else:
                print(f"⚠ Response type: 0x{packet_type:02x}, {len(response)} bytes")
        else:
            print("❌ No response received")

        time.sleep(0.1)

    sock.close()
    print(f"\n{'=' * 60}")
    print("Test completed")
    print(f"{'=' * 60}")

if __name__ == "__main__":
    try:
        test_connection()
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
