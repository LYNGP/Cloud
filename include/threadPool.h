#pragma once

#include "func.h"
#include "taskQueue.h"

// 主线程需要维持每一个子线程的信息
typedef struct threadPool_s
{
    pthread_t *tidArr;     // 子线程id
    int worker_num;        // 子线程数量
    queue_t taskQueue;     // 任务队列
    pthread_mutex_t mutex; // 互斥锁
    pthread_cond_t cond;   // 条件变量
    int exitFlag;          // 退出标志 0:未退出 1:退出

} threadPool_t;

int make_worker(threadPool_t *pthreadPool);
void *threadFunc(void *arg);

int tcpInit(int *psockfd, const char *ip, const char *port);
int epollAdd(int epfd, int fd);
int epollDel(int epfd, int fd);

int transFile(int netfd);
