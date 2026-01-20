#!/usr/bin/env node

/**
 * 使用 Node.js mysql2 驱动连接 Tiny-SQL
 * 安装: npm install mysql2
 * 运行: node nodejs_mysql_client.js
 */

const mysql = require('mysql2');
const util = require('util');

// 基本连接测试
async function testBasicConnection() {
    console.log('\n基本连接测试:');
    console.log('='.repeat(50));

    const connection = mysql.createConnection({
        host: 'localhost',
        port: 3307,
        user: 'root',
        password: '',
        database: 'test'
    });

    // 转换为Promise
    const query = util.promisify(connection.query).bind(connection);

    try {
        // 连接
        await util.promisify(connection.connect).bind(connection)();
        console.log('✓ 成功连接到Tiny-SQL服务器');

        // 测试查询
        console.log('\n执行查询测试:');

        // SELECT 1
        let results = await query('SELECT 1 as result');
        console.log('  SELECT 1 结果:', results[0]);

        // SELECT VERSION()
        results = await query('SELECT VERSION() as version');
        console.log('  VERSION() 结果:', results[0]);

        // SELECT DATABASE()
        results = await query('SELECT DATABASE() as db');
        console.log('  DATABASE() 结果:', results[0]);

        // SHOW DATABASES
        results = await query('SHOW DATABASES');
        console.log('  SHOW DATABASES 结果:', results);

    } catch (error) {
        console.error('✗ 错误:', error.message);
    } finally {
        connection.end();
        console.log('\n✓ 连接已关闭');
    }
}

// 测试连接池
async function testConnectionPool() {
    console.log('\n测试连接池:');
    console.log('='.repeat(50));

    const pool = mysql.createPool({
        host: 'localhost',
        port: 3307,
        user: 'root',
        password: '',
        database: 'test',
        waitForConnections: true,
        connectionLimit: 10,
        queueLimit: 0
    });

    const promisePool = pool.promise();

    try {
        console.log('✓ 连接池配置:');
        console.log('  连接限制: 10');

        // 并发查询测试
        console.log('\n执行并发查询测试:');

        const promises = [];
        for (let i = 0; i < 5; i++) {
            promises.push(
                promisePool.query('SELECT 1 as result')
                    .then(([rows]) => {
                        console.log(`  ✓ 查询 ${i + 1} 成功:`, rows[0]);
                    })
                    .catch(err => {
                        console.error(`  ✗ 查询 ${i + 1} 失败:`, err.message);
                    })
            );
        }

        await Promise.all(promises);
        console.log('✓ 并发查询测试完成');

    } catch (error) {
        console.error('✗ 错误:', error.message);
    } finally {
        await pool.end();
        console.log('✓ 连接池已关闭');
    }
}

// 测试不同用户
async function testDifferentUsers() {
    console.log('\n测试不同用户:');
    console.log('='.repeat(50));

    const users = [
        { user: 'root', password: '' },
        { user: 'test', password: 'test' },
        { user: 'admin', password: 'admin123' }
    ];

    for (const { user, password } of users) {
        const connection = mysql.createConnection({
            host: 'localhost',
            port: 3307,
            user: user,
            password: password
        });

        try {
            await util.promisify(connection.connect).bind(connection)();
            console.log(`✓ 用户 '${user}' 连接成功`);
            connection.end();
        } catch (error) {
            console.error(`✗ 用户 '${user}' 连接失败:`, error.message);
        }
    }
}

// 测试事务（即使Tiny-SQL暂不支持，展示用法）
async function testTransaction() {
    console.log('\n测试事务（示例）:');
    console.log('='.repeat(50));

    const connection = mysql.createConnection({
        host: 'localhost',
        port: 3307,
        user: 'root',
        password: ''
    });

    const query = util.promisify(connection.query).bind(connection);

    try {
        await util.promisify(connection.connect).bind(connection)();

        // 开始事务
        await query('START TRANSACTION');
        console.log('✓ 事务已开始');

        // 这里可以执行多个SQL语句
        // await query('INSERT INTO ...');
        // await query('UPDATE ...');

        // 提交事务
        await query('COMMIT');
        console.log('✓ 事务已提交');

    } catch (error) {
        console.error('✗ 事务错误:', error.message);
        try {
            await query('ROLLBACK');
            console.log('✓ 事务已回滚');
        } catch (rollbackError) {
            console.error('✗ 回滚失败:', rollbackError.message);
        }
    } finally {
        connection.end();
    }
}

// 主函数
async function main() {
    console.log('Tiny-SQL 客户端测试 (Node.js mysql2)');
    console.log('='.repeat(50));

    try {
        await testBasicConnection();
        await testConnectionPool();
        await testDifferentUsers();
        // await testTransaction(); // 取消注释以测试事务
    } catch (error) {
        console.error('主程序错误:', error);
    }
}

// 运行主函数
main().catch(console.error);
