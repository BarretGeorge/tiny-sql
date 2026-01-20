#pragma once

#include "tiny_sql/sql/token.h"
#include <string>
#include <vector>

namespace tiny_sql {

/**
 * SQL 词法分析器
 */
class Lexer {
public:
    explicit Lexer(const std::string& input);

    /**
     * 获取下一个 Token
     */
    Token nextToken();

    /**
     * 预览下一个 Token（不移动位置）
     */
    Token peekToken();

    /**
     * 获取当前行号
     */
    size_t getLine() const { return line_; }

    /**
     * 获取当前列号
     */
    size_t getColumn() const { return column_; }

    /**
     * 一次性解析所有 Token
     */
    std::vector<Token> tokenize();

private:
    /**
     * 读取下一个字符
     */
    char readChar();

    /**
     * 预览下一个字符（不移动位置）
     */
    char peekChar() const;

    /**
     * 跳过空白字符
     */
    void skipWhitespace();

    /**
     * 跳过注释
     */
    void skipComment();

    /**
     * 读取标识符
     */
    std::string readIdentifier();

    /**
     * 读取数字
     */
    std::string readNumber();

    /**
     * 读取字符串
     */
    std::string readString(char quote);

    /**
     * 判断字符是否为字母或下划线
     */
    static bool isLetter(char ch);

    /**
     * 判断字符是否为数字
     */
    static bool isDigit(char ch);

    /**
     * 判断字符是否为空白字符
     */
    static bool isWhitespace(char ch);

    std::string input_;
    size_t position_;       // 当前位置
    size_t read_position_;  // 读取位置（下一个字符）
    char ch_;               // 当前字符
    size_t line_;           // 当前行号
    size_t column_;         // 当前列号
};

} // namespace tiny_sql
