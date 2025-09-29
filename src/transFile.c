#include "threadPool.h"

typedef struct train_s
{
    int length;
    char data[1024];
} train_t;

int transFile(int netfd)
{
    // train_t train = {5, "file1"};
    train_t train = {7, "SQL.pdf"};
    send(netfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    // int fd = open("file1", O_RDWR);
    int fd = open("SQL.pdf", O_RDWR);
    struct stat statbuf;
    fstat(fd, &statbuf);
    printf("file size:%ld\n", statbuf.st_size);
    train.length = sizeof(off_t); //
    memcpy(train.data, &statbuf.st_size, train.length);
    send(netfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

    sendfile(netfd, fd, NULL, statbuf.st_size);
    sleep(5);
    printf("sleep over!\n");
    return 0;
}