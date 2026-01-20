#include "tiny_sql/sql/token.h"
#include <algorithm>
#include <unordered_map>

namespace tiny_sql {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::ILLEGAL: return "ILLEGAL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::SELECT: return "SELECT";
        case TokenType::FROM: return "FROM";
        case TokenType::WHERE: return "WHERE";
        case TokenType::INSERT: return "INSERT";
        case TokenType::INTO: return "INTO";
        case TokenType::VALUES: return "VALUES";
        case TokenType::UPDATE: return "UPDATE";
        case TokenType::DELETE: return "DELETE";
        case TokenType::CREATE: return "CREATE";
        case TokenType::TABLE: return "TABLE";
        case TokenType::DROP: return "DROP";
        case TokenType::DATABASE: return "DATABASE";
        case TokenType::USE: return "USE";
        case TokenType::SHOW: return "SHOW";
        case TokenType::TABLES: return "TABLES";
        case TokenType::DATABASES: return "DATABASES";
        case TokenType::INT: return "INT";
        case TokenType::VARCHAR: return "VARCHAR";
        case TokenType::ASTERISK: return "*";
        case TokenType::COMMA: return ",";
        case TokenType::SEMICOLON: return ";";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::EQ: return "=";
        default: return "UNKNOWN";
    }
}

TokenType lookupKeyword(const std::string& identifier) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"SELECT", TokenType::SELECT},
        {"FROM", TokenType::FROM},
        {"WHERE", TokenType::WHERE},
        {"INSERT", TokenType::INSERT},
        {"INTO", TokenType::INTO},
        {"VALUES", TokenType::VALUES},
        {"UPDATE", TokenType::UPDATE},
        {"DELETE", TokenType::DELETE},
        {"CREATE", TokenType::CREATE},
        {"TABLE", TokenType::TABLE},
        {"DROP", TokenType::DROP},
        {"ALTER", TokenType::ALTER},
        {"INDEX", TokenType::INDEX},
        {"DATABASE", TokenType::DATABASE},
        {"USE", TokenType::USE},
        {"SHOW", TokenType::SHOW},
        {"TABLES", TokenType::TABLES},
        {"DATABASES", TokenType::DATABASES},
        {"DESCRIBE", TokenType::DESCRIBE},
        {"DESC", TokenType::DESC},

        // 数据类型
        {"INT", TokenType::INT},
        {"INTEGER", TokenType::INTEGER},
        {"VARCHAR", TokenType::VARCHAR},
        {"CHAR", TokenType::CHAR},
        {"TEXT", TokenType::TEXT},
        {"FLOAT", TokenType::FLOAT},
        {"DOUBLE", TokenType::DOUBLE},
        {"DECIMAL", TokenType::DECIMAL},
        {"DATE", TokenType::DATE},
        {"DATETIME", TokenType::DATETIME},
        {"TIMESTAMP", TokenType::TIMESTAMP},
        {"BOOLEAN", TokenType::BOOLEAN},
        {"BOOL", TokenType::BOOL},

        // 约束
        {"PRIMARY", TokenType::PRIMARY},
        {"KEY", TokenType::KEY},
        {"FOREIGN", TokenType::FOREIGN},
        {"UNIQUE", TokenType::UNIQUE},
        {"NOT", TokenType::NOT},
        {"NULL", TokenType::NULL_TOKEN},
        {"DEFAULT", TokenType::DEFAULT},
        {"AUTO_INCREMENT", TokenType::AUTO_INCREMENT},

        // 逻辑操作符
        {"AND", TokenType::AND},
        {"OR", TokenType::OR},

        // 其他关键字
        {"AS", TokenType::AS},
        {"LIMIT", TokenType::LIMIT},
        {"OFFSET", TokenType::OFFSET},
        {"ORDER", TokenType::ORDER},
        {"BY", TokenType::BY},
        {"GROUP", TokenType::GROUP},
        {"HAVING", TokenType::HAVING},
        {"JOIN", TokenType::JOIN},
        {"LEFT", TokenType::LEFT},
        {"RIGHT", TokenType::RIGHT},
        {"INNER", TokenType::INNER},
        {"OUTER", TokenType::OUTER},
        {"ON", TokenType::ON},
        {"DISTINCT", TokenType::DISTINCT},
        {"ALL", TokenType::ALL},
        {"COUNT", TokenType::COUNT},
        {"SUM", TokenType::SUM},
        {"AVG", TokenType::AVG},
        {"MAX", TokenType::MAX},
        {"MIN", TokenType::MIN},
        {"IN", TokenType::IN},
        {"BETWEEN", TokenType::BETWEEN},
        {"LIKE", TokenType::LIKE},
        {"IS", TokenType::IS},
        {"ASC", TokenType::ASC},
    };

    // 转换为大写进行查找
    std::string upper = identifier;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    auto it = keywords.find(upper);
    if (it != keywords.end()) {
        return it->second;
    }

    return TokenType::IDENTIFIER;
}

} // namespace tiny_sql
