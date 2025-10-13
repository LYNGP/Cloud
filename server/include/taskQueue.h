#include "func.h"
#ifndef __taskQueue__
#define __taskQueue__
typedef struct node_s
{
    int netfd;
    struct node_s *pNext;
} node_t;
typedef struct taskQueue_s
{
    node_t *pFront;
    node_t *pRear;
    int queueSize;
} taskQueue_t;
int taskQueueInit(taskQueue_t *pqueue);
int enQueue(taskQueue_t *pqueue, int netfd);
int deQueue(taskQueue_t *pqueue);
#endif
