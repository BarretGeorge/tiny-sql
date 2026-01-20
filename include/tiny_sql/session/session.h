#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <array>

namespace tiny_sql {

/**
 * 会话状态枚举
 */
enum class SessionState {
    INIT,              // 初始状态
    HANDSHAKE_SENT,    // 已发送握手包
    AUTHENTICATING,    // 认证中
    AUTHENTICATED,     // 已认证
    COMMAND_PHASE,     // 命令处理阶段
    CLOSING,           // 关闭中
    CLOSED             // 已关闭
};

/**
 * MySQL会话类
 * 维护每个客户端连接的状态信息
 */
class Session {
public:
    explicit Session(uint32_t connection_id);
    ~Session() = default;

    // 禁止拷贝和赋值
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // Getters
    uint32_t getConnectionId() const { return connection_id_; }
    SessionState getState() const { return state_; }
    const std::string& getUsername() const { return username_; }
    const std::string& getCurrentDatabase() const { return current_database_; }
    uint8_t getSequenceId() const { return sequence_id_; }
    const std::array<uint8_t, 20>& getAuthPluginData() const { return auth_plugin_data_; }
    bool isAuthenticated() const { return state_ == SessionState::AUTHENTICATED ||
                                          state_ == SessionState::COMMAND_PHASE; }

    // Setters
    void setState(SessionState state) { state_ = state; }
    void setUsername(const std::string& username) { username_ = username; }
    void setCurrentDatabase(const std::string& database) { current_database_ = database; }
    void setAuthPluginData(const std::array<uint8_t, 20>& data) { auth_plugin_data_ = data; }

    // 序列号管理
    uint8_t nextSequenceId() { return sequence_id_++; }
    void resetSequenceId() { sequence_id_ = 0; }
    void setSequenceId(uint8_t id) { sequence_id_ = id; }

    // 会话信息
    std::string getSessionInfo() const;

private:
    uint32_t connection_id_;              // 连接ID（唯一标识）
    SessionState state_;                  // 会话状态
    std::string username_;                // 用户名
    std::string current_database_;        // 当前数据库
    uint8_t sequence_id_;                 // MySQL协议包序列号
    std::array<uint8_t, 20> auth_plugin_data_;  // 认证挑战数据

    // 未来可以添加更多字段：
    // - 字符集
    // - 事务状态
    // - 自动提交标志
    // - 时区
    // - SQL模式
    // - 连接时间
    // - 最后活动时间
};

} // namespace tiny_sql
