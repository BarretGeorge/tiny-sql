#pragma once

#include "tiny_sql/network/tcp_connection.h"
#include "tiny_sql/network/event_loop.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

namespace tiny_sql {

// 跨平台服务器（自动选择epoll/kqueue）
class Server {
public:
    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, Buffer&)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;

    Server(uint16_t port, int max_connections = 10000);
    ~Server();

    // 禁止拷贝
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

    // 设置回调
    void setConnectionCallback(const ConnectionCallback& cb) {
        connection_callback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        message_callback_ = cb;
    }

    void setCloseCallback(const CloseCallback& cb) {
        close_callback_ = cb;
    }

private:
    // 事件循环
    void eventLoop();

    // 处理新连接
    void handleAccept();

    // 处理读事件
    void handleRead(int fd);

    // 处理写事件
    void handleWrite(int fd);

    // 处理关闭
    void handleClose(int fd);

    uint16_t port_;
    int max_connections_;
    int listen_fd_;
    bool running_;

    std::unique_ptr<EventLoop> event_loop_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
};

} // namespace tiny_sql
