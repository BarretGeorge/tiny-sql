#pragma once

#include <string>
#include <cstdint>

namespace tiny_sql {

class SocketUtils {
public:
    // 创建TCP监听socket
    static int createListenSocket(uint16_t port, int backlog = 1024);

    // 设置非阻塞模式
    static bool setNonBlocking(int fd);

    // 设置地址重用
    static bool setReuseAddr(int fd);

    // 设置端口重用
    static bool setReusePort(int fd);

    // 设置TCP_NODELAY
    static bool setTcpNoDelay(int fd);

    // 设置SO_KEEPALIVE
    static bool setKeepAlive(int fd);

    // 关闭socket
    static void closeSocket(int fd);

    // 获取对端地址
    static std::string getPeerAddress(int fd);

    // 获取本地地址
    static std::string getLocalAddress(int fd);

    // 接受新连接
    static int acceptConnection(int listen_fd, std::string& peer_addr);
};

} // namespace tiny_sql
