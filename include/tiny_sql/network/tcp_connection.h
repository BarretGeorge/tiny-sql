#pragma once

#include "tiny_sql/common/buffer.h"
#include <string>
#include <memory>
#include <functional>

namespace tiny_sql {

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, Buffer&)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using WriteCompleteCallback = std::function<void(std::shared_ptr<TcpConnection>)>;

    TcpConnection(int fd, const std::string& peer_addr);
    ~TcpConnection();

    // 禁止拷贝
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    // 获取文件描述符
    int getFd() const { return fd_; }

    // 获取对端地址
    const std::string& getPeerAddr() const { return peer_addr_; }

    // 是否已连接
    bool isConnected() const { return connected_; }

    // 读取数据
    ssize_t read();

    // 发送数据
    ssize_t send(const void* data, size_t len);
    ssize_t send(const std::string& data);
    ssize_t send(const Buffer& buffer);

    // 关闭连接
    void close();

    // 强制关闭连接
    void forceClose();

    // 获取输入缓冲区
    Buffer& getInputBuffer() { return input_buffer_; }

    // 获取输出缓冲区
    Buffer& getOutputBuffer() { return output_buffer_; }

    // 设置回调
    void setMessageCallback(const MessageCallback& cb) { message_callback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { close_callback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { write_complete_callback_ = cb; }

    // 处理事件
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

private:
    int fd_;
    std::string peer_addr_;
    bool connected_;

    Buffer input_buffer_;
    Buffer output_buffer_;

    MessageCallback message_callback_;
    CloseCallback close_callback_;
    WriteCompleteCallback write_complete_callback_;
};

} // namespace tiny_sql
