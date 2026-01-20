# Tiny-SQL 客户端示例

这个目录包含使用各种官方MySQL驱动连接Tiny-SQL服务器的示例代码。

## 支持的语言和驱动

### 1. Python

#### mysql-connector-python（官方驱动）
```bash
# 安装
pip install mysql-connector-python

# 运行
python3 python_mysql_connector.py
```

**特点**：
- MySQL官方维护
- 纯Python实现
- 支持所有MySQL特性
- 文档完善

#### PyMySQL
```bash
# 安装
pip install pymysql

# 运行
python3 python_pymysql.py
```

**特点**：
- 纯Python实现
- 兼容MySQLdb API
- 轻量级
- 广泛使用

### 2. Go

```bash
# 安装依赖
go get -u github.com/go-sql-driver/mysql

# 编译
go build go_mysql_client.go

# 运行
./go_mysql_client
```

**特点**：
- Go标准database/sql接口
- 高性能
- 支持连接池
- 并发安全

### 3. Node.js

```bash
# 安装依赖
npm install mysql2

# 运行
node nodejs_mysql_client.js
```

**特点**：
- 支持Promise和async/await
- 连接池支持
- 预处理语句
- 高性能

### 4. Java

```bash
# 下载驱动
wget https://repo1.maven.org/maven2/mysql/mysql-connector-java/8.0.33/mysql-connector-java-8.0.33.jar

# 编译
javac -cp mysql-connector-java-8.0.33.jar TinySQLClient.java

# 运行
java -cp .:mysql-connector-java-8.0.33.jar TinySQLClient
```

**特点**：
- JDBC标准接口
- 企业级应用首选
- 完整的事务支持
- 预处理语句

## 功能测试

所有示例都包含以下测试：

1. **基本连接测试**
   - 连接到Tiny-SQL服务器
   - 获取服务器版本信息
   - 执行简单查询

2. **多用户测试**
   - 测试不同用户的连接
   - 验证认证机制

3. **查询测试**
   - SELECT 1
   - SELECT VERSION()
   - SELECT DATABASE()
   - SHOW DATABASES

4. **高级功能**（部分驱动）
   - 连接池
   - 预处理语句
   - 事务处理
   - 并发查询

## 连接参数

### 基本参数
- **Host**: localhost
- **Port**: 3307
- **Database**: test

### 测试用户
| 用户名 | 密码 |
|--------|------|
| root   | (空) |
| test   | test |
| admin  | admin123 |

## 示例输出

```
Tiny-SQL 客户端测试
==================================================
✓ 成功连接到Tiny-SQL服务器
  服务器版本: 1.0.0-tiny-sql

执行查询测试:
  SELECT 1 结果: 1
  VERSION() 结果: 1.0.0-tiny-sql
  DATABASE() 结果: test

测试不同用户:
==================================================
✓ 用户 'root' 连接成功
✓ 用户 'test' 连接成功
✓ 用户 'admin' 连接成功

✓ 连接已关闭
```

## 注意事项

1. **确保服务器运行**
   ```bash
   cd /workspaces/code/tiny-sql/build
   ./tiny-sql 3307
   ```

2. **安装依赖**
   根据使用的语言安装相应的MySQL驱动

3. **防火墙设置**
   确保3307端口可访问

4. **SSL连接**
   当前版本不支持SSL，需要在连接参数中禁用

5. **字符集**
   建议使用UTF-8/UTF8MB4

## 故障排除

### 连接失败
- 检查服务器是否运行：`ps aux | grep tiny-sql`
- 检查端口是否监听：`netstat -tlnp | grep 3307`
- 检查防火墙设置

### 认证失败
- 确认用户名和密码正确
- 查看服务器日志：`tail -f /tmp/server.log`

### 查询失败
- 检查SQL语法
- 查看服务器支持的命令列表
- 查看错误信息

## 扩展阅读

- [MySQL Protocol Documentation](https://dev.mysql.com/doc/dev/mysql-server/latest/PAGE_PROTOCOL.html)
- [Python MySQL Connector](https://dev.mysql.com/doc/connector-python/en/)
- [PyMySQL Documentation](https://pymysql.readthedocs.io/)
- [Go MySQL Driver](https://github.com/go-sql-driver/mysql)
- [Node.js mysql2](https://github.com/sidorares/node-mysql2)
- [MySQL Connector/J](https://dev.mysql.com/doc/connector-j/8.0/en/)

## 贡献

欢迎提交更多语言的客户端示例！
