cmake_minimum_required(VERSION 3.20)
project(tiny-sql VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
add_compile_options(-Wall -Wextra -O2)

# 平台检测
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Platform: Linux (using epoll)")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
    message(STATUS "Platform: macOS (using kqueue)")
elseif(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(PLATFORM_BSD TRUE)
    message(STATUS "Platform: BSD (using kqueue)")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# 查找OpenSSL
find_package(OpenSSL REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 收集通用源文件
file(GLOB COMMON_SOURCES
    "src/common/*.cpp"
    "src/network/socket_utils.cpp"
    "src/network/tcp_connection.cpp"
    "src/network/event_loop.cpp"
    "src/network/server.cpp"
    "src/protocol/*.cpp"
    "src/auth/*.cpp"
    "src/session/*.cpp"
    "src/command/*.cpp"
    "src/sql/*.cpp"
    "src/storage/*.cpp"
)

# 根据平台添加特定的事件循环实现
if(PLATFORM_LINUX)
    list(APPEND COMMON_SOURCES
        "src/network/epoll_event_loop.cpp"
        "src/network/epoll_server.cpp"  # 保留向后兼容
    )
elseif(PLATFORM_MACOS OR PLATFORM_BSD)
    list(APPEND COMMON_SOURCES
        "src/network/kqueue_event_loop.cpp"
    )
endif()

# 创建可执行文件
add_executable(tiny-sql main.cpp ${COMMON_SOURCES})

# 链接库
target_link_libraries(tiny-sql
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# 创建SQL解析器测试程序
add_executable(test_sql_parser test_sql_parser.cpp ${COMMON_SOURCES})
target_link_libraries(test_sql_parser
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# 安装规则
install(TARGETS tiny-sql DESTINATION bin)

# 测试
enable_testing()

# 如果有tests目录,添加测试
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()

# 打印配置信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
