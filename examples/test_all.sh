#!/bin/bash
# 测试所有客户端示例

set -e

echo "Tiny-SQL 客户端示例测试"
echo "========================================"
echo ""

# 检查服务器是否运行
echo "检查 Tiny-SQL 服务器..."
if ! pgrep -f "tiny-sql" > /dev/null; then
    echo "⚠ Tiny-SQL 服务器未运行"
    echo "请先启动服务器: cd ../build && ./tiny-sql 3307"
    exit 1
fi
echo "✓ 服务器正在运行"
echo ""

SUCCESS=0
FAILED=0

# 测试 Python (mysql-connector-python)
echo "1. 测试 Python (mysql-connector-python)..."
echo "========================================"
if command -v python3 &> /dev/null && python3 -c "import mysql.connector" 2>/dev/null; then
    if python3 python_mysql_connector.py; then
        echo "✓ Python mysql-connector 测试通过"
        ((SUCCESS++))
    else
        echo "✗ Python mysql-connector 测试失败"
        ((FAILED++))
    fi
else
    echo "⚠ 跳过（未安装 mysql-connector-python）"
fi
echo ""

# 测试 Python (PyMySQL)
echo "2. 测试 Python (PyMySQL)..."
echo "========================================"
if command -v python3 &> /dev/null && python3 -c "import pymysql" 2>/dev/null; then
    if python3 python_pymysql.py; then
        echo "✓ Python PyMySQL 测试通过"
        ((SUCCESS++))
    else
        echo "✗ Python PyMySQL 测试失败"
        ((FAILED++))
    fi
else
    echo "⚠ 跳过（未安装 pymysql）"
fi
echo ""

# 测试 Node.js
echo "3. 测试 Node.js (mysql2)..."
echo "========================================"
if command -v node &> /dev/null && [ -d "node_modules" ]; then
    if node nodejs_mysql_client.js; then
        echo "✓ Node.js 测试通过"
        ((SUCCESS++))
    else
        echo "✗ Node.js 测试失败"
        ((FAILED++))
    fi
else
    echo "⚠ 跳过（未安装 Node.js 或依赖）"
fi
echo ""

# 测试 Go
echo "4. 测试 Go..."
echo "========================================"
if command -v go &> /dev/null; then
    if go run go_mysql_client.go; then
        echo "✓ Go 测试通过"
        ((SUCCESS++))
    else
        echo "✗ Go 测试失败"
        ((FAILED++))
    fi
else
    echo "⚠ 跳过（未安装 Go）"
fi
echo ""

# 测试 Java
echo "5. 测试 Java..."
echo "========================================"
if command -v javac &> /dev/null && [ -f "mysql-connector-java-8.0.33.jar" ]; then
    if javac -cp mysql-connector-java-8.0.33.jar TinySQLClient.java && \
       java -cp .:mysql-connector-java-8.0.33.jar TinySQLClient; then
        echo "✓ Java 测试通过"
        ((SUCCESS++))
    else
        echo "✗ Java 测试失败"
        ((FAILED++))
    fi
else
    echo "⚠ 跳过（未安装 Java 或 MySQL Connector/J）"
fi
echo ""

# 总结
echo "========================================"
echo "测试总结"
echo "========================================"
echo "成功: $SUCCESS"
echo "失败: $FAILED"
echo "跳过: $((5 - SUCCESS - FAILED))"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "✓ 所有测试通过！"
    exit 0
else
    echo "✗ 有测试失败"
    exit 1
fi
