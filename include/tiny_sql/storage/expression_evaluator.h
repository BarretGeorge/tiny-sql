#pragma once

#include "tiny_sql/sql/ast.h"
#include "tiny_sql/storage/value.h"
#include "tiny_sql/storage/table.h"
#include <vector>
#include <stdexcept>

namespace tiny_sql {

/**
 * 表达式求值器 - 用于WHERE子句的评估
 * Expression Evaluator - Used for WHERE clause evaluation
 */
class ExpressionEvaluator {
public:
    /**
     * 评估表达式，返回布尔结果（用于WHERE子句过滤）
     * Evaluate expression and return boolean result (for WHERE clause filtering)
     *
     * @param expr 要评估的表达式（可以为nullptr，表示无过滤条件）
     * @param row 当前行数据
     * @param columns 表的列定义
     * @return 表达式的布尔结果，true表示匹配
     * @throws std::runtime_error 如果列名不存在或表达式无效
     */
    static bool evaluate(const Expression* expr,
                        const Row& row,
                        const std::vector<ColumnDef>& columns);

    /**
     * 评估表达式，返回Value结果（用于未来的计算列等功能）
     * Evaluate expression and return Value result (for future calculated columns)
     *
     * @param expr 要评估的表达式
     * @param row 当前行数据
     * @param columns 表的列定义
     * @return 表达式的值
     * @throws std::runtime_error 如果列名不存在或表达式无效
     */
    static Value evaluateValue(const Expression* expr,
                              const Row& row,
                              const std::vector<ColumnDef>& columns);

private:
    /**
     * 评估二元表达式（比较运算符和逻辑运算符）
     * Evaluate binary expression (comparison and logical operators)
     */
    static bool evaluateBinaryExpression(const BinaryExpression* expr,
                                        const Row& row,
                                        const std::vector<ColumnDef>& columns);

    /**
     * 从行中获取标识符对应的值
     * Get value for identifier from row
     */
    static Value evaluateIdentifier(const Identifier* id,
                                    const Row& row,
                                    const std::vector<ColumnDef>& columns);

    /**
     * 将字面量转换为Value
     * Convert literal to Value
     */
    static Value evaluateLiteral(const Expression* expr);

    /**
     * 比较两个值
     * Compare two values
     *
     * @param left 左值
     * @param op 操作符 (=, !=, <, >, <=, >=)
     * @param right 右值
     * @return 比较结果
     */
    static bool compareValues(const Value& left,
                             const std::string& op,
                             const Value& right);
};

} // namespace tiny_sql
