#!/bin/bash
# 设置所有示例客户端的依赖

set -e

echo "设置 Tiny-SQL 客户端示例环境"
echo "========================================"

# Python 依赖
echo ""
echo "1. 安装 Python 依赖..."
if command -v pip3 &> /dev/null; then
    pip3 install -r requirements.txt
    echo "✓ Python 依赖安装完成"
else
    echo "⚠ pip3 未找到，跳过 Python 依赖安装"
fi

# Node.js 依赖
echo ""
echo "2. 安装 Node.js 依赖..."
if command -v npm &> /dev/null; then
    npm install
    echo "✓ Node.js 依赖安装完成"
else
    echo "⚠ npm 未找到，跳过 Node.js 依赖安装"
fi

# Go 依赖
echo ""
echo "3. 安装 Go 依赖..."
if command -v go &> /dev/null; then
    go mod download
    echo "✓ Go 依赖安装完成"
else
    echo "⚠ go 未找到，跳过 Go 依赖安装"
fi

# Java 依赖
echo ""
echo "4. Java 依赖说明..."
echo "请手动下载 MySQL Connector/J:"
echo "  wget https://repo1.maven.org/maven2/mysql/mysql-connector-java/8.0.33/mysql-connector-java-8.0.33.jar"
echo "或使用 Maven/Gradle 管理依赖"

echo ""
echo "========================================"
echo "✓ 环境设置完成！"
echo ""
echo "使用方法："
echo "  Python:  python3 python_mysql_connector.py"
echo "  Node.js: node nodejs_mysql_client.js"
echo "  Go:      go run go_mysql_client.go"
echo "  Java:    javac -cp mysql-connector-java-8.0.33.jar TinySQLClient.java"
echo "           java -cp .:mysql-connector-java-8.0.33.jar TinySQLClient"
