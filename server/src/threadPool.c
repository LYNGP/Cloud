#include "threadPool.h"

int tidArrInit(tidArr_t *ptidArr, int workerNum)
{
    ptidArr->arr = (pthread_t *)calloc(workerNum, sizeof(pthread_t));
    ptidArr->workerNum = workerNum;
    return 0;
}

int threadPoolInit(threadPool_t *pthreadPool, int workerNum)
{
    bzero(pthreadPool, sizeof(threadPool_t));
    tidArrInit(&pthreadPool->tidArr, workerNum);
    taskQueueInit(&pthreadPool->taskQueue);
    pthread_mutex_init(&pthreadPool->mutex, NULL);
    pthread_cond_init(&pthreadPool->cond, NULL);
    return 0;
}

int connectMySQL(MYSQL *mysql)
{
    // 从文件中读取出要连接数据库的参数
    // 独处配置文件中的所有内容,配置文件的内容的顺序不能修改，并且不应该大于40960字节

    // 打开配置文件
    int fd = open("../conf/mysql.conf", O_RDONLY);
    // 读取内容
    char conf[40960] = {0};
    read(fd, conf, sizeof(conf));
    // 关闭配置文件
    close(fd);

    // 切分配置文件的内容
    char *argv[1024];
    int arglen = 0;
    argv[0] = strtok(conf, " :\n");
    // printf("argv[0] = %s\n",argv[0]);
    if (argv[0] == NULL)
    {
        printf("please check your mysql.conf!\n");
        return -1;
    }
    arglen++;
    for (int i = 1; i < 1024; i++)
    {
        argv[i] = strtok(NULL, " :\n");
        if (argv[i] == NULL)
        {
            break;
        }
        ++arglen;
        // printf("argv[%d]= %s\n",i,argv[i]);
    }

    MYSQL *cret = mysql_real_connect(mysql, argv[1], argv[3], argv[5], argv[7], 0, NULL, 0);
    if (cret == NULL)
    {
        printf("mysql_real_connect:%s\n", mysql_error(mysql));
        return -1;
    }

    return 0;
}

void unlock(void *arg)
{ // 包装函数
    threadPool_t *pthreadPool = (threadPool_t *)arg;
    printf("unlock!\n");
    pthread_mutex_unlock(&pthreadPool->mutex);
}

void *threadFunc(void *arg)
{
    threadPool_t *pthreadPool = (threadPool_t *)arg;

    // 连接数据库
    pthread_mutex_lock(&pthreadPool->mutex);
    MYSQL *mysql;
    mysql = mysql_init(NULL);
    connectMySQL(mysql);
    pthread_mutex_unlock(&pthreadPool->mutex);

    // 子线程,消费者
    while (1)
    {
        int netfd;
        pthread_mutex_lock(&pthreadPool->mutex); // 加锁
        // pthread_cleanup_push(unlock,pthreadPool); //压栈
        while (pthreadPool->taskQueue.queueSize <= 0 && pthreadPool->exitFlag == 0)
        {
            pthread_cond_wait(&pthreadPool->cond, &pthreadPool->mutex);
        }
        if (pthreadPool->exitFlag == 1)
        {
            printf("I am child,I am going to exit!\n");
            pthread_mutex_unlock(&pthreadPool->mutex);
            pthread_exit(NULL);
        }
        // 队列不为空
        netfd = pthreadPool->taskQueue.pFront->netfd;
        printf("I am worker,I got a netfd = %d\n", netfd);
        deQueue(&pthreadPool->taskQueue);
        pthread_mutex_unlock(&pthreadPool->mutex);
        // pthread_cleanup_pop(1);                 //弹栈

        // 执行业务
        business(netfd, mysql);

        close(netfd);
    }
    // 关闭数据连接
    mysql_close(mysql);
}

int makeWorker(threadPool_t *pthreadPool)
{
    for (int i = 0; i < pthreadPool->tidArr.workerNum; ++i)
    {
        pthread_create(&pthreadPool->tidArr.arr[i], NULL, threadFunc, pthreadPool);
    }
    return 0;
}
