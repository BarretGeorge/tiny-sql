#include "tiny_sql/sql/parser.h"
#include "tiny_sql/common/logger.h"
#include <iostream>

using namespace tiny_sql;

void testSQL(const std::string& sql) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Testing SQL: " << sql << "\n";
    std::cout << std::string(60, '-') << "\n";

    Parser parser(sql);
    auto stmt = parser.parse();

    if (parser.hasErrors()) {
        std::cout << "❌ Parse errors:\n";
        for (const auto& error : parser.getErrors()) {
            std::cout << "  " << error << "\n";
        }
    } else if (stmt) {
        std::cout << "✅ Parsed successfully!\n";
        std::cout << "AST: " << stmt->toString() << "\n";
    } else {
        std::cout << "⚠ Empty statement\n";
    }
}

int main() {
    Logger::instance().setLevel(LogLevel::INFO);

    std::cout << "Tiny-SQL Parser Test\n";
    std::cout << std::string(60, '=') << "\n";

    // Test SELECT statements
    testSQL("SELECT * FROM users");
    testSQL("SELECT id, name FROM users");
    testSQL("SELECT name FROM users WHERE id = 1");
    testSQL("SELECT * FROM users LIMIT 10");

    // Test INSERT statements
    testSQL("INSERT INTO users (name, age) VALUES ('Alice', 25)");
    testSQL("INSERT INTO users VALUES ('Bob', 30)");

    // Test CREATE TABLE
    testSQL("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50))");
    testSQL("CREATE TABLE products (id INT AUTO_INCREMENT PRIMARY KEY, name TEXT NOT NULL, price FLOAT DEFAULT 0.0)");

    // Test other statements
    testSQL("SHOW TABLES");
    testSQL("SHOW DATABASES");
    testSQL("USE mydb");
    testSQL("DROP TABLE users");

    // Test complex SELECT
    testSQL("SELECT name, age FROM users WHERE age > 18 AND name = 'Alice'");

    // Test error cases
    testSQL("SELCT * FROM users");  // typo
    testSQL("SELECT FROM users");    // missing columns

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "All tests completed!\n";

    return 0;
}
