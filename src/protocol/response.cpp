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

// ==================== Helper Functions ====================

/**
 * 将DataType转换为MySQL字段类型
 * Convert DataType to MySQL field type
 */
static uint8_t dataTypeToMySQLType(DataType type) {
    switch (type) {
        case DataType::INT:
            return MySQLFieldType::MYSQL_TYPE_LONG;
        case DataType::BIGINT:
            return MySQLFieldType::MYSQL_TYPE_LONGLONG;
        case DataType::FLOAT:
            return MySQLFieldType::MYSQL_TYPE_FLOAT;
        case DataType::DOUBLE:
            return MySQLFieldType::MYSQL_TYPE_DOUBLE;
        case DataType::VARCHAR:
        case DataType::TEXT:
            return MySQLFieldType::MYSQL_TYPE_STRING;
        case DataType::BOOLEAN:
            return MySQLFieldType::MYSQL_TYPE_TINY;
        case DataType::NULL_TYPE:
        default:
            return MySQLFieldType::MYSQL_TYPE_STRING;
    }
}

/**
 * 获取列的最大长度
 * Get maximum length for a column
 */
static uint32_t getColumnLength(DataType type) {
    switch (type) {
        case DataType::INT:
            return 11;  // -2147483648
        case DataType::BIGINT:
            return 20;  // -9223372036854775808
        case DataType::FLOAT:
            return 12;
        case DataType::DOUBLE:
            return 22;
        case DataType::BOOLEAN:
            return 1;
        case DataType::VARCHAR:
            return 255;
        case DataType::TEXT:
            return 65535;
        case DataType::NULL_TYPE:
        default:
            return 0;
    }
}

// ==================== ColumnDefinitionPacket ====================

ColumnDefinitionPacket::ColumnDefinitionPacket()
    : catalog_("def")
    , schema_("")
    , table_("")
    , org_table_("")
    , name_("")
    , org_name_("")
    , charset_(MySQLCharset::UTF8_GENERAL_CI)
    , column_length_(0)
    , column_type_(MySQLFieldType::MYSQL_TYPE_STRING)
    , flags_(0)
    , decimals_(0)
{}

ColumnDefinitionPacket::ColumnDefinitionPacket(
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
    uint8_t decimals)
    : catalog_(catalog)
    , schema_(schema)
    , table_(table)
    , org_table_(org_table)
    , name_(name)
    , org_name_(org_name)
    , charset_(charset)
    , column_length_(column_length)
    , column_type_(column_type)
    , flags_(flags)
    , decimals_(decimals)
{}

ColumnDefinitionPacket ColumnDefinitionPacket::fromColumnDef(
    const ColumnDef& col,
    const std::string& table_name,
    const std::string& db_name) {

    uint16_t flags = 0;
    if (col.not_null) {
        flags |= ColumnFlags::NOT_NULL_FLAG;
    }
    if (col.primary_key) {
        flags |= ColumnFlags::PRI_KEY_FLAG;
    }
    if (col.auto_increment) {
        flags |= ColumnFlags::AUTO_INCREMENT_FLAG;
    }

    return ColumnDefinitionPacket(
        "def",                              // catalog
        db_name,                            // schema
        table_name,                         // table
        table_name,                         // org_table
        col.name,                           // name
        col.name,                           // org_name
        MySQLCharset::UTF8_GENERAL_CI,     // charset
        getColumnLength(col.type),          // column_length
        dataTypeToMySQLType(col.type),     // column_type
        flags,                              // flags
        0                                   // decimals
    );
}

bool ColumnDefinitionPacket::decode(Buffer& buffer) {
    // 解码暂不实现（服务器只需encode）
    // Decode not implemented (server only needs encode)
    return false;
}

void ColumnDefinitionPacket::encode(Buffer& buffer, uint8_t sequence_id) {
    Buffer payload;

    // catalog (length-encoded string)
    payload.writeLenencString(catalog_);

    // schema (length-encoded string)
    payload.writeLenencString(schema_);

    // table (length-encoded string)
    payload.writeLenencString(table_);

    // org_table (length-encoded string)
    payload.writeLenencString(org_table_);

    // name (length-encoded string)
    payload.writeLenencString(name_);

    // org_name (length-encoded string)
    payload.writeLenencString(org_name_);

    // fixed length fields (0x0c)
    payload.writeUint8(0x0c);

    // charset (2 bytes, little-endian)
    payload.writeUint16(charset_);

    // column_length (4 bytes, little-endian)
    payload.writeUint32(column_length_);

    // type (1 byte)
    payload.writeUint8(column_type_);

    // flags (2 bytes, little-endian)
    payload.writeUint16(flags_);

    // decimals (1 byte)
    payload.writeUint8(decimals_);

    // filler (2 bytes, 0x00 0x00)
    payload.writeUint16(0x0000);

    // 写包头和payload
    // Write header and payload
    writeHeader(buffer, static_cast<uint32_t>(payload.readableBytes()), sequence_id);
    buffer.append(payload.peek(), payload.readableBytes());
}

size_t ColumnDefinitionPacket::getPayloadLength() const {
    size_t len = 0;

    // lenenc strings
    len += 1 + catalog_.size();
    len += 1 + schema_.size();
    len += 1 + table_.size();
    len += 1 + org_table_.size();
    len += 1 + name_.size();
    len += 1 + org_name_.size();

    // fixed length fields
    len += 1;  // 0x0c
    len += 2;  // charset
    len += 4;  // column_length
    len += 1;  // type
    len += 2;  // flags
    len += 1;  // decimals
    len += 2;  // filler

    return len;
}

// ==================== TextResultRowPacket ====================

TextResultRowPacket::TextResultRowPacket()
{}

TextResultRowPacket::TextResultRowPacket(const Row& row) {
    for (size_t i = 0; i < row.getColumnCount(); ++i) {
        values_.push_back(row.getValue(i));
    }
}

bool TextResultRowPacket::decode(Buffer& buffer) {
    // 解码暂不实现（服务器只需encode）
    // Decode not implemented (server only needs encode)
    return false;
}

void TextResultRowPacket::encode(Buffer& buffer, uint8_t sequence_id) {
    Buffer payload;

    // 对每个值编码为length-encoded string
    // Encode each value as length-encoded string
    for (const auto& value : values_) {
        if (value.isNull()) {
            // NULL值用0xFB表示
            // NULL value is represented by 0xFB
            payload.writeUint8(0xFB);
        } else {
            // 其他值转换为字符串后用length-encoded string编码
            // Other values converted to string then encoded as length-encoded string
            std::string str = value.toString();
            payload.writeLenencString(str);
        }
    }

    // 写包头和payload
    // Write header and payload
    writeHeader(buffer, static_cast<uint32_t>(payload.readableBytes()), sequence_id);
    buffer.append(payload.peek(), payload.readableBytes());
}

size_t TextResultRowPacket::getPayloadLength() const {
    size_t len = 0;

    for (const auto& value : values_) {
        if (value.isNull()) {
            len += 1;  // 0xFB
        } else {
            std::string str = value.toString();
            // lenenc string: length + data
            if (str.size() < 251) {
                len += 1 + str.size();
            } else if (str.size() < 65536) {
                len += 3 + str.size();
            } else if (str.size() < 16777216) {
                len += 4 + str.size();
            } else {
                len += 9 + str.size();
            }
        }
    }

    return len;
}

void TextResultRowPacket::addValue(const Value& value) {
    values_.push_back(value);
}

} // namespace tiny_sql
