#pragma once

#include "tiny_sql/common/buffer.h"
#include "tiny_sql/session/session.h"
#include "tiny_sql/common/types.h"
#include <memory>
#include <functional>

namespace tiny_sql {

// 前向声明
class TcpConnection;
class SelectStatement;
class InsertStatement;
class CreateTableStatement;
class DropTableStatement;
class ShowTablesStatement;
class ShowDatabasesStatement;
class UseDatabaseStatement;

/**
 * 命令处理器基类
 */
class CommandHandler {
public:
    using ResponseCallback = std::function<void(Buffer&)>;

    virtual ~CommandHandler() = default;

    /**
     * 处理MySQL命令
     * @param command 命令类型
     * @param buffer 命令数据（不包含命令字节）
     * @param session 会话对象
     * @param response_callback 响应回调（用于发送响应包）
     * @return 是否成功处理命令
     */
    virtual bool handleCommand(MySQLCommand command,
                              Buffer& buffer,
                              Session& session,
                              ResponseCallback response_callback) = 0;
};

/**
 * PING命令处理器
 */
class PingCommandHandler : public CommandHandler {
public:
    bool handleCommand(MySQLCommand command,
                      Buffer& buffer,
                      Session& session,
                      ResponseCallback response_callback) override;
};

/**
 * QUIT命令处理器
 */
class QuitCommandHandler : public CommandHandler {
public:
    bool handleCommand(MySQLCommand command,
                      Buffer& buffer,
                      Session& session,
                      ResponseCallback response_callback) override;
};

/**
 * QUERY命令处理器
 */
class QueryCommandHandler : public CommandHandler {
public:
    bool handleCommand(MySQLCommand command,
                      Buffer& buffer,
                      Session& session,
                      ResponseCallback response_callback) override;

private:
    // SQL执行方法
    bool executeSelect(const SelectStatement* stmt,
                      Session& session,
                      ResponseCallback response_callback);

    bool executeInsert(const InsertStatement* stmt,
                      Session& session,
                      ResponseCallback response_callback);

    bool executeCreateTable(const CreateTableStatement* stmt,
                           Session& session,
                           ResponseCallback response_callback);

    bool executeDropTable(const DropTableStatement* stmt,
                         Session& session,
                         ResponseCallback response_callback);

    bool executeShowTables(Session& session,
                          ResponseCallback response_callback);

    bool executeShowDatabases(Session& session,
                             ResponseCallback response_callback);

    bool executeUseDatabase(const UseDatabaseStatement* stmt,
                           Session& session,
                           ResponseCallback response_callback);
};

/**
 * INIT_DB命令处理器（USE database）
 */
class InitDbCommandHandler : public CommandHandler {
public:
    bool handleCommand(MySQLCommand command,
                      Buffer& buffer,
                      Session& session,
                      ResponseCallback response_callback) override;
};

/**
 * 命令分发器
 * 根据命令类型分发到对应的处理器
 */
class CommandDispatcher {
public:
    CommandDispatcher();
    ~CommandDispatcher() = default;

    /**
     * 分发命令到对应的处理器
     * @param buffer 包含完整MySQL包的缓冲区
     * @param session 会话对象
     * @param response_callback 响应回调
     * @return 是否成功处理
     */
    bool dispatch(Buffer& buffer,
                 Session& session,
                 CommandHandler::ResponseCallback response_callback);

private:
    std::unique_ptr<PingCommandHandler> ping_handler_;
    std::unique_ptr<QuitCommandHandler> quit_handler_;
    std::unique_ptr<QueryCommandHandler> query_handler_;
    std::unique_ptr<InitDbCommandHandler> init_db_handler_;
};

} // namespace tiny_sql
