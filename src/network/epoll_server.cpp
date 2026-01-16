#include "tiny_sql/network/epoll_server.h"
#include "tiny_sql/network/socket_utils.h"
#include "tiny_sql/common/logger.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

namespace tiny_sql {

EpollServer::EpollServer(uint16_t port, int max_connections)
    : port_(port),
      max_connections_(max_connections),
      listen_fd_(-1),
      epoll_fd_(-1),
      running_(false) {}

EpollServer::~EpollServer() {
    stop();
}

void EpollServer::start() {
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

    // 创建epoll
    epoll_fd_ = ::epoll_create1(0);
    if (epoll_fd_ < 0) {
        LOG_FATAL("Failed to create epoll: " << strerror(errno));
        SocketUtils::closeSocket(listen_fd_);
        return;
    }

    // 添加监听socket到epoll
    if (!addFdToEpoll(listen_fd_, EPOLLIN)) {
        LOG_FATAL("Failed to add listen socket to epoll");
        ::close(epoll_fd_);
        SocketUtils::closeSocket(listen_fd_);
        return;
    }

    running_ = true;
    LOG_INFO("Tiny-SQL server started on port " << port_);

    // 进入事件循环
    eventLoop();
}

void EpollServer::stop() {
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

    // 关闭epoll和监听socket
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }

    if (listen_fd_ >= 0) {
        SocketUtils::closeSocket(listen_fd_);
        listen_fd_ = -1;
    }

    LOG_INFO("Server stopped");
}

void EpollServer::eventLoop() {
    const int MAX_EVENTS = 1024;
    struct epoll_event events[MAX_EVENTS];

    while (running_) {
        int n = ::epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

        if (n < 0) {
            if (errno == EINTR) {
                continue;  // 被信号中断,继续
            }
            LOG_ERROR("epoll_wait error: " << strerror(errno));
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == listen_fd_) {
                // 新连接
                handleAccept();
            } else {
                // 客户端连接的事件
                if (ev & (EPOLLERR | EPOLLHUP)) {
                    // 错误或挂断
                    handleClose(fd);
                } else if (ev & EPOLLIN) {
                    // 可读
                    handleRead(fd);
                } else if (ev & EPOLLOUT) {
                    // 可写
                    handleWrite(fd);
                }
            }
        }
    }
}

void EpollServer::handleAccept() {
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

        // 添加到epoll
        if (!addFdToEpoll(conn_fd, EPOLLIN | EPOLLET)) {  // 边缘触发
            LOG_ERROR("Failed to add connection to epoll");
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

void EpollServer::handleRead(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        LOG_WARN("Connection not found for fd " << fd);
        return;
    }

    auto conn = it->second;
    conn->handleRead();
}

void EpollServer::handleWrite(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        LOG_WARN("Connection not found for fd " << fd);
        return;
    }

    auto conn = it->second;
    conn->handleWrite();

    // 如果输出缓冲区为空,移除EPOLLOUT事件
    if (conn->getOutputBuffer().readableBytes() == 0) {
        modifyFdInEpoll(fd, EPOLLIN | EPOLLET);
    }
}

void EpollServer::handleClose(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        return;
    }

    LOG_DEBUG("Closing connection (fd=" << fd << ")");

    auto conn = it->second;
    removeFdFromEpoll(fd);
    connections_.erase(it);
}

bool EpollServer::addFdToEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOG_ERROR("epoll_ctl ADD failed: " << strerror(errno));
        return false;
    }

    return true;
}

bool EpollServer::modifyFdInEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        LOG_ERROR("epoll_ctl MOD failed: " << strerror(errno));
        return false;
    }

    return true;
}

bool EpollServer::removeFdFromEpoll(int fd) {
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        LOG_ERROR("epoll_ctl DEL failed: " << strerror(errno));
        return false;
    }

    return true;
}

} // namespace tiny_sql
