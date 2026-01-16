#include "tiny_sql/network/epoll_server.h"
#include "tiny_sql/common/logger.h"
#include <csignal>
#include <iostream>

using namespace tiny_sql;

// 全局服务器实例,用于信号处理
EpollServer* g_server = nullptr;

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
    EpollServer server(port);
    g_server = &server;

    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 设置回调
    server.setConnectionCallback([](std::shared_ptr<TcpConnection> conn) {
        LOG_INFO("New connection established: " << conn->getPeerAddr());
    });

    server.setMessageCallback([](std::shared_ptr<TcpConnection> conn, Buffer& buffer) {
        // 暂时只打印接收到的数据
        LOG_DEBUG("Received " << buffer.readableBytes() << " bytes from " << conn->getPeerAddr());

        // 简单回显数据(用于测试)
        std::vector<uint8_t> data = buffer.retrieveAll();
        conn->send(data.data(), data.size());
    });

    server.setCloseCallback([](std::shared_ptr<TcpConnection> conn) {
        LOG_INFO("Connection closed: " << conn->getPeerAddr());
    });

    // 启动服务器
    server.start();

    LOG_INFO("Server shutdown completed");
    return 0;
}
