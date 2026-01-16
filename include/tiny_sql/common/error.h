#pragma once

#include <string>
#include <stdexcept>

namespace tiny_sql {

// MySQL错误码
class MySQLError : public std::runtime_error {
public:
    MySQLError(uint16_t error_code, const std::string& sql_state,
               const std::string& message)
        : std::runtime_error(message),
          error_code_(error_code),
          sql_state_(sql_state) {}

    uint16_t getErrorCode() const { return error_code_; }
    const std::string& getSqlState() const { return sql_state_; }

private:
    uint16_t error_code_;
    std::string sql_state_;
};

// 常见错误码
namespace ErrorCode {
    constexpr uint16_t ER_HANDSHAKE_ERROR = 1043;
    constexpr uint16_t ER_ACCESS_DENIED_ERROR = 1045;
    constexpr uint16_t ER_NO_DB_ERROR = 1046;
    constexpr uint16_t ER_UNKNOWN_COM_ERROR = 1047;
    constexpr uint16_t ER_BAD_DB_ERROR = 1049;
    constexpr uint16_t ER_DBACCESS_DENIED_ERROR = 1044;
    constexpr uint16_t ER_UNKNOWN_ERROR = 1105;
    constexpr uint16_t ER_SYNTAX_ERROR = 1064;
    constexpr uint16_t ER_NET_PACKET_TOO_LARGE = 1153;
    constexpr uint16_t ER_NET_READ_ERROR = 1158;
    constexpr uint16_t ER_NET_WRITE_ERROR = 1160;
}

// 常见SQL State
namespace SqlState {
    constexpr const char* HY000 = "HY000";  // General error
    constexpr const char* S1000 = "S1000";  // Server error
    constexpr const char* S1001 = "S1001";  // Authentication failed
    constexpr const char* S0000 = "00000";  // Success
    constexpr const char* S42000 = "42000"; // Syntax error
}

} // namespace tiny_sql
