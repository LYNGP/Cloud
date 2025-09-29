#include "threadPool.h"

int epollAdd(int epfd, int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    ERROR_CHECK(ret, -1, "epollAdd error");
    return 0;
}

int epollDel(int epfd, int fd)
{
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    ERROR_CHECK(ret, -1, "epollDel error");
    return 0;
}