#include "tiny_sql/protocol/protocol_handler.h"
#include "tiny_sql/protocol/handshake.h"
#include "tiny_sql/protocol/response.h"
#include "tiny_sql/protocol/packet.h"
#include "tiny_sql/auth/authenticator.h"
#include "tiny_sql/common/logger.h"

namespace tiny_sql {

ProtocolHandler::ProtocolHandler(std::shared_ptr<TcpConnection> conn)
    : connection_(conn)
    , session_(std::make_shared<Session>(conn->getFd()))
    , command_dispatcher_(std::make_unique<CommandDispatcher>())
{}

void ProtocolHandler::sendHandshake() {
    LOG_DEBUG("Sending handshake to connection: " << session_->getConnectionId());

    // 创建握手包
    HandshakeV10Packet handshake;
    handshake.setConnectionId(session_->getConnectionId());
    handshake.generateAuthPluginData();

    // 保存auth_plugin_data到session
    session_->setAuthPluginData(handshake.getAuthPluginData());

    // 编码握手包
    Buffer response;
    handshake.encode(response, 0);

    // 发送握手包
    connection_->send(response.peek(), response.readableBytes());

    // 更新会话状态
    session_->setState(SessionState::HANDSHAKE_SENT);
    session_->resetSequenceId();
}

bool ProtocolHandler::handleData(Buffer& buffer) {
    // 检查是否有完整的包
    size_t packet_size = checkPacketComplete(buffer);
    if (packet_size == 0) {
        // 包不完整，等待更多数据
        return true;
    }

    LOG_DEBUG("Handling data for session: " << session_->getConnectionId()
              << ", state: " << static_cast<int>(session_->getState())
              << ", packet size: " << packet_size);

    // 根据会话状态处理数据
    switch (session_->getState()) {
        case SessionState::HANDSHAKE_SENT:
            return handleAuthentication(buffer);

        case SessionState::AUTHENTICATED:
        case SessionState::COMMAND_PHASE:
            return handleCommand(buffer);

        case SessionState::CLOSING:
        case SessionState::CLOSED:
            return false; // 连接正在关闭

        default:
            LOG_ERROR("Unexpected session state: " << static_cast<int>(session_->getState()));
            return false;
    }
}

bool ProtocolHandler::handleAuthentication(Buffer& buffer) {
    LOG_DEBUG("Handling authentication for session: " << session_->getConnectionId());

    // 解析HandshakeResponse41包
    HandshakeResponse41Packet auth_response;
    if (!auth_response.decode(buffer)) {
        LOG_ERROR("Failed to decode authentication response");

        // 发送错误包
        Buffer response;
        ErrPacket err_packet(1043, "08S01", "Bad handshake");
        err_packet.encode(response, 2);
        sendResponse(response);
        return false;
    }

    // 验证用户名和密码
    bool auth_success = Authenticator::authenticate(
        auth_response.getUsername(),
        auth_response.getAuthResponse(),
        session_->getAuthPluginData()
    );

    Buffer response;
    if (auth_success) {
        // 认证成功，发送OK包
        LOG_INFO("Authentication successful for user: " << auth_response.getUsername());

        session_->setUsername(auth_response.getUsername());
        if (!auth_response.getDatabase().empty()) {
            session_->setCurrentDatabase(auth_response.getDatabase());
        }
        session_->setState(SessionState::AUTHENTICATED);

        OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0);
        ok_packet.encode(response, 2);
    } else {
        // 认证失败，发送ERR包
        LOG_WARN("Authentication failed for user: " << auth_response.getUsername());

        ErrPacket err_packet(1045, "28000",
                           "Access denied for user '" + auth_response.getUsername() + "'");
        err_packet.encode(response, 2);
        sendResponse(response);
        return false;
    }

    sendResponse(response);
    return true;
}

bool ProtocolHandler::handleCommand(Buffer& buffer) {
    LOG_DEBUG("Handling command for session: " << session_->getConnectionId());

    // 确保会话已认证
    if (!session_->isAuthenticated()) {
        LOG_ERROR("Attempt to execute command before authentication");
        Buffer response;
        ErrPacket err_packet(1184, "08S01", "Aborted connection");
        err_packet.encode(response, session_->nextSequenceId());
        sendResponse(response);
        return false;
    }

    // 使用命令分发器处理命令
    bool result = command_dispatcher_->dispatch(
        buffer,
        *session_,
        [this](Buffer& response) {
            this->sendResponse(response);
        }
    );

    // 如果是QUIT命令，返回false以关闭连接
    if (session_->getState() == SessionState::CLOSING) {
        return false;
    }

    return result;
}

void ProtocolHandler::sendResponse(Buffer& response) {
    if (response.readableBytes() > 0) {
        connection_->send(response.peek(), response.readableBytes());
    }
}

} // namespace tiny_sql
