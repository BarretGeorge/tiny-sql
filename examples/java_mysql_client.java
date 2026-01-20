import java.sql.*;
import java.util.Properties;

/**
 * 使用 JDBC MySQL Connector 连接 Tiny-SQL
 *
 * 依赖: MySQL Connector/J
 * Maven:
 * <dependency>
 *     <groupId>mysql</groupId>
 *     <artifactId>mysql-connector-java</artifactId>
 *     <version>8.0.33</version>
 * </dependency>
 *
 * 编译: javac -cp mysql-connector-java-8.0.33.jar java_mysql_client.java
 * 运行: java -cp .:mysql-connector-java-8.0.33.jar TinySQLClient
 */
public class TinySQLClient {

    private static final String HOST = "localhost";
    private static final int PORT = 3307;
    private static final String DATABASE = "test";

    public static void main(String[] args) {
        System.out.println("Tiny-SQL 客户端测试 (Java JDBC)");
        System.out.println("=".repeat(50));

        testBasicConnection();
        testDifferentUsers();
        testPreparedStatement();
        testConnectionProperties();
    }

    /**
     * 基本连接测试
     */
    private static void testBasicConnection() {
        System.out.println("\n基本连接测试:");

        String url = String.format("jdbc:mysql://%s:%d/%s", HOST, PORT, DATABASE);

        try (Connection conn = DriverManager.getConnection(url, "root", "")) {
            System.out.println("✓ 成功连接到Tiny-SQL服务器");

            DatabaseMetaData metaData = conn.getMetaData();
            System.out.println("  驱动名称: " + metaData.getDriverName());
            System.out.println("  驱动版本: " + metaData.getDriverVersion());
            System.out.println("  数据库产品: " + metaData.getDatabaseProductName());
            System.out.println("  数据库版本: " + metaData.getDatabaseProductVersion());

            // 执行查询
            System.out.println("\n执行查询测试:");

            try (Statement stmt = conn.createStatement()) {
                // SELECT 1
                ResultSet rs = stmt.executeQuery("SELECT 1 as result");
                if (rs.next()) {
                    System.out.println("  SELECT 1 结果: " + rs.getInt("result"));
                }
                rs.close();

                // SELECT VERSION()
                rs = stmt.executeQuery("SELECT VERSION() as version");
                if (rs.next()) {
                    System.out.println("  VERSION() 结果: " + rs.getString("version"));
                }
                rs.close();

                // SELECT DATABASE()
                rs = stmt.executeQuery("SELECT DATABASE() as db");
                if (rs.next()) {
                    System.out.println("  DATABASE() 结果: " + rs.getString("db"));
                }
                rs.close();
            }

            System.out.println("\n✓ 连接已关闭");

        } catch (SQLException e) {
            System.err.println("✗ 错误: " + e.getMessage());
            e.printStackTrace();
        }
    }

    /**
     * 测试不同用户
     */
    private static void testDifferentUsers() {
        System.out.println("\n测试不同用户:");
        System.out.println("=".repeat(50));

        String[][] users = {
            {"root", ""},
            {"test", "test"},
            {"admin", "admin123"}
        };

        String url = String.format("jdbc:mysql://%s:%d/%s", HOST, PORT, DATABASE);

        for (String[] user : users) {
            String username = user[0];
            String password = user[1];

            try (Connection conn = DriverManager.getConnection(url, username, password)) {
                System.out.println("✓ 用户 '" + username + "' 连接成功");
            } catch (SQLException e) {
                System.err.println("✗ 用户 '" + username + "' 连接失败: " + e.getMessage());
            }
        }
    }

    /**
     * 测试预处理语句
     */
    private static void testPreparedStatement() {
        System.out.println("\n测试预处理语句:");
        System.out.println("=".repeat(50));

        String url = String.format("jdbc:mysql://%s:%d/%s", HOST, PORT, DATABASE);

        try (Connection conn = DriverManager.getConnection(url, "root", "")) {
            // 使用预处理语句
            String sql = "SELECT ? as result";

            try (PreparedStatement pstmt = conn.prepareStatement(sql)) {
                pstmt.setInt(1, 42);

                ResultSet rs = pstmt.executeQuery();
                if (rs.next()) {
                    System.out.println("✓ 预处理语句结果: " + rs.getInt("result"));
                }
                rs.close();
            }

        } catch (SQLException e) {
            System.err.println("✗ 预处理语句错误: " + e.getMessage());
        }
    }

    /**
     * 测试连接属性
     */
    private static void testConnectionProperties() {
        System.out.println("\n测试连接属性:");
        System.out.println("=".repeat(50));

        String url = String.format("jdbc:mysql://%s:%d/%s", HOST, PORT, DATABASE);

        Properties props = new Properties();
        props.setProperty("user", "root");
        props.setProperty("password", "");
        props.setProperty("useSSL", "false");
        props.setProperty("serverTimezone", "UTC");
        props.setProperty("characterEncoding", "utf8");

        try (Connection conn = DriverManager.getConnection(url, props)) {
            System.out.println("✓ 使用属性连接成功");

            // 显示连接属性
            System.out.println("  自动提交: " + conn.getAutoCommit());
            System.out.println("  只读模式: " + conn.isReadOnly());
            System.out.println("  事务隔离级别: " + conn.getTransactionIsolation());
            System.out.println("  Catalog: " + conn.getCatalog());

        } catch (SQLException e) {
            System.err.println("✗ 连接属性测试失败: " + e.getMessage());
        }
    }
}
