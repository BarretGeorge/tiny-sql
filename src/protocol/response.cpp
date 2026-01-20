#include "tiny_sql/protocol/response.h"
#include "tiny_sql/common/logger.h"

namespace tiny_sql {

// ==================== OkPacket ====================

OkPacket::OkPacket()
    : affected_rows_(0)
    , last_insert_id_(0)
    , status_flags_(0)
    , warnings_(0)
    , info_("")
{}

OkPacket::OkPacket(uint64_t affected_rows, uint64_t last_insert_id,
                   uint16_t status_flags, uint16_t warnings,
                   const std::string& info)
    : affected_rows_(affected_rows)
    , last_insert_id_(last_insert_id)
    , status_flags_(status_flags)
    , warnings_(warnings)
    , info_(info)
{}

bool OkPacket::decode(Buffer& buffer) {
    PacketHeader header;
    if (!readHeader(buffer, header)) {
        return false;
    }

    if (buffer.readableBytes() < header.payload_length) {
        return false;
    }

    uint8_t packet_header = buffer.readUint8();
    if (packet_header != 0x00 && packet_header != 0xFE) {
        LOG_ERROR("Invalid OK packet header: " << static_cast<int>(packet_header));
        return false;
    }

    affected_rows_ = buffer.readLenencInt();
    last_insert_id_ = buffer.readLenencInt();
    status_flags_ = buffer.readUint16();
    warnings_ = buffer.readUint16();

    // 读取剩余的info字符串
    size_t remaining = header.payload_length - (1 +
        buffer.getLenencIntSize(affected_rows_) +
        buffer.getLenencIntSize(last_insert_id_) + 4);

    if (remaining > 0) {
        info_ = buffer.readString(remaining);
    }

    return true;
}

void OkPacket::encode(Buffer& buffer, uint8_t sequence_id) {
    Buffer payload;

    // header (0x00 for OK)
    payload.writeUint8(0x00);

    // affected rows
    payload.writeLenencInt(affected_rows_);

    // last insert id
    payload.writeLenencInt(last_insert_id_);

    // status flags
    payload.writeUint16(status_flags_);

    // warnings
    payload.writeUint16(warnings_);

    // info
    if (!info_.empty()) {
        payload.writeString(info_);
    }

    writeHeader(buffer, static_cast<uint32_t>(payload.readableBytes()), sequence_id);
    buffer.append(payload.peek(), payload.readableBytes());
}

size_t OkPacket::getPayloadLength() const {
    size_t len = 1; // header

    // lenenc int for affected_rows
    if (affected_rows_ < 251) len += 1;
    else if (affected_rows_ < 65536) len += 3;
    else if (affected_rows_ < 16777216) len += 4;
    else len += 9;

    // lenenc int for last_insert_id
    if (last_insert_id_ < 251) len += 1;
    else if (last_insert_id_ < 65536) len += 3;
    else if (last_insert_id_ < 16777216) len += 4;
    else len += 9;

    len += 2; // status flags
    len += 2; // warnings
    len += info_.size(); // info

    return len;
}

// ==================== ErrPacket ====================

ErrPacket::ErrPacket()
    : error_code_(0)
    , sql_state_("HY000")
    , error_message_("")
{}

ErrPacket::ErrPacket(uint16_t error_code, const std::string& sql_state,
                     const std::string& error_message)
    : error_code_(error_code)
    , sql_state_(sql_state)
    , error_message_(error_message)
{
    // 确保 sql_state 是5个字符
    if (sql_state_.size() != 5) {
        sql_state_ = "HY000";
    }
}

bool ErrPacket::decode(Buffer& buffer) {
    PacketHeader header;
    if (!readHeader(buffer, header)) {
        return false;
    }

    if (buffer.readableBytes() < header.payload_length) {
        return false;
    }

    uint8_t packet_header = buffer.readUint8();
    if (packet_header != 0xFF) {
        LOG_ERROR("Invalid ERR packet header: " << static_cast<int>(packet_header));
        return false;
    }

    error_code_ = buffer.readUint16();

    // sql_state marker
    uint8_t marker = buffer.readUint8();
    if (marker != '#') {
        LOG_WARN("Missing SQL state marker, expected '#'");
    }

    // sql_state (5 bytes)
    sql_state_ = buffer.readString(5);

    // error message (rest of payload)
    size_t msg_len = header.payload_length - 1 - 2 - 1 - 5;
    if (msg_len > 0) {
        error_message_ = buffer.readString(msg_len);
    }

    return true;
}

void ErrPacket::encode(Buffer& buffer, uint8_t sequence_id) {
    Buffer payload;

    // header (0xFF for ERR)
    payload.writeUint8(0xFF);

    // error code
    payload.writeUint16(error_code_);

    // sql_state marker
    payload.writeUint8('#');

    // sql_state (5 bytes)
    std::string state = sql_state_;
    if (state.size() != 5) {
        state = "HY000";
    }
    payload.writeString(state);

    // error message
    if (!error_message_.empty()) {
        payload.writeString(error_message_);
    }

    writeHeader(buffer, static_cast<uint32_t>(payload.readableBytes()), sequence_id);
    buffer.append(payload.peek(), payload.readableBytes());
}

size_t ErrPacket::getPayloadLength() const {
    return 1 + 2 + 1 + 5 + error_message_.size();
}

// ==================== EofPacket ====================

EofPacket::EofPacket()
    : warnings_(0)
    , status_flags_(0)
{}

EofPacket::EofPacket(uint16_t warnings, uint16_t status_flags)
    : warnings_(warnings)
    , status_flags_(status_flags)
{}

bool EofPacket::decode(Buffer& buffer) {
    PacketHeader header;
    if (!readHeader(buffer, header)) {
        return false;
    }

    if (buffer.readableBytes() < header.payload_length) {
        return false;
    }

    uint8_t packet_header = buffer.readUint8();
    if (packet_header != 0xFE) {
        LOG_ERROR("Invalid EOF packet header: " << static_cast<int>(packet_header));
        return false;
    }

    warnings_ = buffer.readUint16();
    status_flags_ = buffer.readUint16();

    return true;
}

void EofPacket::encode(Buffer& buffer, uint8_t sequence_id) {
    Buffer payload;

    // header (0xFE for EOF)
    payload.writeUint8(0xFE);

    // warnings
    payload.writeUint16(warnings_);

    // status flags
    payload.writeUint16(status_flags_);

    writeHeader(buffer, static_cast<uint32_t>(payload.readableBytes()), sequence_id);
    buffer.append(payload.peek(), payload.readableBytes());
}

size_t EofPacket::getPayloadLength() const {
    return 1 + 2 + 2; // header + warnings + status_flags
}

// ==================== 包类型识别 ====================

PacketType identifyPacketType(Buffer& buffer) {
    if (buffer.readableBytes() < 5) { // 至少需要4字节包头 + 1字节payload头
        return PacketType::UNKNOWN;
    }

    // 保存当前读指针位置
    size_t save_pos = buffer.readerIndex();

    // 跳过包头（4字节）
    buffer.retrieve(4);

    if (buffer.readableBytes() == 0) {
        buffer.setReaderIndex(save_pos);
        return PacketType::UNKNOWN;
    }

    // 读取payload的第一个字节
    uint8_t first_byte = buffer.readUint8();

    // 恢复读指针
    buffer.setReaderIndex(save_pos);

    // 根据第一个字节判断包类型
    if (first_byte == 0x00) {
        return PacketType::OK;
    } else if (first_byte == 0xFF) {
        return PacketType::ERR;
    } else if (first_byte == 0xFE) {
        // EOF包的payload长度小于9字节
        // 否则可能是其他类型的包（如大的字段值）
        size_t payload_len = (buffer.peek()[0]) |
                            (static_cast<size_t>(buffer.peek()[1]) << 8) |
                            (static_cast<size_t>(buffer.peek()[2]) << 16);
        if (payload_len < 9) {
            return PacketType::EOF_PKT;
        }
    } else if (first_byte == 0x0a) {
        // Handshake V10的协议版本号
        return PacketType::HANDSHAKE;
    } else if (first_byte == 0x03) {
        // COM_QUERY
        return PacketType::QUERY;
    }

    return PacketType::UNKNOWN;
}

} // namespace tiny_sql
