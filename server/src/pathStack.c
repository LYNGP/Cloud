#include "pathStack.h"
int pathStackInit(pathStack_t *pPathStack)
{
    memset(pPathStack, 0, sizeof(pathStack_t));
    return 0;
}
int pushPathStack(pathStack_t *pPathStack, const char *dirName)
{
    pathStackNode_t *pNew = (pathStackNode_t *)malloc(sizeof(pathStackNode_t));
    memset(pNew, 0, sizeof(pathStackNode_t));
    pNew->dirName = (char *)calloc(1, sizeof(dirName));
    memcpy(pNew->dirName, dirName, strlen(dirName));
    if (pPathStack->pFront == NULL)
    {
        pNew->pNext = NULL;
        pPathStack->pFront = pNew;
        // printf("%s\n",pPathStack->pFront->dirName);
        return 0;
    }
    pNew->pNext = pPathStack->pFront;
    pPathStack->pFront = pNew;
    // printf("%s\n",pPathStack->pFront->dirName);
    return 0;
}
int popPathStack(pathStack_t *pPathStack)
{
    pathStackNode_t *pCur = pPathStack->pFront;
    if (pCur == NULL)
    {
        return -1;
    }
    pPathStack->pFront = pCur->pNext;
    free(pCur);
    return 0;
}
int getTopPathStack(pathStack_t *pPathStack, char *dirName)
{
    if (pPathStack->pFront != NULL)
        memcpy(dirName, pPathStack->pFront->dirName, strlen(pPathStack->pFront->dirName));
    return 0;
}
int getCurPath(pathStack_t *pPathStack, char *pwd)
{
    pathStackNode_t *pCur = pPathStack->pFront;
    char path[4096] = {0};
    char tempPath[4096] = {0};
    while (1)
    {
        if (pCur == NULL)
        {
            break;
        }
        snprintf(tempPath, sizeof(tempPath), "/%s%s", pCur->dirName, path);
        memcpy(path, tempPath, sizeof(tempPath));
        memset(tempPath, 0, sizeof(tempPath));
        pCur = pCur->pNext;
    }
    memcpy(pwd, path, strlen(path));
}
