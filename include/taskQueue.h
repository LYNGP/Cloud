#pragma once

#include "func.h"

typedef struct node_s
{
    int fd;
    struct node_s *pnext;
} node_t;

typedef struct queue_s
{
    node_t *phead;
    node_t *ptail;
    int queue_size;
} queue_t;

int EnQueue(queue_t *pipe, int fd);
int DeQueue(queue_t *pqueue);
