#include "tiny_sql/command/command_handler.h"
#include "tiny_sql/protocol/response.h"
#include "tiny_sql/protocol/handshake.h"
#include "tiny_sql/common/logger.h"
#include "tiny_sql/sql/parser.h"
#include "tiny_sql/storage/storage_engine.h"
#include "tiny_sql/storage/expression_evaluator.h"
#include <algorithm>
#include <string>
#include <cctype>

namespace tiny_sql {

// 辅助函数：解析数据类型
static DataType parseDataType(const std::string& type_str) {
    std::string type_upper;
    type_upper.reserve(type_str.size());
    for (char c : type_str) {
        type_upper.push_back(std::toupper(static_cast<unsigned char>(c)));
    }

    if (type_upper == "INT" || type_upper == "INTEGER") return DataType::INT;
    if (type_upper == "BIGINT") return DataType::BIGINT;
    if (type_upper == "FLOAT") return DataType::FLOAT;
    if (type_upper == "DOUBLE") return DataType::DOUBLE;
    if (type_upper.find("VARCHAR") == 0) return DataType::VARCHAR;
    if (type_upper == "TEXT") return DataType::TEXT;
    if (type_upper == "BOOLEAN" || type_upper == "BOOL") return DataType::BOOLEAN;

    // 默认返回VARCHAR
    return DataType::VARCHAR;
}

// 辅助函数：将AST表达式转换为Value
static Value expressionToValue(const Expression* expr, DataType target_type) {
    // 处理标识符（列名）
    if (auto* id = dynamic_cast<const Identifier*>(expr)) {
        // 对于INSERT，不应该有列名引用，这是一个错误
        return Value::Null();
    }

    // 处理数字字面量
    if (auto* num = dynamic_cast<const NumberLiteral*>(expr)) {
        const std::string& val_str = num->getValue();
        try {
            switch (target_type) {
                case DataType::INT:
                    return Value(static_cast<int32_t>(std::stoi(val_str)));
                case DataType::BIGINT:
                    return Value(static_cast<int64_t>(std::stoll(val_str)));
                case DataType::FLOAT:
                    return Value(static_cast<float>(std::stof(val_str)));
                case DataType::DOUBLE:
                    return Value(static_cast<double>(std::stod(val_str)));
                default:
                    return Value(val_str);
            }
        } catch (...) {
            return Value(val_str);
        }
    }

    // 处理字符串字面量
    if (auto* str = dynamic_cast<const StringLiteral*>(expr)) {
        return Value(str->getValue());
    }

    // 默认返回NULL
    return Value::Null();
}

// ==================== PingCommandHandler ====================

bool PingCommandHandler::handleCommand(MySQLCommand command,
                                      Buffer& buffer,
                                      Session& session,
                                      ResponseCallback response_callback) {
    LOG_DEBUG("Handling PING command for session: " << session.getConnectionId());

    // PING命令直接返回OK包
    Buffer response;
    OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0);
    ok_packet.encode(response, session.nextSequenceId());

    response_callback(response);
    return true;
}

// ==================== QuitCommandHandler ====================

bool QuitCommandHandler::handleCommand(MySQLCommand command,
                                      Buffer& buffer,
                                      Session& session,
                                      ResponseCallback response_callback) {
    LOG_INFO("Handling QUIT command for session: " << session.getConnectionId());

    // QUIT命令不需要响应，直接关闭连接
    session.setState(SessionState::CLOSING);
    return true;
}

// ==================== QueryCommandHandler ====================

bool QueryCommandHandler::handleCommand(MySQLCommand command,
                                       Buffer& buffer,
                                       Session& session,
                                       ResponseCallback response_callback) {
    // 读取SQL查询字符串
    std::string query = buffer.readString(buffer.readableBytes());

    LOG_INFO("Query from " << session.getUsername() << ": " << query);

    // 移除查询字符串前后的空白字符
    query.erase(0, query.find_first_not_of(" \t\n\r"));
    query.erase(query.find_last_not_of(" \t\n\r") + 1);

    Buffer response;

    // 使用SQL解析器解析查询
    Parser parser(query);
    auto stmt = parser.parse();

    // 如果解析出错，返回语法错误
    if (parser.hasErrors()) {
        std::string error_msg = "SQL syntax error: ";
        const auto& errors = parser.getErrors();
        if (!errors.empty()) {
            error_msg += errors[0];
        }
        LOG_ERROR(error_msg);
        ErrPacket err_packet(1064, "42000", error_msg);
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 如果解析为空，返回空结果
    if (!stmt) {
        OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0);
        ok_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    LOG_DEBUG("Parsed SQL: " << stmt->toString());

    // 根据语句类型分发执行
    if (auto* select_stmt = dynamic_cast<SelectStatement*>(stmt.get())) {
        return executeSelect(select_stmt, session, response_callback);
    } else if (auto* insert_stmt = dynamic_cast<InsertStatement*>(stmt.get())) {
        return executeInsert(insert_stmt, session, response_callback);
    } else if (auto* create_stmt = dynamic_cast<CreateTableStatement*>(stmt.get())) {
        return executeCreateTable(create_stmt, session, response_callback);
    } else if (auto* drop_stmt = dynamic_cast<DropTableStatement*>(stmt.get())) {
        return executeDropTable(drop_stmt, session, response_callback);
    } else if (auto* show_tables_stmt = dynamic_cast<ShowTablesStatement*>(stmt.get())) {
        return executeShowTables(session, response_callback);
    } else if (auto* show_dbs_stmt = dynamic_cast<ShowDatabasesStatement*>(stmt.get())) {
        return executeShowDatabases(session, response_callback);
    } else if (auto* use_db_stmt = dynamic_cast<UseDatabaseStatement*>(stmt.get())) {
        return executeUseDatabase(use_db_stmt, session, response_callback);
    }

    // 未知的语句类型
    LOG_WARN("Unsupported statement type");
    ErrPacket err_packet(1064, "42000", "Statement not implemented");
    err_packet.encode(response, session.nextSequenceId());
    response_callback(response);
    return true;
}

bool QueryCommandHandler::executeSelect(const SelectStatement* stmt,
                                       Session& session,
                                       ResponseCallback response_callback) {
    LOG_INFO("Executing SELECT: " << stmt->toString());

    Buffer response;

    // 1. 获取当前数据库
    // Get current database
    const std::string& db_name = session.getCurrentDatabase();
    if (db_name.empty()) {
        ErrPacket err_packet(1046, "3D000", "No database selected");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 2. 获取数据库和表
    // Get database and table
    auto& storage = StorageEngine::instance();
    auto db = storage.getDatabase(db_name);
    if (!db) {
        ErrPacket err_packet(1049, "42000", "Unknown database '" + db_name + "'");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    const std::string& table_name = stmt->getTableName();
    auto table = db->getTable(table_name);
    if (!table) {
        ErrPacket err_packet(1146, "42S02",
            "Table '" + db_name + "." + table_name + "' doesn't exist");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 3. 确定要返回的列
    // Determine which columns to return
    std::vector<ColumnDef> result_columns;
    std::vector<size_t> column_indices;

    if (stmt->getColumns().size() == 1) {
        auto* first_col = dynamic_cast<const Identifier*>(stmt->getColumns()[0].get());
        if (first_col && first_col->getName() == "*") {
            // SELECT * - 所有列
            // SELECT * - all columns
            result_columns = table->getColumns();
            for (size_t i = 0; i < result_columns.size(); ++i) {
                column_indices.push_back(i);
            }
        } else {
            // 单个特定列
            // Single specific column
            if (!first_col) {
                ErrPacket err_packet(1064, "42000", "Invalid column expression");
                err_packet.encode(response, session.nextSequenceId());
                response_callback(response);
                return true;
            }

            int col_idx = table->getColumnIndex(first_col->getName());
            if (col_idx < 0) {
                ErrPacket err_packet(1054, "42S22",
                    "Unknown column '" + first_col->getName() + "' in 'field list'");
                err_packet.encode(response, session.nextSequenceId());
                response_callback(response);
                return true;
            }

            result_columns.push_back(table->getColumns()[col_idx]);
            column_indices.push_back(col_idx);
        }
    } else {
        // 多个特定列
        // Multiple specific columns
        for (const auto& col_expr : stmt->getColumns()) {
            auto* id = dynamic_cast<const Identifier*>(col_expr.get());
            if (!id) {
                ErrPacket err_packet(1064, "42000", "Invalid column expression");
                err_packet.encode(response, session.nextSequenceId());
                response_callback(response);
                return true;
            }

            int col_idx = table->getColumnIndex(id->getName());
            if (col_idx < 0) {
                ErrPacket err_packet(1054, "42S22",
                    "Unknown column '" + id->getName() + "' in 'field list'");
                err_packet.encode(response, session.nextSequenceId());
                response_callback(response);
                return true;
            }

            result_columns.push_back(table->getColumns()[col_idx]);
            column_indices.push_back(col_idx);
        }
    }

    // 4. 使用WHERE子句过滤行
    // Filter rows with WHERE clause
    std::vector<Row> filtered_rows;
    const Expression* where_clause = stmt->getWhereClause();

    for (const auto& row : table->getRows()) {
        bool matches = true;

        if (where_clause) {
            try {
                matches = ExpressionEvaluator::evaluate(
                    where_clause, row, table->getColumns()
                );
            } catch (const std::exception& e) {
                ErrPacket err_packet(1064, "42000",
                    "Error evaluating WHERE clause: " + std::string(e.what()));
                err_packet.encode(response, session.nextSequenceId());
                response_callback(response);
                return true;
            }
        }

        if (matches) {
            filtered_rows.push_back(row);
        }
    }

    // 5. 应用LIMIT和OFFSET
    // Apply LIMIT and OFFSET
    size_t offset = stmt->getOffset();
    int limit = stmt->getLimit();

    if (offset >= filtered_rows.size()) {
        filtered_rows.clear();
    } else {
        if (offset > 0) {
            filtered_rows.erase(filtered_rows.begin(),
                              filtered_rows.begin() + offset);
        }

        if (limit >= 0 && static_cast<size_t>(limit) < filtered_rows.size()) {
            filtered_rows.resize(limit);
        }
    }

    LOG_INFO("SELECT result: " << filtered_rows.size() << " rows matched");

    // 6. 发送结果集
    // Send result set

    // 6a. 列数包
    // Column count packet
    Buffer col_count_buffer;
    col_count_buffer.writeLenencInt(result_columns.size());

    // 写入列数包的头部
    // Write column count packet header
    uint32_t col_count_len = static_cast<uint32_t>(col_count_buffer.readableBytes());
    // Payload length (3 bytes, little-endian)
    response.writeUint8(col_count_len & 0xFF);
    response.writeUint8((col_count_len >> 8) & 0xFF);
    response.writeUint8((col_count_len >> 16) & 0xFF);
    response.writeUint8(session.nextSequenceId());  // sequence id
    response.append(col_count_buffer.peek(), col_count_buffer.readableBytes());

    // 6b. 列定义包
    // Column definition packets
    for (const auto& col : result_columns) {
        ColumnDefinitionPacket col_def =
            ColumnDefinitionPacket::fromColumnDef(col, table_name, db_name);
        col_def.encode(response, session.nextSequenceId());
    }

    // 6c. 列定义后的EOF包
    // EOF packet after column definitions
    EofPacket eof1(0, ServerStatus::SERVER_STATUS_AUTOCOMMIT);
    eof1.encode(response, session.nextSequenceId());

    // 6d. 行数据包
    // Row data packets
    for (const auto& row : filtered_rows) {
        // 投影选定的列
        // Project only selected columns
        Row projected_row;
        for (size_t idx : column_indices) {
            projected_row.addValue(row.getValue(idx));
        }

        TextResultRowPacket row_packet(projected_row);
        row_packet.encode(response, session.nextSequenceId());
    }

    // 6e. 最终EOF包
    // Final EOF packet
    EofPacket eof2(0, ServerStatus::SERVER_STATUS_AUTOCOMMIT);
    eof2.encode(response, session.nextSequenceId());

    // 发送响应
    // Send response
    response_callback(response);
    return true;
}

bool QueryCommandHandler::executeInsert(const InsertStatement* stmt,
                                       Session& session,
                                       ResponseCallback response_callback) {
    LOG_INFO("Executing INSERT: " << stmt->toString());

    Buffer response;

    // 获取当前数据库
    const std::string& db_name = session.getCurrentDatabase();
    if (db_name.empty()) {
        ErrPacket err_packet(1046, "3D000", "No database selected");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 获取数据库
    auto& storage = StorageEngine::instance();
    auto db = storage.getDatabase(db_name);
    if (!db) {
        ErrPacket err_packet(1049, "42000", "Unknown database '" + db_name + "'");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 获取表
    const std::string& table_name = stmt->getTableName();
    auto table = db->getTable(table_name);
    if (!table) {
        ErrPacket err_packet(1146, "42S02", "Table '" + db_name + "." + table_name + "' doesn't exist");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 准备行数据
    Row row;
    const auto& columns = table->getColumns();
    const auto& values = stmt->getValues();

    // 如果指定了列名，按列名插入
    if (!stmt->getColumns().empty()) {
        const auto& col_names = stmt->getColumns();

        // 验证列数匹配
        if (col_names.size() != values.size()) {
            ErrPacket err_packet(1136, "21S01", "Column count doesn't match value count");
            err_packet.encode(response, session.nextSequenceId());
            response_callback(response);
            return true;
        }

        // 为每一列准备值（包括未指定的列）
        for (size_t i = 0; i < columns.size(); ++i) {
            const auto& col_def = columns[i];

            // 查找该列在INSERT语句中的位置
            auto it = std::find(col_names.begin(), col_names.end(), col_def.name);

            if (it != col_names.end()) {
                // 找到了，使用提供的值
                size_t idx = std::distance(col_names.begin(), it);
                Value val = expressionToValue(values[idx].get(), col_def.type);
                row.addValue(val);
            } else {
                // 未提供，处理默认值或自增
                if (col_def.auto_increment) {
                    // 自增列
                    row.addValue(Value(static_cast<int32_t>(table->getNextAutoIncrementValue())));
                } else if (!col_def.default_value.isNull()) {
                    // 使用默认值
                    row.addValue(col_def.default_value);
                } else {
                    // 使用NULL
                    row.addValue(Value::Null());
                }
            }
        }
    } else {
        // 没有指定列名，按顺序插入所有列
        if (values.size() != columns.size()) {
            ErrPacket err_packet(1136, "21S01", "Column count doesn't match value count");
            err_packet.encode(response, session.nextSequenceId());
            response_callback(response);
            return true;
        }

        for (size_t i = 0; i < columns.size(); ++i) {
            Value val = expressionToValue(values[i].get(), columns[i].type);
            row.addValue(val);
        }
    }

    // 插入行
    if (!table->insertRow(row)) {
        ErrPacket err_packet(1062, "23000", "Failed to insert row");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    LOG_INFO("Inserted row into table: " << table_name);

    // 返回成功，affected_rows=1
    OkPacket ok_packet(0, 1, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0);
    ok_packet.encode(response, session.nextSequenceId());
    response_callback(response);
    return true;
}

bool QueryCommandHandler::executeCreateTable(const CreateTableStatement* stmt,
                                            Session& session,
                                            ResponseCallback response_callback) {
    LOG_INFO("Executing CREATE TABLE: " << stmt->toString());

    Buffer response;

    // 获取当前数据库
    const std::string& db_name = session.getCurrentDatabase();
    if (db_name.empty()) {
        ErrPacket err_packet(1046, "3D000", "No database selected");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 获取或创建数据库
    auto& storage = StorageEngine::instance();
    auto db = storage.getOrCreateDatabase(db_name);
    if (!db) {
        ErrPacket err_packet(1049, "42000", "Unknown database '" + db_name + "'");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 检查表是否已存在
    const std::string& table_name = stmt->getTableName();
    if (db->hasTable(table_name)) {
        ErrPacket err_packet(1050, "42S01", "Table '" + table_name + "' already exists");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 创建表对象
    auto table = std::make_shared<Table>(table_name);

    // 添加列定义
    for (const auto& ast_col : stmt->getColumns()) {
        ColumnDef col_def;
        col_def.name = ast_col.name;
        col_def.type = parseDataType(ast_col.type);
        col_def.primary_key = ast_col.primary_key;
        col_def.not_null = ast_col.not_null;
        col_def.auto_increment = ast_col.auto_increment;

        if (!ast_col.default_value.empty()) {
            col_def.default_value = Value(ast_col.default_value);
        }

        table->addColumn(col_def);
    }

    // 在数据库中创建表
    if (!db->createTable(table)) {
        ErrPacket err_packet(1050, "42S01", "Failed to create table '" + table_name + "'");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    LOG_INFO("Created table: " << table_name << " in database: " << db_name);

    // 返回成功
    OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0);
    ok_packet.encode(response, session.nextSequenceId());
    response_callback(response);
    return true;
}

bool QueryCommandHandler::executeDropTable(const DropTableStatement* stmt,
                                          Session& session,
                                          ResponseCallback response_callback) {
    LOG_INFO("Executing DROP TABLE: " << stmt->toString());

    Buffer response;

    // 获取当前数据库
    const std::string& db_name = session.getCurrentDatabase();
    if (db_name.empty()) {
        ErrPacket err_packet(1046, "3D000", "No database selected");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 获取数据库
    auto& storage = StorageEngine::instance();
    auto db = storage.getDatabase(db_name);
    if (!db) {
        ErrPacket err_packet(1049, "42000", "Unknown database '" + db_name + "'");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 删除表
    const std::string& table_name = stmt->getTableName();
    if (!db->dropTable(table_name)) {
        ErrPacket err_packet(1051, "42S02", "Unknown table '" + table_name + "'");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    LOG_INFO("Dropped table: " << table_name << " from database: " << db_name);

    // 返回成功
    OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0);
    ok_packet.encode(response, session.nextSequenceId());
    response_callback(response);
    return true;
}

bool QueryCommandHandler::executeShowTables(Session& session,
                                           ResponseCallback response_callback) {
    LOG_INFO("Executing SHOW TABLES");

    Buffer response;

    // 获取当前数据库
    const std::string& db_name = session.getCurrentDatabase();
    if (db_name.empty()) {
        ErrPacket err_packet(1046, "3D000", "No database selected");
        err_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 获取数据库
    auto& storage = StorageEngine::instance();
    auto db = storage.getDatabase(db_name);
    if (!db) {
        // 数据库不存在，返回空结果
        OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0, "No tables in database");
        ok_packet.encode(response, session.nextSequenceId());
        response_callback(response);
        return true;
    }

    // 获取所有表名
    auto table_names = db->getTableNames();

    // 构建结果消息
    std::string result = "Tables in " + db_name + ": ";
    if (table_names.empty()) {
        result += "(none)";
    } else {
        for (size_t i = 0; i < table_names.size(); ++i) {
            if (i > 0) result += ", ";
            result += table_names[i];
        }
    }

    OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0, result);
    ok_packet.encode(response, session.nextSequenceId());
    response_callback(response);
    return true;
}

bool QueryCommandHandler::executeShowDatabases(Session& session,
                                              ResponseCallback response_callback) {
    LOG_INFO("Executing SHOW DATABASES");

    Buffer response;

    // 获取所有数据库
    auto& storage = StorageEngine::instance();
    auto db_names = storage.getDatabaseNames();

    // 构建结果消息
    std::string result = "Databases: ";
    for (size_t i = 0; i < db_names.size(); ++i) {
        if (i > 0) result += ", ";
        result += db_names[i];
    }

    OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0, result);
    ok_packet.encode(response, session.nextSequenceId());
    response_callback(response);
    return true;
}

bool QueryCommandHandler::executeUseDatabase(const UseDatabaseStatement* stmt,
                                            Session& session,
                                            ResponseCallback response_callback) {
    LOG_INFO("Executing USE DATABASE: " << stmt->getDatabaseName());

    // 设置当前数据库
    session.setCurrentDatabase(stmt->getDatabaseName());

    Buffer response;
    std::string result = "Database changed to: " + stmt->getDatabaseName();
    OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0, result);
    ok_packet.encode(response, session.nextSequenceId());
    response_callback(response);
    return true;
}

// ==================== InitDbCommandHandler ====================

bool InitDbCommandHandler::handleCommand(MySQLCommand command,
                                        Buffer& buffer,
                                        Session& session,
                                        ResponseCallback response_callback) {
    // 读取数据库名称
    std::string database = buffer.readString(buffer.readableBytes());

    LOG_INFO("Switching to database: " << database << " for session: "
             << session.getConnectionId());

    // 设置当前数据库
    session.setCurrentDatabase(database);

    // 返回OK包
    Buffer response;
    OkPacket ok_packet(0, 0, ServerStatus::SERVER_STATUS_AUTOCOMMIT, 0);
    ok_packet.encode(response, session.nextSequenceId());

    response_callback(response);
    return true;
}

// ==================== CommandDispatcher ====================

CommandDispatcher::CommandDispatcher()
    : ping_handler_(std::make_unique<PingCommandHandler>())
    , quit_handler_(std::make_unique<QuitCommandHandler>())
    , query_handler_(std::make_unique<QueryCommandHandler>())
    , init_db_handler_(std::make_unique<InitDbCommandHandler>())
{}

bool CommandDispatcher::dispatch(Buffer& buffer,
                                Session& session,
                                CommandHandler::ResponseCallback response_callback) {
    // 读取包头
    if (buffer.readableBytes() < 4) {
        LOG_ERROR("Insufficient data for packet header");
        return false;
    }

    // 读取payload长度（3字节，小端序）
    uint32_t payload_length = 0;
    payload_length |= buffer.readUint8();
    payload_length |= (static_cast<uint32_t>(buffer.readUint8()) << 8);
    payload_length |= (static_cast<uint32_t>(buffer.readUint8()) << 16);

    // 读取序列号
    uint8_t sequence_id = buffer.readUint8();
    session.setSequenceId(sequence_id);

    // 检查是否有完整的payload
    if (buffer.readableBytes() < payload_length) {
        LOG_ERROR("Incomplete packet payload");
        return false;
    }

    // 读取命令类型
    if (payload_length < 1) {
        LOG_ERROR("Empty command packet");
        return false;
    }

    uint8_t cmd_byte = buffer.readUint8();
    MySQLCommand command = static_cast<MySQLCommand>(cmd_byte);

    LOG_DEBUG("Dispatching command: " << static_cast<int>(cmd_byte)
              << " for session: " << session.getConnectionId());

    // 分发到对应的处理器
    switch (command) {
        case MySQLCommand::COM_PING:
            return ping_handler_->handleCommand(command, buffer, session, response_callback);

        case MySQLCommand::COM_QUIT:
            return quit_handler_->handleCommand(command, buffer, session, response_callback);

        case MySQLCommand::COM_QUERY:
            return query_handler_->handleCommand(command, buffer, session, response_callback);

        case MySQLCommand::COM_INIT_DB:
            return init_db_handler_->handleCommand(command, buffer, session, response_callback);

        default:
            LOG_WARN("Unsupported command: " << static_cast<int>(cmd_byte));
            Buffer response;
            ErrPacket err_packet(1047, "08S01", "Unknown command");
            err_packet.encode(response, session.nextSequenceId());
            response_callback(response);
            return false;
    }
}

} // namespace tiny_sql
