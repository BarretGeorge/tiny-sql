#pragma once

#include "tiny_sql/network/tcp_connection.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

namespace tiny_sql {

class EpollServer {
public:
    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, Buffer&)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;

    EpollServer(uint16_t port, int max_connections = 10000);
    ~EpollServer();

    // 禁止拷贝
    EpollServer(const EpollServer&) = delete;
    EpollServer& operator=(const EpollServer&) = delete;

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

    // 添加fd到epoll
    bool addFdToEpoll(int fd, uint32_t events);

    // 修改fd的epoll事件
    bool modifyFdInEpoll(int fd, uint32_t events);

    // 从epoll删除fd
    bool removeFdFromEpoll(int fd);

    uint16_t port_;
    int max_connections_;
    int listen_fd_;
    int epoll_fd_;
    bool running_;

    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
};

} // namespace tiny_sql
