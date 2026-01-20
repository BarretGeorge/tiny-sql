#pragma once

#include <string>
#include <cstdint>

namespace tiny_sql {

/**
 * SQL Token 类型
 */
enum class TokenType {
    // 特殊符号
    EOF_TOKEN,
    ILLEGAL,

    // 标识符和字面量
    IDENTIFIER,     // user_name, table_name
    NUMBER,         // 123, 45.67
    STRING,         // 'hello', "world"

    // 关键字
    SELECT,
    FROM,
    WHERE,
    INSERT,
    INTO,
    VALUES,
    UPDATE,
    DELETE,
    CREATE,
    TABLE,
    DROP,
    ALTER,
    INDEX,
    DATABASE,
    USE,
    SHOW,
    TABLES,
    DATABASES,
    DESCRIBE,
    DESC,

    // 数据类型
    INT,
    INTEGER,
    VARCHAR,
    CHAR,
    TEXT,
    FLOAT,
    DOUBLE,
    DECIMAL,
    DATE,
    DATETIME,
    TIMESTAMP,
    BOOLEAN,
    BOOL,

    // 约束
    PRIMARY,
    KEY,
    FOREIGN,
    UNIQUE,
    NOT,
    NULL_TOKEN,
    DEFAULT,
    AUTO_INCREMENT,

    // 操作符
    PLUS,           // +
    MINUS,          // -
    ASTERISK,       // *
    SLASH,          // /
    PERCENT,        // %

    EQ,             // =
    NE,             // != or <>
    LT,             // <
    LE,             // <=
    GT,             // >
    GE,             // >=

    // 逻辑操作符
    AND,
    OR,

    // 分隔符
    COMMA,          // ,
    SEMICOLON,      // ;
    DOT,            // .
    LPAREN,         // (
    RPAREN,         // )

    // 其他关键字
    AS,
    LIMIT,
    OFFSET,
    ORDER,
    BY,
    GROUP,
    HAVING,
    JOIN,
    LEFT,
    RIGHT,
    INNER,
    OUTER,
    ON,
    DISTINCT,
    ALL,
    COUNT,
    SUM,
    AVG,
    MAX,
    MIN,
    IN,
    BETWEEN,
    LIKE,
    IS,
    ASC,
    ASCENDING,
    DESCENDING,
};

/**
 * Token 结构
 */
struct Token {
    TokenType type;
    std::string literal;
    size_t line;
    size_t column;

    Token() : type(TokenType::ILLEGAL), line(0), column(0) {}

    Token(TokenType t, const std::string& lit, size_t l = 0, size_t c = 0)
        : type(t), literal(lit), line(l), column(c) {}

    bool operator==(const Token& other) const {
        return type == other.type && literal == other.literal;
    }

    bool operator!=(const Token& other) const {
        return !(*this == other);
    }
};

/**
 * 将 TokenType 转换为字符串
 */
const char* tokenTypeToString(TokenType type);

/**
 * 检查字符串是否为关键字，如果是则返回对应的 TokenType
 */
TokenType lookupKeyword(const std::string& identifier);

} // namespace tiny_sql
