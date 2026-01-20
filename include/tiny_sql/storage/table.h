#pragma once

#include "tiny_sql/storage/value.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace tiny_sql {

/**
 * 行数据 - 表示一行记录
 */
class Row {
public:
    Row() = default;
    explicit Row(const std::vector<Value>& values) : values_(values) {}

    // 添加值
    void addValue(const Value& value) {
        values_.push_back(value);
    }

    // 获取值
    const Value& getValue(size_t index) const {
        return values_.at(index);
    }

    Value& getValue(size_t index) {
        return values_.at(index);
    }

    // 设置值
    void setValue(size_t index, const Value& value) {
        values_[index] = value;
    }

    // 获取列数
    size_t getColumnCount() const {
        return values_.size();
    }

    // 获取所有值
    const std::vector<Value>& getValues() const {
        return values_;
    }

    // 转换为字符串
    std::string toString() const;

private:
    std::vector<Value> values_;
};

/**
 * 表 - 表示一个数据库表
 */
class Table {
public:
    explicit Table(const std::string& name) : name_(name) {}

    // 获取表名
    const std::string& getName() const { return name_; }

    // 添加列定义
    void addColumn(const ColumnDef& column);

    // 获取列定义
    const std::vector<ColumnDef>& getColumns() const { return columns_; }

    // 获取列索引
    int getColumnIndex(const std::string& column_name) const;

    // 获取列数
    size_t getColumnCount() const { return columns_.size(); }

    // 插入行
    bool insertRow(const Row& row);

    // 获取所有行
    const std::vector<Row>& getRows() const { return rows_; }

    // 获取行数
    size_t getRowCount() const { return rows_.size(); }

    // 查找主键列索引
    int getPrimaryKeyIndex() const;

    // 查找自增列索引
    int getAutoIncrementIndex() const;

    // 获取下一个自增值
    int64_t getNextAutoIncrementValue();

    // 清空所有数据（保留表结构）
    void truncate() {
        rows_.clear();
        next_auto_increment_ = 1;
    }

    // 转换为字符串（显示表结构）
    std::string toString() const;

private:
    std::string name_;
    std::vector<ColumnDef> columns_;
    std::unordered_map<std::string, size_t> column_index_map_;
    std::vector<Row> rows_;
    int64_t next_auto_increment_ = 1;
};

} // namespace tiny_sql
