cmake_minimum_required(VERSION 3.20)
project(tiny-sql VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
add_compile_options(-Wall -Wextra -O2)

# 查找OpenSSL
find_package(OpenSSL REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 收集源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(tiny-sql main.cpp ${SOURCES})

# 链接库
target_link_libraries(tiny-sql
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
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
