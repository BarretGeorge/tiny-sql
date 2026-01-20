#pragma once

#include "tiny_sql/protocol/packet.h"
#include <string>
#include <cstdint>
#include <array>

namespace tiny_sql {

/**
 * 服务器能力标志（Capability Flags）
 */
namespace CapabilityFlags {
    constexpr uint32_t CLIENT_LONG_PASSWORD = 0x00000001;
    constexpr uint32_t CLIENT_FOUND_ROWS = 0x00000002;
    constexpr uint32_t CLIENT_LONG_FLAG = 0x00000004;
    constexpr uint32_t CLIENT_CONNECT_WITH_DB = 0x00000008;
    constexpr uint32_t CLIENT_NO_SCHEMA = 0x00000010;
    constexpr uint32_t CLIENT_COMPRESS = 0x00000020;
    constexpr uint32_t CLIENT_ODBC = 0x00000040;
    constexpr uint32_t CLIENT_LOCAL_FILES = 0x00000080;
    constexpr uint32_t CLIENT_IGNORE_SPACE = 0x00000100;
    constexpr uint32_t CLIENT_PROTOCOL_41 = 0x00000200;
    constexpr uint32_t CLIENT_INTERACTIVE = 0x00000400;
    constexpr uint32_t CLIENT_SSL = 0x00000800;
    constexpr uint32_t CLIENT_IGNORE_SIGPIPE = 0x00001000;
    constexpr uint32_t CLIENT_TRANSACTIONS = 0x00002000;
    constexpr uint32_t CLIENT_RESERVED = 0x00004000;
    constexpr uint32_t CLIENT_SECURE_CONNECTION = 0x00008000;
    constexpr uint32_t CLIENT_MULTI_STATEMENTS = 0x00010000;
    constexpr uint32_t CLIENT_MULTI_RESULTS = 0x00020000;
    constexpr uint32_t CLIENT_PS_MULTI_RESULTS = 0x00040000;
    constexpr uint32_t CLIENT_PLUGIN_AUTH = 0x00080000;
    constexpr uint32_t CLIENT_CONNECT_ATTRS = 0x00100000;
    constexpr uint32_t CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA = 0x00200000;
    constexpr uint32_t CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS = 0x00400000;
    constexpr uint32_t CLIENT_SESSION_TRACK = 0x00800000;
    constexpr uint32_t CLIENT_DEPRECATE_EOF = 0x01000000;
}

/**
 * 字符集编码
 */
namespace Charset {
    constexpr uint8_t UTF8_GENERAL_CI = 33;
    constexpr uint8_t UTF8MB4_GENERAL_CI = 45;
    constexpr uint8_t UTF8MB4_0900_AI_CI = 255;
}

/**
 * 服务器状态标志
 */
namespace ServerStatus {
    constexpr uint16_t SERVER_STATUS_IN_TRANS = 0x0001;
    constexpr uint16_t SERVER_STATUS_AUTOCOMMIT = 0x0002;
    constexpr uint16_t SERVER_MORE_RESULTS_EXISTS = 0x0008;
    constexpr uint16_t SERVER_STATUS_NO_GOOD_INDEX_USED = 0x0010;
    constexpr uint16_t SERVER_STATUS_NO_INDEX_USED = 0x0020;
    constexpr uint16_t SERVER_STATUS_CURSOR_EXISTS = 0x0040;
    constexpr uint16_t SERVER_STATUS_LAST_ROW_SENT = 0x0080;
    constexpr uint16_t SERVER_STATUS_DB_DROPPED = 0x0100;
    constexpr uint16_t SERVER_STATUS_NO_BACKSLASH_ESCAPES = 0x0200;
    constexpr uint16_t SERVER_STATUS_METADATA_CHANGED = 0x0400;
    constexpr uint16_t SERVER_QUERY_WAS_SLOW = 0x0800;
    constexpr uint16_t SERVER_PS_OUT_PARAMS = 0x1000;
}

/**
 * Handshake V10 包（服务器发送给客户端）
 *
 * 包结构：
 * - 1 byte: protocol version (0x0a)
 * - string[NUL]: server version
 * - 4 bytes: connection id
 * - 8 bytes: auth-plugin-data-part-1 (challenge)
 * - 1 byte: filler (0x00)
 * - 2 bytes: capability flags (lower 2 bytes)
 * - 1 byte: character set
 * - 2 bytes: status flags
 * - 2 bytes: capability flags (upper 2 bytes)
 * - 1 byte: length of auth-plugin-data
 * - 10 bytes: reserved (all 0x00)
 * - string[$len]: auth-plugin-data-part-2
 * - string[NUL]: auth plugin name
 */
class HandshakeV10Packet : public Packet {
public:
    HandshakeV10Packet();

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override;

    // Getters
    uint8_t getProtocolVersion() const { return protocol_version_; }
    const std::string& getServerVersion() const { return server_version_; }
    uint32_t getConnectionId() const { return connection_id_; }
    const std::array<uint8_t, 20>& getAuthPluginData() const { return auth_plugin_data_; }
    uint32_t getCapabilityFlags() const { return capability_flags_; }
    uint8_t getCharacterSet() const { return character_set_; }
    uint16_t getStatusFlags() const { return status_flags_; }
    const std::string& getAuthPluginName() const { return auth_plugin_name_; }

    // Setters
    void setServerVersion(const std::string& version) { server_version_ = version; }
    void setConnectionId(uint32_t id) { connection_id_ = id; }
    void setAuthPluginData(const std::array<uint8_t, 20>& data) { auth_plugin_data_ = data; }
    void setCapabilityFlags(uint32_t flags) { capability_flags_ = flags; }
    void setCharacterSet(uint8_t charset) { character_set_ = charset; }
    void setStatusFlags(uint16_t flags) { status_flags_ = flags; }
    void setAuthPluginName(const std::string& name) { auth_plugin_name_ = name; }

    /**
     * 生成随机的认证挑战数据
     */
    void generateAuthPluginData();

private:
    uint8_t protocol_version_;              // 协议版本，固定为10
    std::string server_version_;            // 服务器版本字符串
    uint32_t connection_id_;                // 连接ID
    std::array<uint8_t, 20> auth_plugin_data_; // 认证挑战数据（20字节）
    uint32_t capability_flags_;             // 服务器能力标志
    uint8_t character_set_;                 // 默认字符集
    uint16_t status_flags_;                 // 服务器状态标志
    std::string auth_plugin_name_;          // 认证插件名称
};

/**
 * HandshakeResponse41 包（客户端响应）
 *
 * 包结构：
 * - 4 bytes: capability flags
 * - 4 bytes: max-packet size
 * - 1 byte: character set
 * - 23 bytes: reserved (all 0x00)
 * - string[NUL]: username
 * - lenenc-int: length of auth-response
 * - string[$len]: auth-response
 * - if CLIENT_CONNECT_WITH_DB:
 *   - string[NUL]: database
 * - if CLIENT_PLUGIN_AUTH:
 *   - string[NUL]: auth plugin name
 * - if CLIENT_CONNECT_ATTRS:
 *   - lenenc-int: length of all key-values
 *   - lenenc-str: key
 *   - lenenc-str: value
 *   - ... more key-values
 */
class HandshakeResponse41Packet : public Packet {
public:
    HandshakeResponse41Packet() = default;

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override;

    // Getters
    uint32_t getCapabilityFlags() const { return capability_flags_; }
    uint32_t getMaxPacketSize() const { return max_packet_size_; }
    uint8_t getCharacterSet() const { return character_set_; }
    const std::string& getUsername() const { return username_; }
    const std::vector<uint8_t>& getAuthResponse() const { return auth_response_; }
    const std::string& getDatabase() const { return database_; }
    const std::string& getAuthPluginName() const { return auth_plugin_name_; }

    // Setters
    void setCapabilityFlags(uint32_t flags) { capability_flags_ = flags; }
    void setMaxPacketSize(uint32_t size) { max_packet_size_ = size; }
    void setCharacterSet(uint8_t charset) { character_set_ = charset; }
    void setUsername(const std::string& username) { username_ = username; }
    void setAuthResponse(const std::vector<uint8_t>& response) { auth_response_ = response; }
    void setDatabase(const std::string& database) { database_ = database; }
    void setAuthPluginName(const std::string& name) { auth_plugin_name_ = name; }

private:
    uint32_t capability_flags_ = 0;
    uint32_t max_packet_size_ = 0;
    uint8_t character_set_ = 0;
    std::string username_;
    std::vector<uint8_t> auth_response_;
    std::string database_;
    std::string auth_plugin_name_;
};

} // namespace tiny_sql
