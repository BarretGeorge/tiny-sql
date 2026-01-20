#!/usr/bin/env python3
"""
使用官方 mysql-connector-python 连接 Tiny-SQL
安装: pip install mysql-connector-python
"""

import mysql.connector
from mysql.connector import Error

def connect_to_tiny_sql():
    """连接到Tiny-SQL服务器"""
    try:
        # 创建连接
        connection = mysql.connector.connect(
            host='localhost',
            port=3307,
            user='root',
            password='',  # 空密码
            database='test'
        )

        if connection.is_connected():
            db_info = connection.get_server_info()
            print(f"✓ 成功连接到Tiny-SQL服务器")
            print(f"  服务器版本: {db_info}")

            # 创建游标
            cursor = connection.cursor()

            # 执行查询
            print("\n执行查询测试:")

            # 测试1: SELECT 1
            cursor.execute("SELECT 1")
            result = cursor.fetchone()
            print(f"  SELECT 1 结果: {result}")

            # 测试2: 查看版本
            cursor.execute("SELECT VERSION()")
            result = cursor.fetchone()
            print(f"  VERSION() 结果: {result}")

            # 测试3: 查看当前数据库
            cursor.execute("SELECT DATABASE()")
            result = cursor.fetchone()
            print(f"  DATABASE() 结果: {result}")

            # 测试4: SHOW DATABASES
            cursor.execute("SHOW DATABASES")
            result = cursor.fetchall()
            print(f"  SHOW DATABASES 结果: {result}")

            # 关闭游标
            cursor.close()

    except Error as e:
        print(f"✗ 连接错误: {e}")

    finally:
        if connection and connection.is_connected():
            connection.close()
            print("\n✓ 连接已关闭")

def test_different_users():
    """测试不同的用户连接"""
    users = [
        ('root', ''),
        ('test', 'test'),
        ('admin', 'admin123'),
    ]

    print("\n测试不同用户连接:")
    print("=" * 50)

    for username, password in users:
        try:
            connection = mysql.connector.connect(
                host='localhost',
                port=3307,
                user=username,
                password=password
            )

            if connection.is_connected():
                print(f"✓ 用户 '{username}' 连接成功")
                connection.close()
        except Error as e:
            print(f"✗ 用户 '{username}' 连接失败: {e}")

def main():
    print("Tiny-SQL 客户端测试 (mysql-connector-python)")
    print("=" * 50)

    # 基本连接测试
    connect_to_tiny_sql()

    # 测试不同用户
    test_different_users()

if __name__ == '__main__':
    main()
