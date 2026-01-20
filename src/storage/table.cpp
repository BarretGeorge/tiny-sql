#include "tiny_sql/storage/table.h"
#include "tiny_sql/common/logger.h"
#include <sstream>

namespace tiny_sql {

std::string Row::toString() const {
    std::ostringstream oss;
    oss << "(";
    for (size_t i = 0; i < values_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << values_[i].toString();
    }
    oss << ")";
    return oss.str();
}

void Table::addColumn(const ColumnDef& column) {
    column_index_map_[column.name] = columns_.size();
    columns_.push_back(column);
}

int Table::getColumnIndex(const std::string& column_name) const {
    auto it = column_index_map_.find(column_name);
    if (it != column_index_map_.end()) {
        return static_cast<int>(it->second);
    }
    return -1;
}

bool Table::insertRow(const Row& row) {
    // 验证列数匹配
    if (row.getColumnCount() != columns_.size()) {
        LOG_ERROR("Column count mismatch: expected " << columns_.size()
                  << ", got " << row.getColumnCount());
        return false;
    }

    // 验证数据类型（简单验证）
    for (size_t i = 0; i < columns_.size(); ++i) {
        const auto& value = row.getValue(i);
        const auto& column = columns_[i];

        // 检查NOT NULL约束
        if (column.not_null && value.isNull()) {
            LOG_ERROR("Column " << column.name << " cannot be NULL");
            return false;
        }
    }

    rows_.push_back(row);
    return true;
}

int Table::getPrimaryKeyIndex() const {
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].primary_key) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Table::getAutoIncrementIndex() const {
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].auto_increment) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int64_t Table::getNextAutoIncrementValue() {
    return next_auto_increment_++;
}

std::string Table::toString() const {
    std::ostringstream oss;
    oss << "Table: " << name_ << "\n";
    oss << "Columns:\n";
    for (const auto& col : columns_) {
        oss << "  - " << col.name << " ";
        switch (col.type) {
            case DataType::INT: oss << "INT"; break;
            case DataType::BIGINT: oss << "BIGINT"; break;
            case DataType::FLOAT: oss << "FLOAT"; break;
            case DataType::DOUBLE: oss << "DOUBLE"; break;
            case DataType::VARCHAR: oss << "VARCHAR"; break;
            case DataType::TEXT: oss << "TEXT"; break;
            case DataType::BOOLEAN: oss << "BOOLEAN"; break;
            default: oss << "UNKNOWN"; break;
        }
        if (col.primary_key) oss << " PRIMARY KEY";
        if (col.auto_increment) oss << " AUTO_INCREMENT";
        if (col.not_null) oss << " NOT NULL";
        oss << "\n";
    }
    oss << "Rows: " << rows_.size();
    return oss.str();
}

} // namespace tiny_sql
