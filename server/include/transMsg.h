#include"func.h"
#ifndef __transMsg__
#define __transMsg__
typedef struct train_s{
    int length;
    char data[40960];
}train_t;
int recvn(int sockfd,void* buf,long total);
#endif


