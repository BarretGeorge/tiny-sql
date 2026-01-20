#pragma once

#include "tiny_sql/common/buffer.h"
#include "tiny_sql/session/session.h"
#include "tiny_sql/command/command_handler.h"
#include "tiny_sql/network/tcp_connection.h"
#include <memory>
#include <functional>

namespace tiny_sql {

/**
 * MySQL协议处理器
 * 负责处理完整的MySQL协议流程：握手、认证、命令处理
 */
class ProtocolHandler {
public:
    explicit ProtocolHandler(std::shared_ptr<TcpConnection> conn);
    ~ProtocolHandler() = default;

    /**
     * 处理接收到的数据
     * @param buffer 接收缓冲区
     * @return 是否继续保持连接
     */
    bool handleData(Buffer& buffer);

    /**
     * 获取会话对象
     */
    std::shared_ptr<Session> getSession() const { return session_; }

    /**
     * 发送握手包
     */
    void sendHandshake();

private:
    /**
     * 处理认证响应
     */
    bool handleAuthentication(Buffer& buffer);

    /**
     * 处理命令
     */
    bool handleCommand(Buffer& buffer);

    /**
     * 发送响应包
     */
    void sendResponse(Buffer& response);

    std::shared_ptr<TcpConnection> connection_;
    std::shared_ptr<Session> session_;
    std::unique_ptr<CommandDispatcher> command_dispatcher_;
};

} // namespace tiny_sql
