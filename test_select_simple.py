#!/usr/bin/env python3
"""
Simple test to verify SELECT implementation
"""

import mysql.connector
import time

def test_select():
    print("Connecting to tiny-sql server...")

    try:
        # Connect to the server
        conn = mysql.connector.connect(
            host='127.0.0.1',
            port=13306,
            user='root',
            password='root',
            database='test'
        )

        cursor = conn.cursor()

        # Create a test table
        print("\n1. Creating table...")
        cursor.execute("CREATE TABLE users (id INT, name VARCHAR(50), age INT)")
        print("   Table created successfully")

        # Insert some data
        print("\n2. Inserting data...")
        cursor.execute("INSERT INTO users VALUES (1, 'Alice', 25)")
        cursor.execute("INSERT INTO users VALUES (2, 'Bob', 30)")
        cursor.execute("INSERT INTO users VALUES (3, 'Charlie', 35)")
        print("   Data inserted successfully")

        # Test SELECT *
        print("\n3. Testing SELECT * FROM users...")
        cursor.execute("SELECT * FROM users")
        rows = cursor.fetchall()
        print(f"   Result: {len(rows)} rows")
        for row in rows:
            print(f"   {row}")

        # Test SELECT with specific columns
        print("\n4. Testing SELECT id, name FROM users...")
        cursor.execute("SELECT id, name FROM users")
        rows = cursor.fetchall()
        print(f"   Result: {len(rows)} rows")
        for row in rows:
            print(f"   {row}")

        # Test SELECT with WHERE
        print("\n5. Testing SELECT * FROM users WHERE age > 25...")
        cursor.execute("SELECT * FROM users WHERE age > 25")
        rows = cursor.fetchall()
        print(f"   Result: {len(rows)} rows")
        for row in rows:
            print(f"   {row}")

        # Test SELECT with LIMIT
        print("\n6. Testing SELECT * FROM users LIMIT 2...")
        cursor.execute("SELECT * FROM users LIMIT 2")
        rows = cursor.fetchall()
        print(f"   Result: {len(rows)} rows")
        for row in rows:
            print(f"   {row}")

        print("\n✓ All tests passed!")

        cursor.close()
        conn.close()

    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    # Give server time to start if needed
    time.sleep(1)
    test_select()
