cmake_minimum_required(VERSION 3.10)
project(netDisk_client C)

set(CMAKE_C_STANDARD 11)

# 1. 收集源文件
file(GLOB_RECURSE SRC_LIST CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")

# 2. 头文件搜索路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 3. 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 4. 生成可执行文件
add_executable(client ${SRC_LIST})

# 5. 链接第三方库
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(client
    PRIVATE
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        crypt      # libcrypt
)