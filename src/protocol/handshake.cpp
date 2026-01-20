#include "tiny_sql/protocol/handshake.h"
#include "tiny_sql/common/logger.h"
#include <random>
#include <cstring>

namespace tiny_sql {

HandshakeV10Packet::HandshakeV10Packet()
    : protocol_version_(10)
    , server_version_("1.0.0-tiny-sql")
    , connection_id_(0)
    , capability_flags_(
        CapabilityFlags::CLIENT_LONG_PASSWORD |
        CapabilityFlags::CLIENT_PROTOCOL_41 |
        CapabilityFlags::CLIENT_SECURE_CONNECTION |
        CapabilityFlags::CLIENT_PLUGIN_AUTH |
        CapabilityFlags::CLIENT_CONNECT_WITH_DB |
        CapabilityFlags::CLIENT_DEPRECATE_EOF
      )
    , character_set_(Charset::UTF8MB4_GENERAL_CI)
    , status_flags_(ServerStatus::SERVER_STATUS_AUTOCOMMIT)
    , auth_plugin_name_("mysql_native_password")
{
    auth_plugin_data_.fill(0);
    generateAuthPluginData();
}

void HandshakeV10Packet::generateAuthPluginData() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (size_t i = 0; i < auth_plugin_data_.size(); ++i) {
        auth_plugin_data_[i] = static_cast<uint8_t>(dis(gen));
    }
}

bool HandshakeV10Packet::decode(Buffer& buffer) {
    // Handshake V10 通常由服务器发送，客户端不需要解析
    // 这里提供基础实现以支持测试
    PacketHeader header;
    if (!readHeader(buffer, header)) {
        return false;
    }

    if (buffer.readableBytes() < header.payload_length) {
        return false;
    }

    protocol_version_ = buffer.readUint8();
    server_version_ = buffer.readNullTerminatedString();
    connection_id_ = buffer.readUint32();

    // 读取前8字节auth data
    for (int i = 0; i < 8; ++i) {
        auth_plugin_data_[i] = buffer.readUint8();
    }

    buffer.readUint8(); // filler

    // 能力标志低2字节
    uint16_t cap_lower = buffer.readUint16();

    character_set_ = buffer.readUint8();
    status_flags_ = buffer.readUint16();

    // 能力标志高2字节
    uint16_t cap_upper = buffer.readUint16();
    capability_flags_ = (static_cast<uint32_t>(cap_upper) << 16) | cap_lower;

    uint8_t auth_plugin_data_len = buffer.readUint8();

    // 保留字节
    for (int i = 0; i < 10; ++i) {
        buffer.readUint8();
    }

    // 读取剩余的auth data
    size_t remaining_auth_len = std::max(13, static_cast<int>(auth_plugin_data_len) - 8);
    for (size_t i = 0; i < remaining_auth_len && i < 12; ++i) {
        auth_plugin_data_[8 + i] = buffer.readUint8();
    }

    // 读取认证插件名称
    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH) {
        auth_plugin_name_ = buffer.readNullTerminatedString();
    }

    return true;
}

void HandshakeV10Packet::encode(Buffer& buffer, uint8_t sequence_id) {
    Buffer payload;

    // 协议版本
    payload.writeUint8(protocol_version_);

    // 服务器版本
    payload.writeString(server_version_);
    payload.writeUint8(0); // null terminator

    // 连接ID
    payload.writeUint32(connection_id_);

    // auth-plugin-data-part-1 (前8字节)
    for (int i = 0; i < 8; ++i) {
        payload.writeUint8(auth_plugin_data_[i]);
    }

    // filler
    payload.writeUint8(0);

    // capability flags 低2字节
    payload.writeUint16(static_cast<uint16_t>(capability_flags_ & 0xFFFF));

    // character set
    payload.writeUint8(character_set_);

    // status flags
    payload.writeUint16(status_flags_);

    // capability flags 高2字节
    payload.writeUint16(static_cast<uint16_t>((capability_flags_ >> 16) & 0xFFFF));

    // auth plugin data length
    payload.writeUint8(21); // 固定为21（20字节数据 + 1字节结尾）

    // 保留10字节
    for (int i = 0; i < 10; ++i) {
        payload.writeUint8(0);
    }

    // auth-plugin-data-part-2 (后12字节)
    for (int i = 0; i < 12; ++i) {
        payload.writeUint8(auth_plugin_data_[8 + i]);
    }
    payload.writeUint8(0); // null terminator

    // auth plugin name
    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH) {
        payload.writeString(auth_plugin_name_);
        payload.writeUint8(0); // null terminator
    }

    // 写入包头和payload
    writeHeader(buffer, static_cast<uint32_t>(payload.readableBytes()), sequence_id);
    buffer.append(payload.peek(), payload.readableBytes());
}

size_t HandshakeV10Packet::getPayloadLength() const {
    size_t len = 0;
    len += 1; // protocol version
    len += server_version_.size() + 1; // server version + null
    len += 4; // connection id
    len += 8; // auth-plugin-data-part-1
    len += 1; // filler
    len += 2; // capability flags lower
    len += 1; // character set
    len += 2; // status flags
    len += 2; // capability flags upper
    len += 1; // auth plugin data len
    len += 10; // reserved
    len += 12; // auth-plugin-data-part-2
    len += 1; // null terminator
    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH) {
        len += auth_plugin_name_.size() + 1;
    }
    return len;
}

bool HandshakeResponse41Packet::decode(Buffer& buffer) {
    PacketHeader header;
    if (!readHeader(buffer, header)) {
        return false;
    }

    if (buffer.readableBytes() < header.payload_length) {
        return false;
    }

    size_t start_pos = buffer.readerIndex();

    // capability flags
    capability_flags_ = buffer.readUint32();

    // max packet size
    max_packet_size_ = buffer.readUint32();

    // character set
    character_set_ = buffer.readUint8();

    // reserved (23 bytes)
    for (int i = 0; i < 23; ++i) {
        buffer.readUint8();
    }

    // username
    username_ = buffer.readNullTerminatedString();

    // auth response
    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) {
        // length encoded
        uint64_t auth_len = buffer.readLenencInt();
        auth_response_ = buffer.retrieve(static_cast<size_t>(auth_len));
    } else if (capability_flags_ & CapabilityFlags::CLIENT_SECURE_CONNECTION) {
        // 1 byte length
        uint8_t auth_len = buffer.readUint8();
        auth_response_ = buffer.retrieve(auth_len);
    } else {
        // null terminated
        std::string auth_str = buffer.readNullTerminatedString();
        auth_response_.assign(auth_str.begin(), auth_str.end());
    }

    // database (if CLIENT_CONNECT_WITH_DB is set)
    if (capability_flags_ & CapabilityFlags::CLIENT_CONNECT_WITH_DB) {
        database_ = buffer.readNullTerminatedString();
    }

    // auth plugin name
    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH) {
        auth_plugin_name_ = buffer.readNullTerminatedString();
    }

    // 跳过connect attributes（如果有）
    // 这里暂时简单处理

    return true;
}

void HandshakeResponse41Packet::encode(Buffer& buffer, uint8_t sequence_id) {
    Buffer payload;

    payload.writeUint32(capability_flags_);
    payload.writeUint32(max_packet_size_);
    payload.writeUint8(character_set_);

    // reserved 23 bytes
    for (int i = 0; i < 23; ++i) {
        payload.writeUint8(0);
    }

    // username
    payload.writeString(username_);
    payload.writeUint8(0);

    // auth response
    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) {
        payload.writeLenencInt(auth_response_.size());
        payload.append(auth_response_.data(), auth_response_.size());
    } else if (capability_flags_ & CapabilityFlags::CLIENT_SECURE_CONNECTION) {
        payload.writeUint8(static_cast<uint8_t>(auth_response_.size()));
        payload.append(auth_response_.data(), auth_response_.size());
    } else {
        payload.append(auth_response_.data(), auth_response_.size());
        payload.writeUint8(0);
    }

    // database
    if (capability_flags_ & CapabilityFlags::CLIENT_CONNECT_WITH_DB) {
        payload.writeString(database_);
        payload.writeUint8(0);
    }

    // auth plugin name
    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH) {
        payload.writeString(auth_plugin_name_);
        payload.writeUint8(0);
    }

    writeHeader(buffer, static_cast<uint32_t>(payload.readableBytes()), sequence_id);
    buffer.append(payload.peek(), payload.readableBytes());
}

size_t HandshakeResponse41Packet::getPayloadLength() const {
    size_t len = 0;
    len += 4; // capability flags
    len += 4; // max packet size
    len += 1; // character set
    len += 23; // reserved
    len += username_.size() + 1;

    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) {
        // lenenc int + data
        if (auth_response_.size() < 251) len += 1;
        else if (auth_response_.size() < 65536) len += 3;
        else if (auth_response_.size() < 16777216) len += 4;
        else len += 9;
        len += auth_response_.size();
    } else if (capability_flags_ & CapabilityFlags::CLIENT_SECURE_CONNECTION) {
        len += 1 + auth_response_.size();
    } else {
        len += auth_response_.size() + 1;
    }

    if (capability_flags_ & CapabilityFlags::CLIENT_CONNECT_WITH_DB) {
        len += database_.size() + 1;
    }

    if (capability_flags_ & CapabilityFlags::CLIENT_PLUGIN_AUTH) {
        len += auth_plugin_name_.size() + 1;
    }

    return len;
}

} // namespace tiny_sql
