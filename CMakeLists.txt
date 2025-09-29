# 设定最低 CMake 版本
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和语言
project(FileTransferProject C)

# 设定 C 语言标准为 C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# --- 编译服务器端 ---
# 定义服务器的源文件
set(SERVER_SRCS
    src/main.c
    src/epoll.c
    src/taskQueue.c
    src/tcpInit.c
    src/transFile.c
    src/worker.c
)

# 添加服务器可执行文件
add_executable(server ${SERVER_SRCS})

# 服务器使用了线程池，需要链接 pthread 库
target_link_libraries(server pthread)


# --- 编译客户端 ---
# 定义客户端的源文件
set(CLIENT_SRCS
    src/client.c
)

# 添加客户端可执行文件
add_executable(client ${CLIENT_SRCS})

