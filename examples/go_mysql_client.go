package main

/*
使用 Go 官方 MySQL 驱动连接 Tiny-SQL
安装: go get -u github.com/go-sql-driver/mysql
编译: go build go_mysql_client.go
运行: ./go_mysql_client
*/

import (
	"database/sql"
	"fmt"
	"log"
	"strings"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

func main() {
	fmt.Println("Tiny-SQL 客户端测试 (Go)")
	fmt.Println(strings.Repeat("=", 50))

	// 测试基本连接
	testBasicConnection()

	// 测试不同用户
	testDifferentUsers()

	// 测试连接池
	testConnectionPool()
}

func testBasicConnection() {
	fmt.Println("\n基本连接测试:")

	// 连接字符串格式: username:password@tcp(host:port)/database
	dsn := "root:@tcp(localhost:3306)/test"

	db, err := sql.Open("mysql", dsn)
	if err != nil {
		log.Printf("✗ 连接失败: %v\n", err)
		return
	}
	defer db.Close()

	// 测试连接
	err = db.Ping()
	if err != nil {
		log.Printf("✗ Ping失败: %v\n", err)
		return
	}

	fmt.Println("✓ 成功连接到Tiny-SQL服务器")

	// 执行查询
	fmt.Println("\n执行查询测试:")

	// SELECT 1
	var result int
	err = db.QueryRow("SELECT 1").Scan(&result)
	if err != nil {
		log.Printf("✗ 查询失败: %v\n", err)
	} else {
		fmt.Printf("  SELECT 1 结果: %d\n", result)
	}

	// SELECT VERSION()
	var version string
	err = db.QueryRow("SELECT VERSION()").Scan(&version)
	if err != nil {
		log.Printf("✗ 查询失败: %v\n", err)
	} else {
		fmt.Printf("  VERSION() 结果: %s\n", version)
	}

	// SELECT DATABASE()
	var database string
	err = db.QueryRow("SELECT DATABASE()").Scan(&database)
	if err != nil {
		log.Printf("✗ 查询失败: %v\n", err)
	} else {
		fmt.Printf("  DATABASE() 结果: %s\n", database)
	}
}

func testDifferentUsers() {
	fmt.Println("\n测试不同用户:")
	fmt.Println(strings.Repeat("=", 50))

	users := []struct {
		username string
		password string
	}{
		{"root", ""},
		{"test", "test"},
		{"admin", "admin123"},
	}

	for _, user := range users {
		dsn := fmt.Sprintf("%s:%s@tcp(localhost:3307)/test", user.username, user.password)

		db, err := sql.Open("mysql", dsn)
		if err != nil {
			log.Printf("✗ 用户 '%s' 连接失败: %v\n", user.username, err)
			continue
		}

		err = db.Ping()
		if err != nil {
			log.Printf("✗ 用户 '%s' Ping失败: %v\n", user.username, err)
		} else {
			fmt.Printf("✓ 用户 '%s' 连接成功\n", user.username)
		}

		db.Close()
	}
}

func testConnectionPool() {
	fmt.Println("\n测试连接池:")
	fmt.Println(strings.Repeat("=", 50))

	dsn := "root:@tcp(localhost:3307)/test"

	db, err := sql.Open("mysql", dsn)
	if err != nil {
		log.Printf("✗ 连接失败: %v\n", err)
		return
	}
	defer db.Close()

	// 设置连接池参数
	db.SetMaxOpenConns(10)
	db.SetMaxIdleConns(5)
	db.SetConnMaxLifetime(time.Hour)

	fmt.Println("✓ 连接池配置:")
	fmt.Println("  最大打开连接数: 10")
	fmt.Println("  最大空闲连接数: 5")
	fmt.Println("  连接最大生存时间: 1小时")

	// 测试并发查询
	fmt.Println("\n执行并发查询测试:")

	done := make(chan bool, 5)

	for i := 0; i < 5; i++ {
		go func(id int) {
			var result int
			err := db.QueryRow("SELECT 1").Scan(&result)
			if err != nil {
				log.Printf("✗ 协程 %d 查询失败: %v\n", id, err)
			} else {
				fmt.Printf("  ✓ 协程 %d 查询成功\n", id)
			}
			done <- true
		}(i)
	}

	// 等待所有协程完成
	for i := 0; i < 5; i++ {
		<-done
	}

	fmt.Println("✓ 并发查询测试完成")
}
