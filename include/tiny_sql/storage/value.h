#pragma once

#include <string>
#include <variant>
#include <memory>
#include <cstdint>

namespace tiny_sql {

/**
 * SQL数据类型枚举
 */
enum class DataType {
    INT,
    BIGINT,
    FLOAT,
    DOUBLE,
    VARCHAR,
    TEXT,
    BOOLEAN,
    NULL_TYPE
};

/**
 * 值类型 - 使用variant实现多态
 */
class Value {
public:
    using ValueVariant = std::variant<
        std::nullptr_t,     // NULL
        int32_t,            // INT
        int64_t,            // BIGINT
        float,              // FLOAT
        double,             // DOUBLE
        std::string,        // VARCHAR/TEXT
        bool                // BOOLEAN
    >;

    // 构造函数
    Value() : data_(nullptr) {}
    explicit Value(int32_t val) : data_(val) {}
    explicit Value(int64_t val) : data_(val) {}
    explicit Value(float val) : data_(val) {}
    explicit Value(double val) : data_(val) {}
    explicit Value(const std::string& val) : data_(val) {}
    explicit Value(const char* val) : data_(std::string(val)) {}
    explicit Value(bool val) : data_(val) {}

    // 静态工厂方法
    static Value Null() { return Value(); }

    // 类型判断
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(data_); }
    bool isInt() const { return std::holds_alternative<int32_t>(data_); }
    bool isBigInt() const { return std::holds_alternative<int64_t>(data_); }
    bool isFloat() const { return std::holds_alternative<float>(data_); }
    bool isDouble() const { return std::holds_alternative<double>(data_); }
    bool isString() const { return std::holds_alternative<std::string>(data_); }
    bool isBool() const { return std::holds_alternative<bool>(data_); }

    // 获取值
    int32_t asInt() const { return std::get<int32_t>(data_); }
    int64_t asBigInt() const { return std::get<int64_t>(data_); }
    float asFloat() const { return std::get<float>(data_); }
    double asDouble() const { return std::get<double>(data_); }
    const std::string& asString() const { return std::get<std::string>(data_); }
    bool asBool() const { return std::get<bool>(data_); }

    // 获取数据类型
    DataType getType() const;

    // 转换为字符串（用于显示）
    std::string toString() const;

    // 比较操作
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;

    // 获取底层variant
    const ValueVariant& getData() const { return data_; }

private:
    ValueVariant data_;
};

/**
 * 列定义
 */
struct ColumnDef {
    std::string name;
    DataType type;
    bool primary_key = false;
    bool not_null = false;
    bool auto_increment = false;
    Value default_value;

    ColumnDef() = default;
    ColumnDef(const std::string& n, DataType t)
        : name(n), type(t) {}
};

} // namespace tiny_sql
