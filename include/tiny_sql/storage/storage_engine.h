#pragma once

#include "tiny_sql/storage/table.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace tiny_sql {

/**
 * 数据库 - 包含多个表
 */
class Database {
public:
    explicit Database(const std::string& name) : name_(name) {}

    // 获取数据库名
    const std::string& getName() const { return name_; }

    // 创建表
    bool createTable(std::shared_ptr<Table> table);

    // 删除表
    bool dropTable(const std::string& table_name);

    // 获取表
    std::shared_ptr<Table> getTable(const std::string& table_name);

    // 检查表是否存在
    bool hasTable(const std::string& table_name) const;

    // 获取所有表名
    std::vector<std::string> getTableNames() const;

    // 获取表数量
    size_t getTableCount() const { return tables_.size(); }

private:
    std::string name_;
    std::unordered_map<std::string, std::shared_ptr<Table>> tables_;
    mutable std::mutex mutex_;
};

/**
 * 存储引擎 - 管理所有数据库
 */
class StorageEngine {
public:
    StorageEngine();
    ~StorageEngine() = default;

    // 单例模式
    static StorageEngine& instance();

    // 创建数据库
    bool createDatabase(const std::string& db_name);

    // 删除数据库
    bool dropDatabase(const std::string& db_name);

    // 获取数据库
    std::shared_ptr<Database> getDatabase(const std::string& db_name);

    // 检查数据库是否存在
    bool hasDatabase(const std::string& db_name) const;

    // 获取所有数据库名
    std::vector<std::string> getDatabaseNames() const;

    // 获取或创建数据库（如果不存在则创建）
    std::shared_ptr<Database> getOrCreateDatabase(const std::string& db_name);

private:
    std::unordered_map<std::string, std::shared_ptr<Database>> databases_;
    mutable std::mutex mutex_;
};

} // namespace tiny_sql
