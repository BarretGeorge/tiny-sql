#include "tiny_sql/storage/expression_evaluator.h"
#include "tiny_sql/common/logger.h"
#include <sstream>
#include <cmath>

namespace tiny_sql {

bool ExpressionEvaluator::evaluate(const Expression* expr,
                                   const Row& row,
                                   const std::vector<ColumnDef>& columns) {
    // 如果表达式为空（无WHERE子句），返回true（匹配所有行）
    // If expression is null (no WHERE clause), return true (match all rows)
    if (!expr) {
        return true;
    }

    // 尝试将表达式评估为布尔值
    // Try to evaluate expression as boolean
    try {
        // 对于二元表达式，直接评估为布尔
        // For binary expressions, evaluate directly as boolean
        if (const auto* bin_expr = dynamic_cast<const BinaryExpression*>(expr)) {
            return evaluateBinaryExpression(bin_expr, row, columns);
        }

        // 其他类型的表达式，先评估为Value，然后转换为布尔
        // For other expression types, evaluate to Value then convert to boolean
        Value val = evaluateValue(expr, row, columns);

        // 如果是布尔类型，直接返回
        // If boolean type, return directly
        if (val.isBool()) {
            return val.asBool();
        }

        // 如果是NULL，返回false（SQL三值逻辑：NULL在WHERE中视为false）
        // If NULL, return false (SQL three-valued logic: NULL is treated as false in WHERE)
        if (val.isNull()) {
            return false;
        }

        // 其他类型：非零/非空为true
        // Other types: non-zero/non-empty is true
        if (val.isInt()) return val.asInt() != 0;
        if (val.isBigInt()) return val.asBigInt() != 0;
        if (val.isFloat()) return std::fabs(val.asFloat()) > 1e-9;
        if (val.isDouble()) return std::fabs(val.asDouble()) > 1e-9;
        if (val.isString()) return !val.asString().empty();

        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Expression evaluation error: " << e.what());
        throw;
    }
}

Value ExpressionEvaluator::evaluateValue(const Expression* expr,
                                         const Row& row,
                                         const std::vector<ColumnDef>& columns) {
    if (!expr) {
        return Value::Null();
    }

    // 标识符：从行中获取列值
    // Identifier: get column value from row
    if (const auto* id = dynamic_cast<const Identifier*>(expr)) {
        return evaluateIdentifier(id, row, columns);
    }

    // 字面量：转换为Value
    // Literal: convert to Value
    if (const auto* num_lit = dynamic_cast<const NumberLiteral*>(expr)) {
        return evaluateLiteral(num_lit);
    }

    if (const auto* str_lit = dynamic_cast<const StringLiteral*>(expr)) {
        return evaluateLiteral(str_lit);
    }

    // 二元表达式：递归评估
    // Binary expression: evaluate recursively
    if (const auto* bin_expr = dynamic_cast<const BinaryExpression*>(expr)) {
        // 对于逻辑运算符，返回布尔值
        // For logical operators, return boolean value
        const std::string& op = bin_expr->getOperator();
        if (op == "AND" || op == "OR") {
            bool result = evaluateBinaryExpression(bin_expr, row, columns);
            return Value(result);
        }

        // 对于比较运算符，返回布尔值
        // For comparison operators, return boolean value
        if (op == "=" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            bool result = evaluateBinaryExpression(bin_expr, row, columns);
            return Value(result);
        }

        // 未来可以扩展：算术运算符 +, -, *, /
        // Future extension: arithmetic operators +, -, *, /
        throw std::runtime_error("Unsupported operator in expression: " + op);
    }

    throw std::runtime_error("Unsupported expression type");
}

bool ExpressionEvaluator::evaluateBinaryExpression(const BinaryExpression* expr,
                                                   const Row& row,
                                                   const std::vector<ColumnDef>& columns) {
    const std::string& op = expr->getOperator();

    // 逻辑运算符：AND, OR
    // Logical operators: AND, OR
    if (op == "AND") {
        bool left_result = evaluate(expr->getLeft(), row, columns);
        // 短路求值：如果左边为false，不评估右边
        // Short-circuit: if left is false, don't evaluate right
        if (!left_result) {
            return false;
        }
        return evaluate(expr->getRight(), row, columns);
    }

    if (op == "OR") {
        bool left_result = evaluate(expr->getLeft(), row, columns);
        // 短路求值：如果左边为true，不评估右边
        // Short-circuit: if left is true, don't evaluate right
        if (left_result) {
            return true;
        }
        return evaluate(expr->getRight(), row, columns);
    }

    // 比较运算符：=, !=, <, >, <=, >=
    // Comparison operators: =, !=, <, >, <=, >=
    Value left_value = evaluateValue(expr->getLeft(), row, columns);
    Value right_value = evaluateValue(expr->getRight(), row, columns);

    return compareValues(left_value, op, right_value);
}

Value ExpressionEvaluator::evaluateIdentifier(const Identifier* id,
                                              const Row& row,
                                              const std::vector<ColumnDef>& columns) {
    const std::string& col_name = id->getName();

    // 查找列索引
    // Find column index
    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == col_name) {
            // 检查行是否有足够的列
            // Check if row has enough columns
            if (i >= row.getColumnCount()) {
                throw std::runtime_error("Row has insufficient columns for identifier: " + col_name);
            }
            return row.getValue(i);
        }
    }

    throw std::runtime_error("Unknown column in expression: " + col_name);
}

Value ExpressionEvaluator::evaluateLiteral(const Expression* expr) {
    // 数字字面量
    // Number literal
    if (const auto* num_lit = dynamic_cast<const NumberLiteral*>(expr)) {
        const std::string& value_str = num_lit->getValue();

        // 判断是否包含小数点或科学计数法
        // Check if contains decimal point or scientific notation
        if (value_str.find('.') != std::string::npos ||
            value_str.find('e') != std::string::npos ||
            value_str.find('E') != std::string::npos) {
            // 浮点数
            // Floating point
            try {
                double val = std::stod(value_str);
                return Value(val);
            } catch (const std::exception& e) {
                throw std::runtime_error("Invalid floating point literal: " + value_str);
            }
        } else {
            // 整数：根据大小选择int32或int64
            // Integer: choose int32 or int64 based on size
            try {
                long long val = std::stoll(value_str);
                // 如果在int32范围内，使用int32
                // If within int32 range, use int32
                if (val >= INT32_MIN && val <= INT32_MAX) {
                    return Value(static_cast<int32_t>(val));
                } else {
                    return Value(static_cast<int64_t>(val));
                }
            } catch (const std::exception& e) {
                throw std::runtime_error("Invalid integer literal: " + value_str);
            }
        }
    }

    // 字符串字面量
    // String literal
    if (const auto* str_lit = dynamic_cast<const StringLiteral*>(expr)) {
        return Value(str_lit->getValue());
    }

    throw std::runtime_error("Unsupported literal type");
}

bool ExpressionEvaluator::compareValues(const Value& left,
                                       const std::string& op,
                                       const Value& right) {
    // NULL值处理：SQL三值逻辑，任何与NULL的比较都返回NULL（这里简化为false）
    // NULL handling: SQL three-valued logic, any comparison with NULL returns NULL (simplified to false here)
    if (left.isNull() || right.isNull()) {
        // 特殊情况：NULL = NULL 在某些SQL实现中为true，但标准是NULL
        // Special case: NULL = NULL is true in some SQL implementations, but standard is NULL
        // 这里我们简化处理，返回false
        // Here we simplify and return false
        return false;
    }

    // 使用Value类已有的比较运算符
    // Use existing comparison operators in Value class
    try {
        if (op == "=") {
            return left == right;
        } else if (op == "!=") {
            return left != right;
        } else if (op == "<") {
            return left < right;
        } else if (op == ">") {
            return left > right;
        } else if (op == "<=") {
            return left <= right;
        } else if (op == ">=") {
            return left >= right;
        } else {
            throw std::runtime_error("Unknown comparison operator: " + op);
        }
    } catch (const std::exception& e) {
        // 如果比较失败（类型不兼容等），记录日志并抛出异常
        // If comparison fails (incompatible types, etc.), log and throw
        LOG_ERROR("Value comparison error: " << e.what()
                  << " (operator: " << op << ")");
        throw;
    }
}

} // namespace tiny_sql
