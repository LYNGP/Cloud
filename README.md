### ✅ **0. 项目简介**

这是一个在 Linux 下用 C 语言实现的轻量级网盘/文件管理服务器，支持多用户注册与登录、文件上传/下载（含断点续传）、目录操作（cd/ls/mkdir/rmdir/rm）、逻辑删除（回收/恢复）、秒传（快速完成重复文件存储）和并发处理（多线程池 + epoll），使用 MySQL 存储元数据（虚拟文件系统信息、用户信息等）。


#### **0.1 项目动机：**
一是用来锻炼自己的C语言，Linux系统编程技能，
二是方便实验室内部权重共享、训练集管理、日志归档、保密文档传输等。

#### **0.2 项目功能（功能概述）**
1.功能介绍：

- 用户管理
  - 用户注册：客户端申请用户名，服务器查询数据库避免重复；服务器生成 salt（示例为 $6$… 格式），返回给客户端以便客户端做哈希（服务端保存盐与密文）。
  - 用户登录：服务器根据 username 查 salt 并发送给客户端，客户端用 salt 对密码处理后返回密文，服务器与数据库中保存的密文比较以验证身份。
- 并发连接处理
  - 主线程基于 epoll 监听 socket 与退出管道，accept 到连接后将连接描述符放入任务队列，唤醒工作线程处理。
  - 线程池中的工作线程（消费者）从队列取出 netfd，建立/复用 MySQL 连接并调用业务处理函数。
- 文件与目录操作（命令集为自定义私有协议）
  - cd/ls/pwd：虚拟路径栈（pathStack）维护当前路径，查询数据库返回目录结构或路径字符串。
  - mkdir/rmdir：在文件表中插入/逻辑删除目录记录（type=D），支持逻辑恢复已删除目录。
  - puts/gets（上传/下载）：
    - 上传：先上传文件的 sha1（或 sha1sum）作为唯一标识，服务器查询是否已有相同 sha1：
      - 若存在：秒传（只在数据库中插入或恢复引用，不上传实际内容）。
      - 不存在：客户端上传真实文件（将文件以 sha1 命名存储），服务器写入并实时返回上传进度；上传成功后插入 file 表记录。
    - 下载：服务器查找对应 sha1 的磁盘文件并使用 mmap + send（零拷贝思路）直接把数据发送给客户端，支持断点续传（客户端告知已存在的长度 existLength，服务器从该偏移发送）。
  - rm：逻辑删除文件（设置 tomb = 1）。
- 断点续传
  - 上传端与服务器在传输前交换已经存在的长度或 sha1；下载时客户端发送 existLength，服务器只发送剩余部分。上传过程中服务器记录并周期性/实时返回进度；若连接中断，可从已上传长度继续。
- 秒传
  - 依赖文件内容唯一标识（sha1），若数据库已有相同 sha1 的记录，则服务器不要求上传文件内容，仅在元数据表中插入/恢复记录即可，实现“秒传”。
- 日志记录与超时/断开机制（部分由信号/管道控制）
  - 主进程/子进程通过 pipe + signal 协同退出，日志通过 SYSTEM_LOG 宏记录。

2.用户系统:

-     用户登录和注册功能，登录过程中，密码通过密文传输；
-     服务器先传输salt值给客户端，客户端crypt加密后，传输密码个密文个服务器；

3.文件系统:

-     实现文件存储和文件元数据相分离，文件内容存储在服务器的文件系统中，文件的元数据信息存储在MySQL数据库中；
-     文件内容存储在文件系统中，每个文件的文件名是文件内容生成sha1sum，用于唯一标识文件内容；
-     文件元数据存储在虚拟文件表中，每个用户需要拥有自己独立的目录结构，每个用户只能看到自己上传的文件和创建的目录。

4.运行环境：
```bash
mysql  Ver 8.0.43-0ubuntu0.22.04.2 for Linux on aarch64 ((Ubuntu))
cmake version 3.22.1
gcc (Ubuntu 11.4.0-1ubuntu1~22.04.2) 11.4.0
GNU gdb (Ubuntu 12.1-0ubuntu1~22.04.3) 12.1
```

### ✅ **1. 核心功能模块**

#### 1.1 客户端核心功能:

| 模块 | 功能描述 |
|------|----------|
| `client.c` | 客户端入口，连接服务器并启动交互式命令行界面。 |
| `business.c` | 实现用户注册、登录认证逻辑（基于 `crypt()` 和 salt）。 |
| `handleCommand.c` | 实现客户端命令解析与执行（如 `ls`, `cd`, `puts`, `gets`, `rm`, `mkdir` 等）。 |
| `transMsg.c` | 提供可靠的 TCP 数据接收函数 `recvn()`，防止粘包/半包问题。 |

#### 1.2 服务端核心功能:

| 模块 | 功能描述 |
|------|------|
| `main.c` | 服务端入口，初始化 epoll、线程池、信号处理 |
| `tcpInit.c` | 封装 socket、bind、listen |
| `epoll.c` | 封装 epoll_add / epoll_del |
| `threadPool.c` | 线程池初始化、任务分发、数据库连接 |
| `taskQueue.c` | 线程安全任务队列（存放客户端 fd） |
| `worker.c` | 每个线程的任务处理函数，处理注册/登录/命令 |
| `handleCommand.c` | 服务端命令处理核心（cd/ls/puts/gets/rm/mkdir/rmdir） |
| `pathStack.c` | 目录栈结构，模拟 `cd` 路径切换 |
| `transMsg.c` | 封装 `recvn()`，解决 TCP 粘包问题 |
| `business.c`（客户端） | 客户端逻辑（注册/登录/命令交互） |

#### 1.3 支持的命令（客户端）

| 命令 | 功能 |
|------|------|
| `ls <目录>` | 列出远程目录内容 |
| `cd <目录>` | 切换远程目录 |
| `pwd` | 显示当前远程路径 |
| `puts <文件>` | 上传文件（支持秒传，基于 SHA-1） |
| `gets <文件>` | 下载文件（支持断点续传） |
| `rm <文件>` | 删除远程文件 |
| `mkdir <目录>` | 创建远程目录 |
| `rmdir <目录>` | 删除远程目录 |
| `exit` | 退出客户端 |

#### 1.4 功能展示：

客户端编译：
```bash
./build.sh
-- The C compiler identification is GNU 11.4.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE  
-- Found OpenSSL: /usr/lib/aarch64-linux-gnu/libcrypto.so (found version "3.0.2")  
-- Configuring done
-- Generating done
-- Build files have been written to: /home/HwHiAiUser/gp/netDisk/Cloud/client/build
[ 20%] Building C object CMakeFiles/client.dir/src/business.c.o
[ 40%] Building C object CMakeFiles/client.dir/src/client.c.o
[ 60%] Building C object CMakeFiles/client.dir/src/handleCommand.c.o
[ 80%] Building C object CMakeFiles/client.dir/src/transMsg.c.o
[100%] Linking C executable ../client
[100%] Built target client
```
服务端编译：
```bash
./build.sh
-- The C compiler identification is GNU 11.4.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE  
-- Found OpenSSL: /usr/lib/aarch64-linux-gnu/libcrypto.so (found version "3.0.2")  
-- Found PkgConfig: /usr/bin/pkg-config (found version "0.29.2") 
-- Checking for module 'mysqlclient'
--   Found mysqlclient, version 21.2.43
-- Configuring done
-- Generating done
-- Build files have been written to: /home/HwHiAiUser/gp/netDisk/Cloud/server/build
[ 10%] Building C object CMakeFiles/server.dir/src/handleCommand.c.o
[ 20%] Building C object CMakeFiles/server.dir/src/pathStack.c.o
[ 30%] Building C object CMakeFiles/server.dir/src/main.c.o
[ 40%] Building C object CMakeFiles/server.dir/src/epoll.c.o
[ 50%] Building C object CMakeFiles/server.dir/src/taskQueue.c.o
[ 60%] Building C object CMakeFiles/server.dir/src/threadPool.c.o
[ 70%] Building C object CMakeFiles/server.dir/src/tcpInit.c.o
[ 80%] Building C object CMakeFiles/server.dir/src/transMsg.c.o
[ 90%] Building C object CMakeFiles/server.dir/src/worker.c.o
[100%] Linking C executable ../server
[100%] Built target server
```
服务端开启：
```bash
(base) root@davinci-mini:/home/HwHiAiUser/gp/netDisk/Cloud/server# ./server 0.0.0.0 6666 5
I got 1 task!
I am master,I send a netfd = 11
I am worker,I got a netfd = 11
targetDir = 
dirExist = 1
targetDir = 
dirExist = 1
targetDir = 
dirExist = 1
targetDir = /test1
dirExist = 1
mysql_query:Duplicate entry 'qaz-/test1/SQL.pdf-F-0' for key 'file.username'
targetDir = 
dirExist = 1
targetDir = 
dirExist = 1
1 client left!
I got 1 task!
I am master,I send a netfd = 11
I am worker,I got a netfd = 11
targetDir = 
dirExist = 1
targetDir = /test1
dirExist = 1
1 client left!
Parent is going to exit!

```
客户端开启：
```bash
(base) root@davinci-mini:/home/HwHiAiUser/gp/netDisk/Cloud/client# ./client 10.14.118.48 6666
#---------------------------------------#
请选择你操作
1.用户注册  2.用户登录  3.退出
#---------------------------------------#
1
Please enter your username:qaz
Please enter your password:qaz123
Please enter your password again:qaz123
passWord =qaz123,rePassWord =qaz123
isSame = 1
Success!
qaz@10.14.118.48:6666:~$ ls .
qaz@10.14.118.48:6666:~$ mkdir test0
Success
qaz@10.14.118.48:6666:~$ ls .
D               test0                           4096
qaz@10.14.118.48:6666:~$ mkdir test1
Success
qaz@10.14.118.48:6666:~$ mkdir test2
Success
qaz@10.14.118.48:6666:~$ ls .
D               test0                           4096
D               test1                           4096
D               test2                           4096
qaz@10.14.118.48:6666:~$ cd test1
success!
qaz@10.14.118.48:6666:~/test1$ mkdir test1_0
Success
qaz@10.14.118.48:6666:~/test1$ ls .
D               test1_0                         4096
qaz@10.14.118.48:6666:~/test1$ puts SQL.pdf
sha1sum = 9b9c7d6894c7055865dffad411a2aea8f713282c
filesize = 1178086
100.00%
qaz@10.14.118.48:6666:~/test1$ puts SQL.pdf
sha1sum = 9b9c7d6894c7055865dffad411a2aea8f713282c
Failed!
qaz@10.14.118.48:6666:~/test1$ cd ..
success!
qaz@10.14.118.48:6666:~$ ls .
D               test0                           4096
D               test1                           4096
D               test2                           4096
qaz@10.14.118.48:6666:~$ rmdir test2
Success
qaz@10.14.118.48:6666:~$ ls .    
D               test0                           4096
D               test1                           4096
qaz@10.14.118.48:6666:~$ exit

```
同一局域网下的客户端：
```bash
(gp) fusion@fusion-SYS-740GP-TNRT:/media/fusion/8TB-2/gy/gp/client$ ./client 10.14.118.48 6666
#---------------------------------------#
请选择你操作
1.用户注册  2.用户登录  3.退出
#---------------------------------------#
2
Please enter your username:qaz
Please enter your password:qaz123
Success!
qaz@10.14.118.48:6666:~$ ls .
D               test0                           4096
D               test1                           4096
qaz@10.14.118.48:6666:~$ cd test1
success!
qaz@10.14.118.48:6666:~/test1$ ls .
D               test1_0                         4096
F               SQL.pdf                         1178086
qaz@10.14.118.48:6666:~/test1$ gets SQL.pdf
100.00%
qaz@10.14.118.48:6666:~/test1$ rm SQL.pdf
Success
qaz@10.14.118.48:6666:~/test1$ ls .
D               test1_0                         4096
qaz@10.14.118.48:6666:~/test1$ exit
```
关闭服务端：
```bash
(base) root@davinci-mini:/home/HwHiAiUser/gp/netDisk# ss -ltnp | grep ':6666'
LISTEN 0      50           0.0.0.0:6666       0.0.0.0:*    users:(("server",pid=775850,fd=4))       
(base) root@davinci-mini:/home/HwHiAiUser/gp/netDisk# kill -10 775850
```


### ✅ **2. 核心功能详解**

#### 2.1. 用户系统（注册/登录）
- 使用 `crypt(password, salt)` 加密密码。
- 每个用户注册时在数据库中创建一条记录，并在文件表中创建其根目录。
- 登录成功后，服务端为该用户建立一个 `cmdShare_t` 结构，保存其用户名、数据库连接、当前目录栈。

#### 2.2 文件系统结构（逻辑路径 vs 实际存储）
| 概念 | 说明 |
|------|------|
| 逻辑路径 | 用户看到的路径，如 `/Documents/Notes` |
| 实际文件 | 以 SHA1 值命名，存储在 `netDisk/` 目录下 |
| 数据库 | 记录文件名、路径、SHA1、大小、类型、父目录ID、是否删除（tomb） |

> ✅ 实现**秒传**：上传前比对 SHA1，若已存在则直接插入记录，不接收文件。


#### 2.3 网络通信协议（私有协议）

##### 消息格式：
```c
typedef struct {
    int length;     // 数据长度
    char data[1024]; // 实际数据
} train_t;
```
- 先发长度，再发数据，避免粘包。
- 所有命令、文件内容、状态码都通过该结构传输。

##### 通信流程：
1. 客户端连接后先选择操作：`1=注册`，`2=登录`
2. 登录成功后进入命令交互模式
3. 每条命令封装为 `train_t` 发送，服务端解析并执行
4. 所有文件传输（上传/下载）都通过 `train_t` 分块传输，支持进度回显

---

#### 2.4 并发模型

| 组件 | 作用 |
|------|------|
| `epoll` | 主线程监听所有 fd，包括 listen fd 和管道退出信号 |
| `线程池` | 固定数量线程，处理客户端命令 |
| `任务队列` | 线程安全队列，主线程将新连接 fd 入队，工作线程取出处理 |

#### 2.5 数据库设计（MySQL）

##### `user` 表：
```sql
username | salt | encryted_password | tomb
```

##### `file` 表：
```sql
id | filename | username | pre_id | path | type (D/F) | sha1sum | tomb | filesize
```

- `pre_id`：父目录 ID，构建目录树
- `tomb`：逻辑删除标志（0=存在，1=已删除）
- `sha1sum`：文件内容指纹，用于秒传和磁盘存储


| 特性 | 实现方式 |
|------|----------|
| **秒传** | 上传前比对 SHA1，存在则直接插入记录 |
| **断点续传** | 客户端发送已接收字节数，服务端从偏移量发送 |
| **逻辑删除** | 文件不真正删除，仅设置 `tomb=1`，可恢复 |
| **目录栈** | 使用链表栈模拟 `cd` 行为，支持相对路径 |
| **并发安全** | 线程池 + 互斥锁 + 条件变量 |
| **优雅退出** | 父进程捕获 `SIGUSR1`，通过管道通知子进程退出线程池 |


### ✅ **安全性**
- 使用 `crypt(password, salt)` 进行密码加密，基于 Unix 传统加密方式。
- 用户名、密码、命令均通过私有协议传输，**未加密通信内容**（无 TLS/SSL）。
- 无权限控制逻辑（如用户不能访问他人目录），依赖于服务端路径隔离。


### ✅ **目录结构**
- 客户端本地创建 `netDisk/` 目录用于下载文件。
- 服务端为每个用户创建独立目录（如 `/home/server/users/zhangsan`）。

### ✅ **依赖库**
- 标准库：`pthread`, `openssl`（SHA-1 计算）
- 系统调用：`socket`, `mmap`, `open`, `read`, `write`, `stat`, `mkdir`, `chdir`
- 非标准库：`shadow.h`, `crypt.h`（密码加密）