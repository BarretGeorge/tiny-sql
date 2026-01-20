#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tiny_sql {

/**
 * AST 节点基类
 */
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

/**
 * 表达式基类
 */
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

/**
 * 标识符表达式
 */
class Identifier : public Expression {
public:
    explicit Identifier(const std::string& name) : name_(name) {}

    std::string toString() const override { return name_; }
    const std::string& getName() const { return name_; }

private:
    std::string name_;
};

/**
 * 数字字面量
 */
class NumberLiteral : public Expression {
public:
    explicit NumberLiteral(const std::string& value) : value_(value) {}

    std::string toString() const override { return value_; }
    const std::string& getValue() const { return value_; }

private:
    std::string value_;
};

/**
 * 字符串字面量
 */
class StringLiteral : public Expression {
public:
    explicit StringLiteral(const std::string& value) : value_(value) {}

    std::string toString() const override { return "'" + value_ + "'"; }
    const std::string& getValue() const { return value_; }

private:
    std::string value_;
};

/**
 * 二元表达式
 */
class BinaryExpression : public Expression {
public:
    BinaryExpression(std::unique_ptr<Expression> left,
                    const std::string& op,
                    std::unique_ptr<Expression> right)
        : left_(std::move(left))
        , operator_(op)
        , right_(std::move(right)) {}

    std::string toString() const override {
        return "(" + left_->toString() + " " + operator_ + " " + right_->toString() + ")";
    }

    const Expression* getLeft() const { return left_.get(); }
    const Expression* getRight() const { return right_.get(); }
    const std::string& getOperator() const { return operator_; }

private:
    std::unique_ptr<Expression> left_;
    std::string operator_;
    std::unique_ptr<Expression> right_;
};

/**
 * SQL 语句基类
 */
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

/**
 * SELECT 语句
 */
class SelectStatement : public Statement {
public:
    SelectStatement() = default;

    void addColumn(std::unique_ptr<Expression> column) {
        columns_.push_back(std::move(column));
    }

    void setTableName(const std::string& table) {
        table_name_ = table;
    }

    void setWhereClause(std::unique_ptr<Expression> where) {
        where_clause_ = std::move(where);
    }

    void setLimit(int limit) { limit_ = limit; }
    void setOffset(int offset) { offset_ = offset; }

    std::string toString() const override;

    const std::vector<std::unique_ptr<Expression>>& getColumns() const { return columns_; }
    const std::string& getTableName() const { return table_name_; }
    const Expression* getWhereClause() const { return where_clause_.get(); }
    int getLimit() const { return limit_; }
    int getOffset() const { return offset_; }

private:
    std::vector<std::unique_ptr<Expression>> columns_;
    std::string table_name_;
    std::unique_ptr<Expression> where_clause_;
    int limit_ = -1;
    int offset_ = 0;
};

/**
 * INSERT 语句
 */
class InsertStatement : public Statement {
public:
    InsertStatement() = default;

    void setTableName(const std::string& table) { table_name_ = table; }

    void addColumn(const std::string& column) {
        columns_.push_back(column);
    }

    void addValue(std::unique_ptr<Expression> value) {
        values_.push_back(std::move(value));
    }

    std::string toString() const override;

    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::string>& getColumns() const { return columns_; }
    const std::vector<std::unique_ptr<Expression>>& getValues() const { return values_; }

private:
    std::string table_name_;
    std::vector<std::string> columns_;
    std::vector<std::unique_ptr<Expression>> values_;
};

/**
 * CREATE TABLE 语句
 */
class CreateTableStatement : public Statement {
public:
    struct ColumnDefinition {
        std::string name;
        std::string type;
        bool primary_key = false;
        bool not_null = false;
        bool auto_increment = false;
        std::string default_value;
    };

    CreateTableStatement() = default;

    void setTableName(const std::string& table) { table_name_ = table; }

    void addColumn(const ColumnDefinition& column) {
        columns_.push_back(column);
    }

    std::string toString() const override;

    const std::string& getTableName() const { return table_name_; }
    const std::vector<ColumnDefinition>& getColumns() const { return columns_; }

private:
    std::string table_name_;
    std::vector<ColumnDefinition> columns_;
};

/**
 * DROP TABLE 语句
 */
class DropTableStatement : public Statement {
public:
    explicit DropTableStatement(const std::string& table) : table_name_(table) {}

    std::string toString() const override {
        return "DROP TABLE " + table_name_;
    }

    const std::string& getTableName() const { return table_name_; }

private:
    std::string table_name_;
};

/**
 * SHOW TABLES 语句
 */
class ShowTablesStatement : public Statement {
public:
    ShowTablesStatement() = default;

    std::string toString() const override {
        return "SHOW TABLES";
    }
};

/**
 * SHOW DATABASES 语句
 */
class ShowDatabasesStatement : public Statement {
public:
    ShowDatabasesStatement() = default;

    std::string toString() const override {
        return "SHOW DATABASES";
    }
};

/**
 * USE DATABASE 语句
 */
class UseDatabaseStatement : public Statement {
public:
    explicit UseDatabaseStatement(const std::string& db) : database_name_(db) {}

    std::string toString() const override {
        return "USE " + database_name_;
    }

    const std::string& getDatabaseName() const { return database_name_; }

private:
    std::string database_name_;
};

} // namespace tiny_sql
