#include "func.h"
#include "threadPool.h"

int exitpipe[2]; // 退出管道
void sigFunc(int signum)
{
    printf("receive signal %d\n", signum);
    write(exitpipe[1], "1", 1);
}
int main(int argc, char *argv[])
{ // ./bin/server 0.0.0.0 8080 4
    ARGS_CHECK(argc, 4);
    pipe(exitpipe);
    if (fork() != 0)
    {
        close(exitpipe[0]);
        signal(SIGUSR1, sigFunc);
        wait(NULL);
        exit(0);
    }
    close(exitpipe[1]);

    int worker_num = atoi(argv[3]);                                         // 子线程数量
    threadPool_t threadPool;                                                // 包含线程池运行的所有组件
    threadPool.tidArr = (pthread_t *)calloc(worker_num, sizeof(pthread_t)); // 子线程id数组
    threadPool.worker_num = worker_num;                                     // 子线程数量
    pthread_mutex_init(&threadPool.mutex, NULL);                            // 初始化互斥锁
    pthread_cond_init(&threadPool.cond, NULL);                              // 初始化条件变量
    bzero(&threadPool.taskQueue, sizeof(queue_t));                          // 初始化任务队列
    threadPool.exitFlag = 0;                                                // 子线程退出标志位 0:未退出 1:退出

    int sockfd;                         // 套接字
    tcpInit(&sockfd, argv[1], argv[2]); // 创建tcp连接
    int epfd = epoll_create(1024);      // 创建epoll句柄
    epollAdd(epfd, sockfd);             // 注册套接字到epoll中
    epollAdd(epfd, exitpipe[0]);        // 注册退出管道到epoll中

    make_worker(&threadPool); // 创建子线程, 网络服务准备好后再创建工作线程
    printf("server start with %d workers:\n", worker_num);

    while (1)
    {
        struct epoll_event readySet[1];
        int readyNum = epoll_wait(epfd, readySet, 1, -1); // 等待事件
        for (int i = 0; i < readyNum; ++i)
        {
            if (readySet[i].data.fd == sockfd)
            {
                int netfd = accept(sockfd, NULL, NULL);
                pthread_mutex_lock(&threadPool.mutex);
                EnQueue(&threadPool.taskQueue, netfd);
                printf("new client connected, master send a task to worker\n");
                pthread_cond_signal(&threadPool.cond);
                pthread_mutex_unlock(&threadPool.mutex);
            }
            else if (readySet[i].data.fd == exitpipe[0])
            {
                printf("server exit\n");
                pthread_mutex_lock(&threadPool.mutex);
                threadPool.exitFlag = 1;
                pthread_cond_broadcast(&threadPool.cond);
                pthread_mutex_unlock(&threadPool.mutex);

                for (int j = 0; j < threadPool.worker_num; j++)
                {
                    pthread_join(threadPool.tidArr[j], NULL);
                }
                printf("server exit success\n");
                exit(0);
            }
        }
    }
    return 0;
}