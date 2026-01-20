#include "tiny_sql/sql/ast.h"
#include <sstream>

namespace tiny_sql {

std::string SelectStatement::toString() const {
    std::ostringstream oss;
    oss << "SELECT ";

    if (columns_.empty()) {
        oss << "*";
    } else {
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << columns_[i]->toString();
        }
    }

    if (!table_name_.empty()) {
        oss << " FROM " << table_name_;
    }

    if (where_clause_) {
        oss << " WHERE " << where_clause_->toString();
    }

    if (limit_ >= 0) {
        oss << " LIMIT " << limit_;
    }

    if (offset_ > 0) {
        oss << " OFFSET " << offset_;
    }

    return oss.str();
}

std::string InsertStatement::toString() const {
    std::ostringstream oss;
    oss << "INSERT INTO " << table_name_;

    if (!columns_.empty()) {
        oss << " (";
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << columns_[i];
        }
        oss << ")";
    }

    oss << " VALUES (";
    for (size_t i = 0; i < values_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << values_[i]->toString();
    }
    oss << ")";

    return oss.str();
}

std::string CreateTableStatement::toString() const {
    std::ostringstream oss;
    oss << "CREATE TABLE " << table_name_ << " (";

    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0) oss << ", ";

        const auto& col = columns_[i];
        oss << col.name << " " << col.type;

        if (col.not_null) {
            oss << " NOT NULL";
        }

        if (col.primary_key) {
            oss << " PRIMARY KEY";
        }

        if (col.auto_increment) {
            oss << " AUTO_INCREMENT";
        }

        if (!col.default_value.empty()) {
            oss << " DEFAULT " << col.default_value;
        }
    }

    oss << ")";
    return oss.str();
}

} // namespace tiny_sql
