#pragma once

#include "tiny_sql/protocol/packet.h"
#include <string>
#include <cstdint>

namespace tiny_sql {

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
