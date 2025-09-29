#include "threadPool.h"

void *threadFunc(void *arg)
{
    threadPool_t *pthreadPool = (threadPool_t *)arg;
    while (1)
    {
        // 取出任务
        pthread_mutex_lock(&pthreadPool->mutex);
        while (pthreadPool->taskQueue.queue_size == 0 && pthreadPool->exitFlag == 0)
        {
            pthread_cond_wait(&pthreadPool->cond, &pthreadPool->mutex);
        }
        if (pthreadPool->exitFlag == 1)
        {
            pthread_mutex_unlock(&pthreadPool->mutex);
            pthread_exit(NULL);
        }
        printf("worker %lu get a task\n", (unsigned long)pthread_self());
        int netfd = pthreadPool->taskQueue.phead->fd;
        DeQueue(&pthreadPool->taskQueue);
        pthread_mutex_unlock(&pthreadPool->mutex);
        // 处理任务
        transFile(netfd);
        close(netfd);
    }
}

int make_worker(threadPool_t *pthreadPool)
{
    for (int i = 0; i < pthreadPool->worker_num; i++)
    {
        pthread_create(&pthreadPool->tidArr[i], NULL, threadFunc, (void *)pthreadPool);
    }
    return 0;
}
