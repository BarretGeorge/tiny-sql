#include "tiny_sql/network/socket_utils.h"
#include "tiny_sql/common/logger.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

namespace tiny_sql {

int SocketUtils::createListenSocket(uint16_t port, int backlog) {
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG_ERROR("Failed to create socket: " << strerror(errno));
        return -1;
    }

    // 设置地址重用
    if (!setReuseAddr(listen_fd)) {
        closeSocket(listen_fd);
        return -1;
    }

    // 绑定地址
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(listen_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind port " << port << ": " << strerror(errno));
        closeSocket(listen_fd);
        return -1;
    }

    // 开始监听
    if (::listen(listen_fd, backlog) < 0) {
        LOG_ERROR("Failed to listen: " << strerror(errno));
        closeSocket(listen_fd);
        return -1;
    }

    LOG_INFO("Listening on port " << port);
    return listen_fd;
}

bool SocketUtils::setNonBlocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        LOG_ERROR("fcntl F_GETFL failed: " << strerror(errno));
        return false;
    }

    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_ERROR("fcntl F_SETFL O_NONBLOCK failed: " << strerror(errno));
        return false;
    }

    return true;
}

bool SocketUtils::setReuseAddr(int fd) {
    int optval = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOG_ERROR("setsockopt SO_REUSEADDR failed: " << strerror(errno));
        return false;
    }
    return true;
}

bool SocketUtils::setReusePort(int fd) {
    int optval = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        LOG_ERROR("setsockopt SO_REUSEPORT failed: " << strerror(errno));
        return false;
    }
    return true;
}

bool SocketUtils::setTcpNoDelay(int fd) {
    int optval = 1;
    if (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0) {
        LOG_ERROR("setsockopt TCP_NODELAY failed: " << strerror(errno));
        return false;
    }
    return true;
}

bool SocketUtils::setKeepAlive(int fd) {
    int optval = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
        LOG_ERROR("setsockopt SO_KEEPALIVE failed: " << strerror(errno));
        return false;
    }
    return true;
}

void SocketUtils::closeSocket(int fd) {
    if (fd >= 0) {
        ::close(fd);
    }
}

std::string SocketUtils::getPeerAddress(int fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (::getpeername(fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len) < 0) {
        return "unknown";
    }

    char ip[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));

    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

std::string SocketUtils::getLocalAddress(int fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (::getsockname(fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len) < 0) {
        return "unknown";
    }

    char ip[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));

    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

int SocketUtils::acceptConnection(int listen_fd, std::string& peer_addr) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int conn_fd = ::accept(listen_fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len);
    if (conn_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("accept failed: " << strerror(errno));
        }
        return -1;
    }

    char ip[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    peer_addr = std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));

    return conn_fd;
}

} // namespace tiny_sql
