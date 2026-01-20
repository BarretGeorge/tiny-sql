#include "tiny_sql/storage/storage_engine.h"
#include "tiny_sql/common/logger.h"
#include <algorithm>

namespace tiny_sql {

// ==================== Database ====================

bool Database::createTable(std::shared_ptr<Table> table) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string& table_name = table->getName();
    if (tables_.find(table_name) != tables_.end()) {
        LOG_WARN("Table " << table_name << " already exists in database " << name_);
        return false;
    }

    tables_[table_name] = table;
    LOG_INFO("Created table " << table_name << " in database " << name_);
    return true;
}

bool Database::dropTable(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        LOG_WARN("Table " << table_name << " does not exist in database " << name_);
        return false;
    }

    tables_.erase(it);
    LOG_INFO("Dropped table " << table_name << " from database " << name_);
    return true;
}

std::shared_ptr<Table> Database::getTable(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tables_.find(table_name);
    if (it != tables_.end()) {
        return it->second;
    }
    return nullptr;
}

bool Database::hasTable(const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tables_.find(table_name) != tables_.end();
}

std::vector<std::string> Database::getTableNames() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names;
    names.reserve(tables_.size());
    for (const auto& pair : tables_) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// ==================== StorageEngine ====================

StorageEngine::StorageEngine() {
    // 创建默认数据库
    databases_["mysql"] = std::make_shared<Database>("mysql");
    databases_["test"] = std::make_shared<Database>("test");
    LOG_INFO("StorageEngine initialized with default databases: mysql, test");
}

StorageEngine& StorageEngine::instance() {
    static StorageEngine engine;
    return engine;
}

bool StorageEngine::createDatabase(const std::string& db_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (databases_.find(db_name) != databases_.end()) {
        LOG_WARN("Database " << db_name << " already exists");
        return false;
    }

    databases_[db_name] = std::make_shared<Database>(db_name);
    LOG_INFO("Created database: " << db_name);
    return true;
}

bool StorageEngine::dropDatabase(const std::string& db_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 不允许删除系统数据库
    if (db_name == "mysql" || db_name == "information_schema") {
        LOG_ERROR("Cannot drop system database: " << db_name);
        return false;
    }

    auto it = databases_.find(db_name);
    if (it == databases_.end()) {
        LOG_WARN("Database " << db_name << " does not exist");
        return false;
    }

    databases_.erase(it);
    LOG_INFO("Dropped database: " << db_name);
    return true;
}

std::shared_ptr<Database> StorageEngine::getDatabase(const std::string& db_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = databases_.find(db_name);
    if (it != databases_.end()) {
        return it->second;
    }
    return nullptr;
}

bool StorageEngine::hasDatabase(const std::string& db_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return databases_.find(db_name) != databases_.end();
}

std::vector<std::string> StorageEngine::getDatabaseNames() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names;
    names.reserve(databases_.size());
    for (const auto& pair : databases_) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::shared_ptr<Database> StorageEngine::getOrCreateDatabase(const std::string& db_name) {
    auto db = getDatabase(db_name);
    if (!db) {
        createDatabase(db_name);
        db = getDatabase(db_name);
    }
    return db;
}

} // namespace tiny_sql
