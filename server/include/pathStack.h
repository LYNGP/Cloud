#include "func.h"
#ifndef __pathStack__
#define __pathStack__
typedef struct pathStackNode_s
{
    char *dirName;
    struct pathStackNode_s *pNext;
} pathStackNode_t;
typedef struct pathStack_s
{
    pathStackNode_t *pFront;
} pathStack_t;

int pathStackInit(pathStack_t *pPathStack);
int pushPathStack(pathStack_t *pPathStack, const char *dirName);
int popPathStack(pathStack_t *pPathStack);
int getTopPathStack(pathStack_t *pPathStack, char *dirName);
int getCurPath(pathStack_t *pPathStack, char *pwd);
#endif
