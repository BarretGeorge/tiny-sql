#pragma once

#include "tiny_sql/sql/lexer.h"
#include "tiny_sql/sql/ast.h"
#include <memory>
#include <vector>
#include <string>

namespace tiny_sql {

/**
 * SQL 语法分析器
 */
class Parser {
public:
    explicit Parser(const std::string& input);

    /**
     * 解析SQL语句
     */
    std::unique_ptr<Statement> parse();

    /**
     * 获取解析错误信息
     */
    const std::vector<std::string>& getErrors() const { return errors_; }

    /**
     * 是否有错误
     */
    bool hasErrors() const { return !errors_.empty(); }

private:
    /**
     * 解析 SELECT 语句
     */
    std::unique_ptr<SelectStatement> parseSelectStatement();

    /**
     * 解析 INSERT 语句
     */
    std::unique_ptr<InsertStatement> parseInsertStatement();

    /**
     * 解析 CREATE TABLE 语句
     */
    std::unique_ptr<CreateTableStatement> parseCreateTableStatement();

    /**
     * 解析 DROP TABLE 语句
     */
    std::unique_ptr<DropTableStatement> parseDropTableStatement();

    /**
     * 解析 SHOW 语句
     */
    std::unique_ptr<Statement> parseShowStatement();

    /**
     * 解析 USE 语句
     */
    std::unique_ptr<UseDatabaseStatement> parseUseStatement();

    /**
     * 解析表达式
     */
    std::unique_ptr<Expression> parseExpression();

    /**
     * 解析主表达式
     */
    std::unique_ptr<Expression> parsePrimaryExpression();

    /**
     * 解析二元表达式
     */
    std::unique_ptr<Expression> parseBinaryExpression(int precedence, std::unique_ptr<Expression> left);

    /**
     * 获取操作符优先级
     */
    int getPrecedence(TokenType type);

    /**
     * 当前 token
     */
    Token& currentToken() { return current_token_; }

    /**
     * 预览下一个 token
     */
    Token& peekToken() { return peek_token_; }

    /**
     * 移动到下一个 token
     */
    void nextToken();

    /**
     * 期望当前 token 为指定类型
     */
    bool expect(TokenType type);

    /**
     * 期望当前 token 为指定类型，并移动到下一个
     */
    bool expectAndNext(TokenType type);

    /**
     * 添加错误信息
     */
    void addError(const std::string& message);

    Lexer lexer_;
    Token current_token_;
    Token peek_token_;
    std::vector<std::string> errors_;
};

} // namespace tiny_sql
