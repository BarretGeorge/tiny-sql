#pragma once

#include "tiny_sql/protocol/packet.h"
#include "tiny_sql/storage/value.h"
#include "tiny_sql/storage/table.h"
#include <string>
#include <cstdint>
#include <vector>

namespace tiny_sql {

/**
 * MySQL字段类型常量
 * MySQL Field Type Constants
 */
namespace MySQLFieldType {
    constexpr uint8_t MYSQL_TYPE_TINY = 0x01;       // TINYINT, BOOLEAN
    constexpr uint8_t MYSQL_TYPE_LONG = 0x03;       // INT
    constexpr uint8_t MYSQL_TYPE_FLOAT = 0x04;      // FLOAT
    constexpr uint8_t MYSQL_TYPE_DOUBLE = 0x05;     // DOUBLE
    constexpr uint8_t MYSQL_TYPE_LONGLONG = 0x08;   // BIGINT
    constexpr uint8_t MYSQL_TYPE_STRING = 0xFE;     // VARCHAR, TEXT
}

/**
 * MySQL列标志常量
 * MySQL Column Flag Constants
 */
namespace ColumnFlags {
    constexpr uint16_t NOT_NULL_FLAG = 0x0001;
    constexpr uint16_t PRI_KEY_FLAG = 0x0002;
    constexpr uint16_t AUTO_INCREMENT_FLAG = 0x0200;
}

/**
 * MySQL字符集常量
 * MySQL Charset Constants
 */
namespace MySQLCharset {
    constexpr uint16_t UTF8_GENERAL_CI = 33;
}

/**
 * OK 包
 *
 * 包结构（Protocol 4.1）：
 * - 1 byte: header (0x00 for OK, 0xFE for EOF in some contexts)
 * - lenenc-int: affected rows
 * - lenenc-int: last insert id
 * - 2 bytes: status flags
 * - 2 bytes: warnings
 * - string[EOF]: info (human readable status information)
 */
class OkPacket : public Packet {
public:
    OkPacket();
    explicit OkPacket(uint64_t affected_rows, uint64_t last_insert_id = 0,
                      uint16_t status_flags = 0, uint16_t warnings = 0,
                      const std::string& info = "");

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override;

    // Getters
    uint64_t getAffectedRows() const { return affected_rows_; }
    uint64_t getLastInsertId() const { return last_insert_id_; }
    uint16_t getStatusFlags() const { return status_flags_; }
    uint16_t getWarnings() const { return warnings_; }
    const std::string& getInfo() const { return info_; }

    // Setters
    void setAffectedRows(uint64_t rows) { affected_rows_ = rows; }
    void setLastInsertId(uint64_t id) { last_insert_id_ = id; }
    void setStatusFlags(uint16_t flags) { status_flags_ = flags; }
    void setWarnings(uint16_t warnings) { warnings_ = warnings; }
    void setInfo(const std::string& info) { info_ = info; }

private:
    uint64_t affected_rows_;
    uint64_t last_insert_id_;
    uint16_t status_flags_;
    uint16_t warnings_;
    std::string info_;
};

/**
 * ERR 包
 *
 * 包结构（Protocol 4.1）：
 * - 1 byte: header (0xFF)
 * - 2 bytes: error code
 * - 1 byte: sql_state_marker (#)
 * - 5 bytes: sql_state
 * - string[EOF]: error message
 */
class ErrPacket : public Packet {
public:
    ErrPacket();
    explicit ErrPacket(uint16_t error_code, const std::string& sql_state,
                       const std::string& error_message);

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override;

    // Getters
    uint16_t getErrorCode() const { return error_code_; }
    const std::string& getSqlState() const { return sql_state_; }
    const std::string& getErrorMessage() const { return error_message_; }

    // Setters
    void setErrorCode(uint16_t code) { error_code_ = code; }
    void setSqlState(const std::string& state) { sql_state_ = state; }
    void setErrorMessage(const std::string& message) { error_message_ = message; }

private:
    uint16_t error_code_;
    std::string sql_state_;      // 5字符的SQLSTATE
    std::string error_message_;
};

/**
 * EOF 包（在某些场景下已被废弃，改用OK包）
 *
 * 包结构：
 * - 1 byte: header (0xFE)
 * - 2 bytes: warnings
 * - 2 bytes: status flags
 */
class EofPacket : public Packet {
public:
    EofPacket();
    explicit EofPacket(uint16_t warnings, uint16_t status_flags);

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override;

    // Getters
    uint16_t getWarnings() const { return warnings_; }
    uint16_t getStatusFlags() const { return status_flags_; }

    // Setters
    void setWarnings(uint16_t warnings) { warnings_ = warnings; }
    void setStatusFlags(uint16_t flags) { status_flags_ = flags; }

private:
    uint16_t warnings_;
    uint16_t status_flags_;
};

/**
 * 列定义包（字段包）
 * Column Definition Packet (Field Packet)
 *
 * 结果集协议的第一部分，描述每个列的元数据
 * Part 1 of result set protocol, describes metadata for each column
 */
class ColumnDefinitionPacket : public Packet {
public:
    ColumnDefinitionPacket();
    explicit ColumnDefinitionPacket(
        const std::string& catalog,
        const std::string& schema,
        const std::string& table,
        const std::string& org_table,
        const std::string& name,
        const std::string& org_name,
        uint16_t charset,
        uint32_t column_length,
        uint8_t column_type,
        uint16_t flags,
        uint8_t decimals
    );

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override;

    /**
     * 从ColumnDef创建列定义包
     * Create column definition packet from ColumnDef
     */
    static ColumnDefinitionPacket fromColumnDef(
        const ColumnDef& col,
        const std::string& table_name,
        const std::string& db_name
    );

    // Getters
    const std::string& getName() const { return name_; }
    uint8_t getColumnType() const { return column_type_; }

private:
    std::string catalog_;       // "def"
    std::string schema_;        // 数据库名 / database name
    std::string table_;         // 表名（或别名） / table name (or alias)
    std::string org_table_;     // 原始表名 / original table name
    std::string name_;          // 列名（或别名） / column name (or alias)
    std::string org_name_;      // 原始列名 / original column name
    uint16_t charset_;          // UTF8_GENERAL_CI
    uint32_t column_length_;    // 最大长度 / max length
    uint8_t column_type_;       // MYSQL_TYPE_*
    uint16_t flags_;            // NOT_NULL_FLAG, PRI_KEY_FLAG, etc.
    uint8_t decimals_;          // 小数位数（数值类型） / decimal places (for numeric types)
};

/**
 * 文本结果行包
 * Text Result Row Packet
 *
 * 结果集协议的第二部分，包含一行数据
 * Part 2 of result set protocol, contains one row of data
 */
class TextResultRowPacket : public Packet {
public:
    TextResultRowPacket();
    explicit TextResultRowPacket(const Row& row);

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override;

    void addValue(const Value& value);
    size_t getValueCount() const { return values_.size(); }

private:
    std::vector<Value> values_;
};

/**
 * 包类型识别器
 */
enum class PacketType {
    UNKNOWN,
    OK,
    ERR,
    EOF_PKT,
    HANDSHAKE,
    AUTH_RESPONSE,
    QUERY,
    RESULT_SET
};

/**
 * 从缓冲区识别包类型（不移动读指针）
 */
PacketType identifyPacketType(Buffer& buffer);

} // namespace tiny_sql
