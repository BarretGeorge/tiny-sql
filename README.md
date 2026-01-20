# Tiny-SQL

一个基于C++20实现的轻量级MySQL兼容数据库服务器，使用高性能异步网络框架和完整的MySQL协议实现。

## 项目概述

Tiny-SQL是一个教学型数据库项目，实现了MySQL协议的客户端-服务器通信机制。项目采用事件驱动的网络架构，支持跨平台（Linux/macOS/BSD），并实现了MySQL协议的握手、认证和基础命令处理。

## 主要特性

### 已实现功能

✅ **网络层 (Network Layer)**
- 异步非阻塞I/O模型
- 跨平台事件循环
  - Linux: epoll
  - macOS/BSD: kqueue
- TCP连接管理
- 双缓冲设计（输入/输出缓冲区）

✅ **协议层 (Protocol Layer)**
- MySQL协议包解析和编码
- Handshake V10协议
- HandshakeResponse41协议
- OK/ERR/EOF响应包

✅ **认证模块 (Authentication)**
- `mysql_native_password` 认证方法
- SHA1密码哈希验证
- 多用户支持（root, test, admin）

✅ **会话管理 (Session Management)**
- 连接会话状态跟踪
- 用户信息管理
- 当前数据库上下文

✅ **命令处理 (Command Handling)**
- COM_PING - 心跳检测
- COM_QUIT - 断开连接
- COM_QUERY - SQL查询（基础）
- COM_INIT_DB - 切换数据库

✅ **SQL解析器 (SQL Parser)**
- 词法分析器 (Lexer) - Token化
- 语法分析器 (Parser) - AST生成
- 支持SELECT, INSERT, CREATE TABLE, DROP TABLE
- 支持WHERE, LIMIT等子句
- 详细的错误报告
- 已集成到命令处理器

✅ **存储引擎 (Storage Engine)** 🆕
- 内存表存储
- 完整的类型系统（INT, BIGINT, FLOAT, DOUBLE, VARCHAR, TEXT等）
- Table和Row数据结构
- Database和StorageEngine管理
- CREATE TABLE - 创建表
- INSERT - 插入数据
- DROP TABLE - 删除表
- SHOW TABLES/DATABASES - 查看结构
- 线程安全设计

### 待实现功能

❌ **SQL执行引擎**
- SELECT查询执行
- WHERE条件过滤
- MySQL结果集协议

❌ **高级存储功能**
- 磁盘持久化
- 索引系统（B+树）
- 事务支持（ACID）
- 查询优化器

❌ **高级功能**
- SSL/TLS加密通信
- 预处理语句
- 查询缓存
- 主从复制

## 架构设计

```
┌─────────────────────────────────────┐
│   应用层 (Application Layer)        │
│   - 命令分发器                      │
│   - 会话管理                        │
└────────────────┬────────────────────┘
                 │
┌─────────────────▼────────────────────┐
│   协议层 (Protocol Layer)            │
│   - MySQL协议处理器                  │
│   - 认证模块                         │
│   - 包编解码                         │
└────────────────┬────────────────────┘
                 │
┌─────────────────▼────────────────────┐
│   网络层 (Network Layer)             │
│   - TCP连接管理                      │
│   - 事件循环（Epoll/Kqueue）         │
│   - Buffer管理                       │
└────────────────┬────────────────────┘
                 │
            操作系统
```

## 编译和运行

### 环境要求

- CMake 3.20+
- C++20 编译器（GCC 12+, Clang 14+）
- OpenSSL 开发库

### 编译步骤

```bash
# 克隆项目
cd tiny-sql

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make

# 运行服务器
./tiny-sql [端口号]  # 默认端口 3306
```

### 示例

```bash
# 启动服务器在3307端口
./tiny-sql 3307

# 使用MySQL客户端连接（如果有）
mysql -h 127.0.0.1 -P 3307 -u root

# 使用Python测试客户端
python3 test_client.py
```

## 测试

项目包含测试客户端脚本：

- `test_client.py` - 完整的MySQL协议测试客户端
- `simple_test.py` - 简单的连接测试

运行测试：

```bash
# 启动服务器
./tiny-sql 3307 &

# 运行测试
python3 test_client.py
```

## 客户端示例

[examples/](file:///workspaces/code/tiny-sql/examples) 目录包含使用官方MySQL驱动的多语言客户端示例：

### 支持的语言

- **Python**: mysql-connector-python, PyMySQL
- **Node.js**: mysql2
- **Go**: go-sql-driver/mysql
- **Java**: MySQL Connector/J (JDBC)

### 快速开始

```bash
# 安装依赖
cd examples
./setup.sh

# 测试所有客户端
./test_all.sh

# 或单独运行
python3 python_mysql_connector.py
node nodejs_mysql_client.js
go run go_mysql_client.go
```

详见 [examples/README.md](file:///workspaces/code/tiny-sql/examples/README.md)

## 用户认证

当前支持的测试用户：

| 用户名 | 密码 | 说明 |
|--------|------|------|
| root   | (空) | 超级用户 |
| tiny   | (空) | 测试用户 |
| test   | test | 测试用户 |
| admin  | admin123 | 管理员用户 |

## 项目结构

```
tiny-sql/
├── CMakeLists.txt          # 构建配置
├── main.cpp               # 主程序入口
├── README.md              # 项目文档
├── test_client.py         # 测试客户端
├── include/tiny_sql/      # 头文件目录
│   ├── common/           # 通用模块
│   │   ├── buffer.h      # 缓冲区类
│   │   ├── logger.h      # 日志系统
│   │   ├── types.h       # 类型定义
│   │   └── error.h       # 错误定义
│   ├── network/          # 网络模块
│   │   ├── server.h      # 服务器类
│   │   ├── tcp_connection.h
│   │   ├── event_loop.h
│   │   ├── epoll_event_loop.h
│   │   ├── kqueue_event_loop.h
│   │   └── socket_utils.h
│   ├── protocol/         # 协议模块
│   │   ├── packet.h      # 包基类
│   │   ├── handshake.h   # 握手协议
│   │   ├── response.h    # 响应包
│   │   └── protocol_handler.h
│   ├── auth/            # 认证模块
│   │   └── authenticator.h
│   ├── session/         # 会话模块
│   │   └── session.h
│   └── command/         # 命令处理模块
│       └── command_handler.h
└── src/                 # 实现文件
    ├── common/
    ├── network/
    ├── protocol/
    ├── auth/
    ├── session/
    └── command/
```

## 技术栈

- **编程语言**: C++20
- **构建工具**: CMake
- **网络I/O**: Epoll (Linux), Kqueue (macOS/BSD)
- **加密库**: OpenSSL
- **并发模型**: 单线程事件驱动
- **设计模式**: 工厂模式、策略模式、观察者模式

## 性能特性

- 非阻塞I/O
- 零拷贝缓冲区设计
- 边缘触发模式（ET）
- 高效的事件循环
- TCP_NODELAY 和 SO_KEEPALIVE 优化

## 开发路线图

### 第一阶段 ✅ (已完成)
- [x] 网络框架
- [x] MySQL协议基础
- [x] 握手和认证
- [x] 基础命令处理

### 第二阶段 ✅ (已完成)
- [x] SQL词法分析器
- [x] SQL语法分析器
- [x] AST抽象语法树
- [x] 多语言客户端示例

### 第三阶段 ✅ (已完成)
- [x] SQL执行引擎框架
- [x] SQL解析器集成到命令处理器
- [x] SELECT/INSERT/CREATE/DROP语句解析
- [x] 测试集成验证

### 第四阶段 (进行中)
- [ ] 内存表存储引擎
- [ ] SQL执行器实现
- [ ] 实现真正的数据存储和查询
- [ ] 支持WHERE条件过滤

### 第五阶段 (未来)
- [ ] 事务支持
- [ ] 索引系统
- [ ] 查询优化
- [ ] SSL/TLS支持

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License

## 参考资料

- [MySQL Protocol Documentation](https://dev.mysql.com/doc/dev/mysql-server/latest/PAGE_PROTOCOL.html)
- [MySQL Internals Manual](https://dev.mysql.com/doc/internals/en/)
- [Epoll Documentation](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Kqueue Documentation](https://www.freebsd.org/cgi/man.cgi?query=kqueue)

## 作者

Tiny-SQL 团队

## 致谢

感谢所有开源项目和文档的贡献者。
