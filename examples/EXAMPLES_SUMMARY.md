# Tiny-SQL 客户端示例总览

## 📁 文件列表

```
examples/
├── README.md                      # 详细文档
├── EXAMPLES_SUMMARY.md           # 本文件（快速总览）
├── setup.sh                      # 依赖安装脚本
├── test_all.sh                   # 自动测试所有客户端
├── requirements.txt              # Python依赖
├── package.json                  # Node.js依赖
├── go.mod                        # Go模块定义
├── .gitignore                    # Git忽略文件
│
├── python_mysql_connector.py     # Python官方驱动示例
├── python_pymysql.py             # PyMySQL驱动示例
├── nodejs_mysql_client.js        # Node.js示例
├── go_mysql_client.go            # Go语言示例
└── java_mysql_client.java        # Java JDBC示例
```

## 🚀 快速开始

### 1. 确保服务器运行

```bash
cd /workspaces/code/tiny-sql/build
./tiny-sql 3307
```

### 2. 选择你喜欢的语言

#### Python (mysql-connector-python)

```bash
# 安装
pip install mysql-connector-python

# 运行
python3 python_mysql_connector.py
```

**特点**：MySQL官方驱动，功能完整

#### Python (PyMySQL)

```bash
# 安装
pip install pymysql

# 运行
python3 python_pymysql.py
```

**特点**：纯Python实现，轻量级

#### Node.js

```bash
# 安装
npm install mysql2

# 运行
node nodejs_mysql_client.js
```

**特点**：支持Promise/async-await，高性能

#### Go

```bash
# 安装依赖
go get -u github.com/go-sql-driver/mysql

# 运行
go run go_mysql_client.go
```

**特点**：高性能，并发安全，连接池支持

#### Java

```bash
# 下载驱动
wget https://repo1.maven.org/maven2/mysql/mysql-connector-java/8.0.33/mysql-connector-java-8.0.33.jar

# 编译
javac -cp mysql-connector-java-8.0.33.jar TinySQLClient.java

# 运行
java -cp .:mysql-connector-java-8.0.33.jar TinySQLClient
```

**特点**：JDBC标准，企业级

## 📊 功能对比

| 功能 | Python | Node.js | Go | Java |
|------|--------|---------|----|----- |
| 基本连接 | ✅ | ✅ | ✅ | ✅ |
| 查询执行 | ✅ | ✅ | ✅ | ✅ |
| 预处理语句 | ✅ | ✅ | ✅ | ✅ |
| 连接池 | ✅ | ✅ | ✅ | ✅ |
| 事务支持 | ✅ | ✅ | ✅ | ✅ |
| Async/Promise | ⚠️ | ✅ | ✅ | ⚠️ |
| 并发查询 | ⚠️ | ✅ | ✅ | ✅ |

## 🎯 每个示例包含的测试

1. **基本连接测试**
   - 连接Tiny-SQL服务器
   - 获取服务器版本

2. **查询测试**
   - SELECT 1
   - SELECT VERSION()
   - SELECT DATABASE()
   - SHOW DATABASES

3. **用户认证测试**
   - root (空密码)
   - test (密码: test)
   - admin (密码: admin123)

4. **高级功能**（部分驱动）
   - 连接池管理
   - 并发查询
   - 预处理语句
   - 事务处理

## 💡 使用建议

### 选择驱动

- **Python开发**：mysql-connector-python（官方支持）或 PyMySQL（轻量级）
- **Node.js开发**：mysql2（推荐，支持Promise）
- **Go开发**：go-sql-driver/mysql（标准选择）
- **Java开发**：MySQL Connector/J（JDBC标准）

### 生产环境

- 使用连接池管理连接
- 设置合理的超时时间
- 正确处理错误和重连
- 使用预处理语句防止SQL注入

### 性能优化

- 重用连接（使用连接池）
- 批量操作时使用事务
- 适当的并发控制
- 设置合理的连接参数

## 🔧 故障排除

### 连接失败

```bash
# 检查服务器
ps aux | grep tiny-sql

# 检查端口
netstat -tlnp | grep 3307

# 查看日志
tail -f /tmp/server.log
```

### 认证失败

- 确认用户名密码正确
- 检查服务器日志
- 确认支持的认证方法

### 查询失败

- 检查SQL语法
- 查看服务器支持的命令
- 检查错误消息

## 📚 相关资源

- [主项目 README](../README.md)
- [详细示例文档](README.md)
- [MySQL协议文档](https://dev.mysql.com/doc/dev/mysql-server/latest/PAGE_PROTOCOL.html)
- [Tiny-SQL 源码](../)

## 🤝 贡献

欢迎提交更多语言的客户端示例！

支持的语言建议：
- Rust
- PHP
- Ruby
- C#/.NET
- C/C++

---

**提示**：所有示例都设计为独立运行，无需修改即可连接到默认的Tiny-SQL服务器（localhost:3307）。
