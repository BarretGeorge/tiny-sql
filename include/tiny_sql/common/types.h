#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tiny_sql {

// 字段类型枚举 (MySQL field types)
enum class FieldType : uint8_t {
    MYSQL_TYPE_DECIMAL = 0x00,
    MYSQL_TYPE_TINY = 0x01,
    MYSQL_TYPE_SHORT = 0x02,
    MYSQL_TYPE_LONG = 0x03,
    MYSQL_TYPE_FLOAT = 0x04,
    MYSQL_TYPE_DOUBLE = 0x05,
    MYSQL_TYPE_NULL = 0x06,
    MYSQL_TYPE_TIMESTAMP = 0x07,
    MYSQL_TYPE_LONGLONG = 0x08,
    MYSQL_TYPE_INT24 = 0x09,
    MYSQL_TYPE_DATE = 0x0a,
    MYSQL_TYPE_TIME = 0x0b,
    MYSQL_TYPE_DATETIME = 0x0c,
    MYSQL_TYPE_YEAR = 0x0d,
    MYSQL_TYPE_NEWDATE = 0x0e,
    MYSQL_TYPE_VARCHAR = 0x0f,
    MYSQL_TYPE_BIT = 0x10,
    MYSQL_TYPE_NEWDECIMAL = 0xf6,
    MYSQL_TYPE_ENUM = 0xf7,
    MYSQL_TYPE_SET = 0xf8,
    MYSQL_TYPE_TINY_BLOB = 0xf9,
    MYSQL_TYPE_MEDIUM_BLOB = 0xfa,
    MYSQL_TYPE_LONG_BLOB = 0xfb,
    MYSQL_TYPE_BLOB = 0xfc,
    MYSQL_TYPE_VAR_STRING = 0xfd,
    MYSQL_TYPE_STRING = 0xfe,
    MYSQL_TYPE_GEOMETRY = 0xff
};

// MySQL命令类型
enum class MySQLCommand : uint8_t {
    COM_SLEEP = 0x00,
    COM_QUIT = 0x01,
    COM_INIT_DB = 0x02,
    COM_QUERY = 0x03,
    COM_FIELD_LIST = 0x04,
    COM_CREATE_DB = 0x05,
    COM_DROP_DB = 0x06,
    COM_REFRESH = 0x07,
    COM_SHUTDOWN = 0x08,
    COM_STATISTICS = 0x09,
    COM_PROCESS_INFO = 0x0a,
    COM_CONNECT = 0x0b,
    COM_PROCESS_KILL = 0x0c,
    COM_DEBUG = 0x0d,
    COM_PING = 0x0e,
    COM_TIME = 0x0f,
    COM_DELAYED_INSERT = 0x10,
    COM_CHANGE_USER = 0x11,
    COM_BINLOG_DUMP = 0x12,
    COM_TABLE_DUMP = 0x13,
    COM_CONNECT_OUT = 0x14,
    COM_REGISTER_SLAVE = 0x15,
    COM_STMT_PREPARE = 0x16,
    COM_STMT_EXECUTE = 0x17,
    COM_STMT_SEND_LONG_DATA = 0x18,
    COM_STMT_CLOSE = 0x19,
    COM_STMT_RESET = 0x1a,
    COM_SET_OPTION = 0x1b,
    COM_STMT_FETCH = 0x1c,
    COM_DAEMON = 0x1d,
    COM_BINLOG_DUMP_GTID = 0x1e,
    COM_RESET_CONNECTION = 0x1f
};

// 向后兼容的别名
using Command = MySQLCommand;

} // namespace tiny_sql
