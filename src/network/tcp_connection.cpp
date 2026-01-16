#include "tiny_sql/network/tcp_connection.h"
#include "tiny_sql/network/socket_utils.h"
#include "tiny_sql/common/logger.h"

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

namespace tiny_sql {

TcpConnection::TcpConnection(int fd, const std::string& peer_addr)
    : fd_(fd), peer_addr_(peer_addr), connected_(true) {
    LOG_INFO("New connection from " << peer_addr_);
}

TcpConnection::~TcpConnection() {
    if (connected_) {
        close();
    }
}

ssize_t TcpConnection::read() {
    if (!connected_) {
        return -1;
    }

    ssize_t n = input_buffer_.readFromFd(fd_);
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Read error from " << peer_addr_ << ": " << strerror(errno));
            return -1;
        }
        return 0;  // EAGAIN, no data available
    } else if (n == 0) {
        // Connection closed by peer
        LOG_INFO("Connection closed by peer: " << peer_addr_);
        return -1;
    }

    return n;
}

ssize_t TcpConnection::send(const void* data, size_t len) {
    if (!connected_) {
        return -1;
    }

    // 尝试直接发送
    ssize_t n = ::write(fd_, data, len);
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Write error to " << peer_addr_ << ": " << strerror(errno));
            return -1;
        }
        // EAGAIN, 写入输出缓冲区
        output_buffer_.writeBytes(static_cast<const uint8_t*>(data), len);
        return 0;
    }

    // 部分发送成功
    if (static_cast<size_t>(n) < len) {
        output_buffer_.writeBytes(
            static_cast<const uint8_t*>(data) + n,
            len - n
        );
    }

    return n;
}

ssize_t TcpConnection::send(const std::string& data) {
    return send(data.data(), data.size());
}

ssize_t TcpConnection::send(const Buffer& buffer) {
    const auto& data = buffer.data();
    return send(data.data(), data.size());
}

void TcpConnection::close() {
    if (!connected_) {
        return;
    }

    connected_ = false;
    LOG_INFO("Closing connection to " << peer_addr_);
    SocketUtils::closeSocket(fd_);
    fd_ = -1;
}

void TcpConnection::forceClose() {
    close();
}

void TcpConnection::handleRead() {
    ssize_t n = read();
    if (n < 0) {
        handleClose();
        return;
    }

    if (n > 0 && message_callback_) {
        message_callback_(shared_from_this(), input_buffer_);
    }
}

void TcpConnection::handleWrite() {
    if (!connected_ || output_buffer_.readableBytes() == 0) {
        return;
    }

    ssize_t n = output_buffer_.writeToFd(fd_);
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Write error to " << peer_addr_ << ": " << strerror(errno));
            handleError();
            return;
        }
    }

    if (output_buffer_.readableBytes() == 0 && write_complete_callback_) {
        write_complete_callback_(shared_from_this());
    }
}

void TcpConnection::handleClose() {
    if (connected_) {
        connected_ = false;
        if (close_callback_) {
            close_callback_(shared_from_this());
        }
        close();
    }
}

void TcpConnection::handleError() {
    int err = 0;
    socklen_t err_len = sizeof(err);
    if (::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &err_len) < 0) {
        err = errno;
    }

    LOG_ERROR("Socket error on " << peer_addr_ << ": " << strerror(err));
    handleClose();
}

} // namespace tiny_sql
