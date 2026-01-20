#include "tiny_sql/sql/parser.h"
#include "tiny_sql/common/logger.h"

namespace tiny_sql {

Parser::Parser(const std::string& input)
    : lexer_(input)
{
    // 读取两个token来初始化current和peek
    nextToken();
    nextToken();
}

std::unique_ptr<Statement> Parser::parse() {
    errors_.clear();

    // 跳过前导的分号
    while (currentToken().type == TokenType::SEMICOLON) {
        nextToken();
    }

    if (currentToken().type == TokenType::EOF_TOKEN) {
        return nullptr;
    }

    switch (currentToken().type) {
        case TokenType::SELECT:
            return parseSelectStatement();

        case TokenType::INSERT:
            return parseInsertStatement();

        case TokenType::CREATE:
            return parseCreateTableStatement();

        case TokenType::DROP:
            return parseDropTableStatement();

        case TokenType::SHOW:
            return parseShowStatement();

        case TokenType::USE:
            return parseUseStatement();

        default:
            addError("Unexpected token: " + currentToken().literal);
            return nullptr;
    }
}

std::unique_ptr<SelectStatement> Parser::parseSelectStatement() {
    auto stmt = std::make_unique<SelectStatement>();

    if (!expectAndNext(TokenType::SELECT)) {
        return nullptr;
    }

    // 解析列
    if (currentToken().type == TokenType::ASTERISK) {
        stmt->addColumn(std::make_unique<Identifier>("*"));
        nextToken();
    } else {
        while (true) {
            auto expr = parseExpression();
            if (!expr) {
                return nullptr;
            }
            stmt->addColumn(std::move(expr));

            if (currentToken().type != TokenType::COMMA) {
                break;
            }
            nextToken(); // skip comma
        }
    }

    // FROM 子句
    if (currentToken().type == TokenType::FROM) {
        nextToken();
        if (currentToken().type != TokenType::IDENTIFIER) {
            addError("Expected table name after FROM");
            return nullptr;
        }
        stmt->setTableName(currentToken().literal);
        nextToken();
    }

    // WHERE 子句
    if (currentToken().type == TokenType::WHERE) {
        nextToken();
        auto where = parseExpression();
        if (!where) {
            return nullptr;
        }
        stmt->setWhereClause(std::move(where));
    }

    // LIMIT 子句
    if (currentToken().type == TokenType::LIMIT) {
        nextToken();
        if (currentToken().type != TokenType::NUMBER) {
            addError("Expected number after LIMIT");
            return nullptr;
        }
        stmt->setLimit(std::stoi(currentToken().literal));
        nextToken();
    }

    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsertStatement() {
    auto stmt = std::make_unique<InsertStatement>();

    if (!expectAndNext(TokenType::INSERT)) {
        return nullptr;
    }

    if (!expectAndNext(TokenType::INTO)) {
        return nullptr;
    }

    if (currentToken().type != TokenType::IDENTIFIER) {
        addError("Expected table name after INTO");
        return nullptr;
    }

    stmt->setTableName(currentToken().literal);
    nextToken();

    // 解析列名（可选）
    if (currentToken().type == TokenType::LPAREN) {
        nextToken();

        while (true) {
            if (currentToken().type != TokenType::IDENTIFIER) {
                addError("Expected column name");
                return nullptr;
            }
            stmt->addColumn(currentToken().literal);
            nextToken();

            if (currentToken().type != TokenType::COMMA) {
                break;
            }
            nextToken();
        }

        if (!expectAndNext(TokenType::RPAREN)) {
            return nullptr;
        }
    }

    // VALUES
    if (!expectAndNext(TokenType::VALUES)) {
        return nullptr;
    }

    if (!expectAndNext(TokenType::LPAREN)) {
        return nullptr;
    }

    // 解析值
    while (true) {
        auto expr = parseExpression();
        if (!expr) {
            return nullptr;
        }
        stmt->addValue(std::move(expr));

        if (currentToken().type != TokenType::COMMA) {
            break;
        }
        nextToken();
    }

    if (!expectAndNext(TokenType::RPAREN)) {
        return nullptr;
    }

    return stmt;
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTableStatement() {
    auto stmt = std::make_unique<CreateTableStatement>();

    if (!expectAndNext(TokenType::CREATE)) {
        return nullptr;
    }

    if (!expectAndNext(TokenType::TABLE)) {
        return nullptr;
    }

    if (currentToken().type != TokenType::IDENTIFIER) {
        addError("Expected table name");
        return nullptr;
    }

    stmt->setTableName(currentToken().literal);
    nextToken();

    if (!expectAndNext(TokenType::LPAREN)) {
        return nullptr;
    }

    // 解析列定义
    while (currentToken().type != TokenType::RPAREN) {
        CreateTableStatement::ColumnDefinition col;

        // 列名
        if (currentToken().type != TokenType::IDENTIFIER) {
            addError("Expected column name");
            return nullptr;
        }
        col.name = currentToken().literal;
        nextToken();

        // 数据类型
        col.type = currentToken().literal;
        nextToken();

        // 解析约束
        while (true) {
            if (currentToken().type == TokenType::PRIMARY) {
                nextToken();
                if (currentToken().type == TokenType::KEY) {
                    col.primary_key = true;
                    nextToken();
                }
            } else if (currentToken().type == TokenType::NOT) {
                nextToken();
                if (currentToken().type == TokenType::NULL_TOKEN) {
                    col.not_null = true;
                    nextToken();
                }
            } else if (currentToken().type == TokenType::AUTO_INCREMENT) {
                col.auto_increment = true;
                nextToken();
            } else if (currentToken().type == TokenType::DEFAULT) {
                nextToken();
                col.default_value = currentToken().literal;
                nextToken();
            } else {
                break;
            }
        }

        stmt->addColumn(col);

        if (currentToken().type == TokenType::COMMA) {
            nextToken();
        } else {
            break;
        }
    }

    if (!expectAndNext(TokenType::RPAREN)) {
        return nullptr;
    }

    return stmt;
}

std::unique_ptr<DropTableStatement> Parser::parseDropTableStatement() {
    if (!expectAndNext(TokenType::DROP)) {
        return nullptr;
    }

    if (!expectAndNext(TokenType::TABLE)) {
        return nullptr;
    }

    if (currentToken().type != TokenType::IDENTIFIER) {
        addError("Expected table name");
        return nullptr;
    }

    auto stmt = std::make_unique<DropTableStatement>(currentToken().literal);
    nextToken();

    return stmt;
}

std::unique_ptr<Statement> Parser::parseShowStatement() {
    if (!expectAndNext(TokenType::SHOW)) {
        return nullptr;
    }

    if (currentToken().type == TokenType::TABLES) {
        nextToken();
        return std::make_unique<ShowTablesStatement>();
    } else if (currentToken().type == TokenType::DATABASES) {
        nextToken();
        return std::make_unique<ShowDatabasesStatement>();
    }

    addError("Unexpected token after SHOW");
    return nullptr;
}

std::unique_ptr<UseDatabaseStatement> Parser::parseUseStatement() {
    if (!expectAndNext(TokenType::USE)) {
        return nullptr;
    }

    if (currentToken().type != TokenType::IDENTIFIER) {
        addError("Expected database name");
        return nullptr;
    }

    auto stmt = std::make_unique<UseDatabaseStatement>(currentToken().literal);
    nextToken();

    return stmt;
}

std::unique_ptr<Expression> Parser::parseExpression() {
    auto left = parsePrimaryExpression();
    if (!left) {
        return nullptr;
    }

    return parseBinaryExpression(0, std::move(left));
}

std::unique_ptr<Expression> Parser::parsePrimaryExpression() {
    switch (currentToken().type) {
        case TokenType::IDENTIFIER: {
            auto expr = std::make_unique<Identifier>(currentToken().literal);
            nextToken();
            return expr;
        }

        case TokenType::NUMBER: {
            auto expr = std::make_unique<NumberLiteral>(currentToken().literal);
            nextToken();
            return expr;
        }

        case TokenType::STRING: {
            auto expr = std::make_unique<StringLiteral>(currentToken().literal);
            nextToken();
            return expr;
        }

        case TokenType::ASTERISK: {
            auto expr = std::make_unique<Identifier>("*");
            nextToken();
            return expr;
        }

        case TokenType::LPAREN: {
            nextToken();
            auto expr = parseExpression();
            if (!expectAndNext(TokenType::RPAREN)) {
                return nullptr;
            }
            return expr;
        }

        default:
            addError("Unexpected token in expression: " + currentToken().literal);
            return nullptr;
    }
}

std::unique_ptr<Expression> Parser::parseBinaryExpression(int precedence, std::unique_ptr<Expression> left) {
    while (true) {
        int current_precedence = getPrecedence(currentToken().type);
        if (current_precedence < precedence) {
            return left;
        }

        std::string op = currentToken().literal;
        TokenType op_type = currentToken().type;
        nextToken();

        auto right = parsePrimaryExpression();
        if (!right) {
            return nullptr;
        }

        int next_precedence = getPrecedence(currentToken().type);
        if (current_precedence < next_precedence) {
            right = parseBinaryExpression(current_precedence + 1, std::move(right));
            if (!right) {
                return nullptr;
            }
        }

        left = std::make_unique<BinaryExpression>(std::move(left), op, std::move(right));
    }
}

int Parser::getPrecedence(TokenType type) {
    switch (type) {
        case TokenType::OR:
            return 1;
        case TokenType::AND:
            return 2;
        case TokenType::EQ:
        case TokenType::NE:
        case TokenType::LT:
        case TokenType::LE:
        case TokenType::GT:
        case TokenType::GE:
            return 3;
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 4;
        case TokenType::ASTERISK:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            return 5;
        default:
            return 0;
    }
}

void Parser::nextToken() {
    current_token_ = peek_token_;
    peek_token_ = lexer_.nextToken();
}

bool Parser::expect(TokenType type) {
    if (currentToken().type != type) {
        addError("Expected " + std::string(tokenTypeToString(type)) +
                ", got " + currentToken().literal);
        return false;
    }
    return true;
}

bool Parser::expectAndNext(TokenType type) {
    if (!expect(type)) {
        return false;
    }
    nextToken();
    return true;
}

void Parser::addError(const std::string& message) {
    std::string error = "Parse error at line " + std::to_string(currentToken().line) +
                       ", column " + std::to_string(currentToken().column) +
                       ": " + message;
    errors_.push_back(error);
    LOG_ERROR(error);
}

} // namespace tiny_sql
