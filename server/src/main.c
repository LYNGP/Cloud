#include"threadPool.h"
int exitPipe[2];
void handler(int signum){
    printf("signum = %d\n",signum);
    write(exitPipe[1],"1",1);
}

int main(int argc,char*argv[])
{
    // ./server 192.168.30.128 1234 3
    ARGS_CHECK(argc,4);

    pipe(exitPipe);
    if(fork()!= 0){
        //  父进程接收信号，通过管道给子进程发送退出信息
        close(exitPipe[0]);
        signal(SIGUSR1,handler);
        wait(NULL);
        printf("Parent is going to exit!\n");
        exit(0);
    }
    // 只有子进程才能创建线程池
    close(exitPipe[1]);
    int sockfd;
    tcpInit(argv[1],argv[2],&sockfd);

    // 创建线程池
    int workerNum = atoi(argv[3]);
    threadPool_t threadPool;
    threadPoolInit(&threadPool,workerNum);
    makeWorker(&threadPool);

    // 将sockfd加入监听
    int epfd = epoll_create(1);
    epollAdd(epfd,sockfd);

    //将exitPipe[0]加入监听，当就绪时表示要退出线程池
    epollAdd(epfd,exitPipe[0]); 

    //主线程，生产者
    //创建储存文件的目录                                                                                 
    mkdir("netDisk",0777);
    //修改当前工作目录
    chdir("netDisk");
    while(1){
        struct epoll_event readySet[1024];
        int readyNum = epoll_wait(epfd,readySet,1024,-1);
        for(int i=0;i<readyNum;++i){
            if(readySet[i].data.fd == sockfd){
                int netfd = accept(sockfd,NULL,NULL);
                SYSTEM_LOG(LOG_NOTICE,"","logging...");
                printf("I got 1 task!\n");
                // 分配任务
                pthread_mutex_lock(&threadPool.mutex);      //加锁
                enQueue(&threadPool.taskQueue,netfd);        // 入队
                printf("I am master,I send a netfd = %d\n",netfd); 
                pthread_cond_signal(&threadPool.cond);      //通知
                pthread_mutex_unlock(&threadPool.mutex);    //解锁
            }else if(readySet[i].data.fd == exitPipe[0]){
                // 退出线程池
                //for(int j = 0;j<threadPool.tidArr.workerNum;++j){
                //    pthread_cancel(threadPool.tidArr.arr[j]);
                //}
            
                //修改退出标志位，要加锁，并唤醒条件变量
                pthread_mutex_lock(&threadPool.mutex);
                threadPool.exitFlag =1;
                pthread_cond_broadcast(&threadPool.cond);
                pthread_mutex_unlock(&threadPool.mutex);
                for(int j = 0;j<threadPool.tidArr.workerNum;++j){
                    pthread_join(threadPool.tidArr.arr[j],NULL);
                }
                printf("main thread is going to exit!\n");
                exit(0);
            }       
        }
    }
    return 0;
}

