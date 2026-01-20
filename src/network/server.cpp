#include "tiny_sql/network/server.h"
#include "tiny_sql/network/socket_utils.h"
#include "tiny_sql/common/logger.h"
#include <unistd.h>

namespace tiny_sql {

Server::Server(uint16_t port, int max_connections)
    : port_(port),
      max_connections_(max_connections),
      listen_fd_(-1),
      running_(false),
      event_loop_(createEventLoop()) {}

Server::~Server() {
    stop();
}

void Server::start() {
    // 创建监听socket
    listen_fd_ = SocketUtils::createListenSocket(port_);
    if (listen_fd_ < 0) {
        LOG_FATAL("Failed to create listen socket");
        return;
    }

    // 设置非阻塞
    if (!SocketUtils::setNonBlocking(listen_fd_)) {
        LOG_FATAL("Failed to set listen socket non-blocking");
        SocketUtils::closeSocket(listen_fd_);
        return;
    }

    // 初始化事件循环
    if (!event_loop_->init()) {
        LOG_FATAL("Failed to initialize event loop");
        SocketUtils::closeSocket(listen_fd_);
        return;
    }

    // 添加监听socket到事件循环
    if (!event_loop_->addFd(listen_fd_, static_cast<uint32_t>(EventType::READ))) {
        LOG_FATAL("Failed to add listen socket to event loop");
        event_loop_->close();
        SocketUtils::closeSocket(listen_fd_);
        return;
    }

    running_ = true;
    LOG_INFO("Tiny-SQL server started on port " << port_);

    // 进入事件循环
    eventLoop();
}

void Server::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    LOG_INFO("Stopping server...");

    // 关闭所有连接
    for (auto& pair : connections_) {
        pair.second->forceClose();
    }
    connections_.clear();

    // 关闭事件循环
    if (event_loop_) {
        event_loop_->close();
    }

    // 关闭监听socket
    if (listen_fd_ >= 0) {
        SocketUtils::closeSocket(listen_fd_);
        listen_fd_ = -1;
    }

    LOG_INFO("Server stopped");
}

void Server::eventLoop() {
    while (running_) {
        int n = event_loop_->wait(-1);

        if (n < 0) {
            LOG_ERROR("Event loop wait error");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = event_loop_->getReadyFd(i);
            uint32_t events = event_loop_->getReadyEvents(i);

            if (fd == listen_fd_) {
                // 新连接
                handleAccept();
            } else {
                // 客户端连接的事件
                if (events & (static_cast<uint32_t>(EventType::ERROR) |
                             static_cast<uint32_t>(EventType::CLOSE))) {
                    // 错误或关闭
                    handleClose(fd);
                } else if (events & static_cast<uint32_t>(EventType::READ)) {
                    // 可读
                    handleRead(fd);
                } else if (events & static_cast<uint32_t>(EventType::WRITE)) {
                    // 可写
                    handleWrite(fd);
                }
            }
        }
    }
}

void Server::handleAccept() {
    while (true) {
        std::string peer_addr;
        int conn_fd = SocketUtils::acceptConnection(listen_fd_, peer_addr);

        if (conn_fd < 0) {
            break;  // 没有更多连接
        }

        // 检查连接数限制
        if (static_cast<int>(connections_.size()) >= max_connections_) {
            LOG_WARN("Max connections reached, rejecting connection from " << peer_addr);
            SocketUtils::closeSocket(conn_fd);
            continue;
        }

        // 设置非阻塞
        if (!SocketUtils::setNonBlocking(conn_fd)) {
            LOG_ERROR("Failed to set connection non-blocking");
            SocketUtils::closeSocket(conn_fd);
            continue;
        }

        // 设置TCP_NODELAY
        SocketUtils::setTcpNoDelay(conn_fd);

        // 创建TcpConnection
        auto conn = std::make_shared<TcpConnection>(conn_fd, peer_addr);

        // 设置回调
        conn->setMessageCallback(message_callback_);
        conn->setCloseCallback([this](std::shared_ptr<TcpConnection> c) {
            handleClose(c->getFd());
            if (close_callback_) {
                close_callback_(c);
            }
        });

        // 添加到事件循环
        if (!event_loop_->addFd(conn_fd, static_cast<uint32_t>(EventType::READ))) {
            LOG_ERROR("Failed to add connection to event loop");
            conn->forceClose();
            continue;
        }

        // 保存连接
        connections_[conn_fd] = conn;

        // 调用连接回调
        if (connection_callback_) {
            connection_callback_(conn);
        }

        LOG_DEBUG("Accepted connection from " << peer_addr << " (fd=" << conn_fd << ")");
    }
}

void Server::handleRead(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        LOG_WARN("Connection not found for fd " << fd);
        return;
    }

    auto conn = it->second;
    conn->handleRead();
}

void Server::handleWrite(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        LOG_WARN("Connection not found for fd " << fd);
        return;
    }

    auto conn = it->second;
    conn->handleWrite();

    // 如果输出缓冲区为空,只监听读事件
    if (conn->getOutputBuffer().readableBytes() == 0) {
        event_loop_->modifyFd(fd, static_cast<uint32_t>(EventType::READ));
    }
}

void Server::handleClose(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        return;
    }

    LOG_DEBUG("Closing connection (fd=" << fd << ")");

    auto conn = it->second;
    event_loop_->removeFd(fd);
    connections_.erase(it);
}

} // namespace tiny_sql
