#!/usr/bin/env python3
"""
使用 PyMySQL 连接 Tiny-SQL
安装: pip install pymysql
"""

import pymysql
from pymysql import Error

def connect_with_pymysql():
    """使用PyMySQL连接到Tiny-SQL"""
    connection = None

    try:
        # 创建连接
        connection = pymysql.connect(
            host='localhost',
            port=3307,
            user='root',
            password='',
            database='test',
            charset='utf8mb4',
            cursorclass=pymysql.cursors.DictCursor
        )

        print("✓ 成功连接到Tiny-SQL服务器 (PyMySQL)")
        print(f"  连接ID: {connection.thread_id()}")

        # 创建游标
        with connection.cursor() as cursor:
            # 测试查询
            print("\n执行查询测试:")

            # Ping测试
            connection.ping(reconnect=False)
            print("  ✓ PING 成功")

            # SELECT 1
            cursor.execute("SELECT 1 as result")
            result = cursor.fetchone()
            print(f"  SELECT 1 结果: {result}")

            # 查看版本
            cursor.execute("SELECT VERSION() as version")
            result = cursor.fetchone()
            print(f"  版本信息: {result}")

            # 查看当前数据库
            cursor.execute("SELECT DATABASE() as db")
            result = cursor.fetchone()
            print(f"  当前数据库: {result}")

    except Error as e:
        print(f"✗ 错误: {e}")

    finally:
        if connection:
            connection.close()
            print("\n✓ 连接已关闭")

def test_connection_context_manager():
    """测试使用上下文管理器"""
    print("\n测试上下文管理器:")
    print("=" * 50)

    try:
        with pymysql.connect(
            host='localhost',
            port=3307,
            user='test',
            password='test'
        ) as connection:
            with connection.cursor() as cursor:
                cursor.execute("SELECT 1")
                result = cursor.fetchone()
                print(f"✓ 上下文管理器工作正常: {result}")

    except Error as e:
        print(f"✗ 错误: {e}")

def test_execute_many():
    """测试批量执行"""
    print("\n测试批量查询:")
    print("=" * 50)

    try:
        connection = pymysql.connect(
            host='localhost',
            port=3307,
            user='root',
            password=''
        )

        with connection.cursor() as cursor:
            queries = [
                "SELECT 1",
                "SELECT VERSION()",
                "SELECT DATABASE()"
            ]

            for query in queries:
                cursor.execute(query)
                result = cursor.fetchone()
                print(f"  {query}: {result}")

        connection.close()
        print("✓ 批量查询完成")

    except Error as e:
        print(f"✗ 错误: {e}")

def main():
    print("Tiny-SQL 客户端测试 (PyMySQL)")
    print("=" * 50)

    # 基本连接测试
    connect_with_pymysql()

    # 上下文管理器测试
    test_connection_context_manager()

    # 批量执行测试
    test_execute_many()

if __name__ == '__main__':
    main()
