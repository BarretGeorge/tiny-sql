#include "tiny_sql/sql/lexer.h"
#include "tiny_sql/common/logger.h"
#include <cctype>

namespace tiny_sql {

Lexer::Lexer(const std::string& input)
    : input_(input)
    , position_(0)
    , read_position_(0)
    , ch_(0)
    , line_(1)
    , column_(0)
{
    readChar(); // 读取第一个字符
}

Token Lexer::nextToken() {
    skipWhitespace();
    skipComment();

    Token token;
    token.line = line_;
    token.column = column_;

    switch (ch_) {
        case '\0':
            token.type = TokenType::EOF_TOKEN;
            token.literal = "";
            break;

        case '+':
            token.type = TokenType::PLUS;
            token.literal = ch_;
            readChar();
            break;

        case '-':
            // 可能是注释 --
            if (peekChar() == '-') {
                skipComment();
                return nextToken();
            }
            token.type = TokenType::MINUS;
            token.literal = ch_;
            readChar();
            break;

        case '*':
            token.type = TokenType::ASTERISK;
            token.literal = ch_;
            readChar();
            break;

        case '/':
            token.type = TokenType::SLASH;
            token.literal = ch_;
            readChar();
            break;

        case '%':
            token.type = TokenType::PERCENT;
            token.literal = ch_;
            readChar();
            break;

        case '=':
            token.type = TokenType::EQ;
            token.literal = ch_;
            readChar();
            break;

        case '!':
            if (peekChar() == '=') {
                char first = ch_;
                readChar();
                token.type = TokenType::NE;
                token.literal = std::string(1, first) + ch_;
                readChar();
            } else {
                token.type = TokenType::ILLEGAL;
                token.literal = ch_;
                readChar();
            }
            break;

        case '<':
            if (peekChar() == '=') {
                char first = ch_;
                readChar();
                token.type = TokenType::LE;
                token.literal = std::string(1, first) + ch_;
                readChar();
            } else if (peekChar() == '>') {
                char first = ch_;
                readChar();
                token.type = TokenType::NE;
                token.literal = std::string(1, first) + ch_;
                readChar();
            } else {
                token.type = TokenType::LT;
                token.literal = ch_;
                readChar();
            }
            break;

        case '>':
            if (peekChar() == '=') {
                char first = ch_;
                readChar();
                token.type = TokenType::GE;
                token.literal = std::string(1, first) + ch_;
                readChar();
            } else {
                token.type = TokenType::GT;
                token.literal = ch_;
                readChar();
            }
            break;

        case ',':
            token.type = TokenType::COMMA;
            token.literal = ch_;
            readChar();
            break;

        case ';':
            token.type = TokenType::SEMICOLON;
            token.literal = ch_;
            readChar();
            break;

        case '.':
            token.type = TokenType::DOT;
            token.literal = ch_;
            readChar();
            break;

        case '(':
            token.type = TokenType::LPAREN;
            token.literal = ch_;
            readChar();
            break;

        case ')':
            token.type = TokenType::RPAREN;
            token.literal = ch_;
            readChar();
            break;

        case '\'':
        case '"':
            token.type = TokenType::STRING;
            token.literal = readString(ch_);
            break;

        default:
            if (isLetter(ch_)) {
                token.literal = readIdentifier();
                token.type = lookupKeyword(token.literal);
                return token; // 不调用 readChar()，因为 readIdentifier() 已经移动了
            } else if (isDigit(ch_)) {
                token.type = TokenType::NUMBER;
                token.literal = readNumber();
                return token; // 不调用 readChar()
            } else {
                token.type = TokenType::ILLEGAL;
                token.literal = ch_;
                readChar();
            }
            break;
    }

    return token;
}

Token Lexer::peekToken() {
    size_t save_position = position_;
    size_t save_read_position = read_position_;
    char save_ch = ch_;
    size_t save_line = line_;
    size_t save_column = column_;

    Token token = nextToken();

    position_ = save_position;
    read_position_ = save_read_position;
    ch_ = save_ch;
    line_ = save_line;
    column_ = save_column;

    return token;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        Token token = nextToken();
        tokens.push_back(token);

        if (token.type == TokenType::EOF_TOKEN) {
            break;
        }
    }

    return tokens;
}

char Lexer::readChar() {
    if (read_position_ >= input_.size()) {
        ch_ = '\0';
    } else {
        ch_ = input_[read_position_];
    }

    position_ = read_position_;
    read_position_++;

    if (ch_ == '\n') {
        line_++;
        column_ = 0;
    } else {
        column_++;
    }

    return ch_;
}

char Lexer::peekChar() const {
    if (read_position_ >= input_.size()) {
        return '\0';
    }
    return input_[read_position_];
}

void Lexer::skipWhitespace() {
    while (isWhitespace(ch_)) {
        readChar();
    }
}

void Lexer::skipComment() {
    // 处理 -- 注释
    if (ch_ == '-' && peekChar() == '-') {
        while (ch_ != '\n' && ch_ != '\0') {
            readChar();
        }
        skipWhitespace();
    }

    // 处理 /* */ 注释
    if (ch_ == '/' && peekChar() == '*') {
        readChar(); // /
        readChar(); // *

        while (true) {
            if (ch_ == '\0') {
                break;
            }
            if (ch_ == '*' && peekChar() == '/') {
                readChar(); // *
                readChar(); // /
                break;
            }
            readChar();
        }
        skipWhitespace();
    }

    // 处理 # 注释（MySQL风格）
    if (ch_ == '#') {
        while (ch_ != '\n' && ch_ != '\0') {
            readChar();
        }
        skipWhitespace();
    }
}

std::string Lexer::readIdentifier() {
    size_t start = position_;

    while (isLetter(ch_) || isDigit(ch_)) {
        readChar();
    }

    return input_.substr(start, position_ - start);
}

std::string Lexer::readNumber() {
    size_t start = position_;
    bool has_dot = false;

    while (isDigit(ch_) || (ch_ == '.' && !has_dot)) {
        if (ch_ == '.') {
            has_dot = true;
        }
        readChar();
    }

    return input_.substr(start, position_ - start);
}

std::string Lexer::readString(char quote) {
    readChar(); // 跳过引号

    std::string result;

    while (ch_ != quote && ch_ != '\0') {
        if (ch_ == '\\') {
            // 处理转义字符
            readChar();
            switch (ch_) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '\'': result += '\''; break;
                case '"': result += '"'; break;
                default: result += ch_; break;
            }
        } else {
            result += ch_;
        }
        readChar();
    }

    readChar(); // 跳过结束引号

    return result;
}

bool Lexer::isLetter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

bool Lexer::isDigit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool Lexer::isWhitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

} // namespace tiny_sql
