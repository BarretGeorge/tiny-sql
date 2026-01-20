#include "tiny_sql/network/server.h"
#include "tiny_sql/protocol/protocol_handler.h"
#include "tiny_sql/common/logger.h"
#include <csignal>
#include <iostream>
#include <unordered_map>
#include <memory>

using namespace tiny_sql;

// 全局服务器实例,用于信号处理
Server* g_server = nullptr;

void signalHandler(int signum) {
    LOG_INFO("Received signal " << signum);
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    uint16_t port = 3306;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }

    // 设置日志级别
    Logger::instance().setLevel(LogLevel::DEBUG);

    LOG_INFO("Starting Tiny-SQL Server...");
    LOG_INFO("Version: 1.0.0");
    LOG_INFO("Port: " << port);

    // 创建服务器
    Server server(port);
    g_server = &server;

    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 协议处理器映射表（每个连接一个处理器）
    std::unordered_map<int, std::shared_ptr<ProtocolHandler>> protocol_handlers;

    // 设置回调
    server.setConnectionCallback([&protocol_handlers](std::shared_ptr<TcpConnection> conn) {
        LOG_INFO("New connection established: " << conn->getPeerAddr());

        // 创建协议处理器
        auto handler = std::make_shared<ProtocolHandler>(conn);
        protocol_handlers[conn->getFd()] = handler;

        // 发送握手包
        handler->sendHandshake();
    });

    server.setMessageCallback([&protocol_handlers](std::shared_ptr<TcpConnection> conn, Buffer& buffer) {
        LOG_DEBUG("Received " << buffer.readableBytes() << " bytes from " << conn->getPeerAddr());

        // 查找对应的协议处理器
        auto it = protocol_handlers.find(conn->getFd());
        if (it == protocol_handlers.end()) {
            LOG_ERROR("No protocol handler found for connection: " << conn->getFd());
            return;
        }

        // 处理数据
        if (!it->second->handleData(buffer)) {
            // 处理失败或连接需要关闭
            LOG_INFO("Connection will be closed: " << conn->getPeerAddr());
            conn->close();
        }
    });

    server.setCloseCallback([&protocol_handlers](std::shared_ptr<TcpConnection> conn) {
        LOG_INFO("Connection closed: " << conn->getPeerAddr());

        // 清理协议处理器
        protocol_handlers.erase(conn->getFd());
    });

    // 启动服务器
    server.start();

    LOG_INFO("Server shutdown completed");
    return 0;
}
