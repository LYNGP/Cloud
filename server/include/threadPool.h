#include"func.h"
#include"worker.h"
#include"taskQueue.h"
#include"pathStack.h"
#include"transMsg.h"
#include"handleCommand.h"


#ifndef __threadPool__
#define __threadPool__
// 记录线程池中线程的tid和数量
typedef struct tidArr_s{
    pthread_t *arr;
    int workerNum;
}tidArr_t;

//用来描述整个线程池的信息，也是线程间共享的数据
typedef struct threadPool_s{
    // 记录所有的子进程信息
    tidArr_t tidArr;
    //任务队列
    taskQueue_t taskQueue;
    //锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;
    // 退出标志位
    int exitFlag;
}threadPool_t;

int tidArrInit(tidArr_t *ptidArr,int workerNum);

void* threadFunc(void* arg);
int threadPoolInit(threadPool_t *pthreadPool,int workerNum);
int makeWorker(threadPool_t *pthreadPool);
int connectMySQL(MYSQL* mysql);



// 其他
int tcpInit(const char* ip,const char* port,int *psockfd);
int epollAdd(int epfd,int fd);
int epollDEL(int epfd,int fd);
#endif
