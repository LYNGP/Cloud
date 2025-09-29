#include "taskQueue.h"

int EnQueue(queue_t *pqueue, int fd)
{
    node_t *pnew = (node_t *)calloc(1, sizeof(node_t));
    pnew->fd = fd;
    if (pqueue->queue_size == 0)
    {
        pqueue->phead = pnew;
        pqueue->ptail = pnew;
    }
    else
    {
        pqueue->ptail->pnext = pnew;
        pqueue->ptail = pnew;
    }
    pqueue->queue_size++;
    return 0;
}

int DeQueue(queue_t *pqueue)
{
    // 增加对空队列的检查，防止意外调用导致崩溃
    if (pqueue->queue_size == 0)
    {
        return -1; // 或者返回一个错误码
    }
    node_t *pcur = pqueue->phead;
    pqueue->phead = pcur->pnext;
    free(pcur);
    pcur = NULL; // 释放后置为NULL是个好习惯
    if (pqueue->queue_size == 1)
    {
        pqueue->ptail = NULL;
    }
    pqueue->queue_size--;
    return 0;
}