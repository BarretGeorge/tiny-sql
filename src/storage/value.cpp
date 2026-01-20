#include "tiny_sql/storage/value.h"
#include <sstream>
#include <iomanip>

namespace tiny_sql {

DataType Value::getType() const {
    if (isNull()) return DataType::NULL_TYPE;
    if (isInt()) return DataType::INT;
    if (isBigInt()) return DataType::BIGINT;
    if (isFloat()) return DataType::FLOAT;
    if (isDouble()) return DataType::DOUBLE;
    if (isString()) return DataType::VARCHAR;
    if (isBool()) return DataType::BOOLEAN;
    return DataType::NULL_TYPE;
}

std::string Value::toString() const {
    if (isNull()) return "NULL";
    if (isInt()) return std::to_string(asInt());
    if (isBigInt()) return std::to_string(asBigInt());
    if (isFloat()) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << asFloat();
        return oss.str();
    }
    if (isDouble()) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4) << asDouble();
        return oss.str();
    }
    if (isString()) return asString();
    if (isBool()) return asBool() ? "TRUE" : "FALSE";
    return "UNKNOWN";
}

bool Value::operator==(const Value& other) const {
    if (data_.index() != other.data_.index()) {
        return false;
    }
    return data_ == other.data_;
}

bool Value::operator<(const Value& other) const {
    if (data_.index() != other.data_.index()) {
        return data_.index() < other.data_.index();
    }

    if (isInt()) return asInt() < other.asInt();
    if (isBigInt()) return asBigInt() < other.asBigInt();
    if (isFloat()) return asFloat() < other.asFloat();
    if (isDouble()) return asDouble() < other.asDouble();
    if (isString()) return asString() < other.asString();
    if (isBool()) return asBool() < other.asBool();

    return false;
}

bool Value::operator<=(const Value& other) const {
    return *this < other || *this == other;
}

bool Value::operator>(const Value& other) const {
    return !(*this <= other);
}

bool Value::operator>=(const Value& other) const {
    return !(*this < other);
}

} // namespace tiny_sql
